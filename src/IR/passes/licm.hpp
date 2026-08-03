#pragma once

#include "../custom_ir.hpp"
#include "tool.hpp"
#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace IR {
namespace Pass {

// 一个循环头可能有多条回边，因此以 header 为单位合并自然循环。
struct LoopInfo {
  IRBasicBlock* header = nullptr;
  std::unordered_set<IRBasicBlock*> blocks;
  std::unordered_set<IRBasicBlock*> latches;
  IRBasicBlock* pre_header = nullptr;
};

class LICM {
public:
  std::vector<LoopInfo> FindLoops(IRFunction& function,
                                  const DominatorTree& dom_tree) {
    std::vector<LoopInfo> loops;
    std::unordered_map<IRBasicBlock*, std::size_t> loop_by_header;

    for (const auto& block_ptr : function.blocks) {
      IRBasicBlock* latch = block_ptr.get();
      for (IRBasicBlock* header : latch->GetSuccessors()) {
        if (!header || !dom_tree.Dominates(header, latch)) continue;

        std::unordered_set<IRBasicBlock*> natural_loop = {header, latch};
        std::vector<IRBasicBlock*> worklist;
        // 自环的自然循环只包含循环头，不能把头结点的外部前驱吸进循环。
        if (latch != header) worklist.push_back(latch);
        while (!worklist.empty()) {
          IRBasicBlock* current = worklist.back();
          worklist.pop_back();
          for (IRBasicBlock* predecessor :
               function.GetPredecessors(current)) {
            if (!predecessor || predecessor == header) continue;
            if (natural_loop.insert(predecessor).second)
              worklist.push_back(predecessor);
          }
        }

        auto [it, inserted] =
            loop_by_header.emplace(header, loops.size());
        if (inserted) {
          LoopInfo loop;
          loop.header = header;
          loops.push_back(std::move(loop));
        }
        LoopInfo& loop = loops[it->second];
        loop.blocks.insert(natural_loop.begin(), natural_loop.end());
        loop.latches.insert(latch);
      }
    }
    return loops;
  }

  IRBasicBlock* EnsurePreheader(IRFunction& function, LoopInfo& loop) {
    IRBasicBlock* header = loop.header;
    if (!header) return nullptr;

    std::vector<IRBasicBlock*> external_predecessors;
    for (IRBasicBlock* predecessor : function.GetPredecessors(header)) {
      if (loop.blocks.count(predecessor) == 0)
        external_predecessors.push_back(predecessor);
    }
    if (external_predecessors.empty()) return nullptr;

    // 唯一外部前驱只有一条出边时，它已经是合法 preheader。
    if (external_predecessors.size() == 1) {
      IRBasicBlock* predecessor = external_predecessors.front();
      const auto successors = predecessor->GetSuccessors();
      if (successors.size() == 1 && successors.front() == header) {
        loop.pre_header = predecessor;
        return predecessor;
      }
    }

    // 多个入口边先汇合到专用 preheader。header 的块参数也要在这里转发，
    // 否则新建基本块会破坏 SSA 边实参与形参的对应关系。
    std::string name = header->name + ".licm_preheader";
    unsigned suffix = 0;
    auto name_exists = [&](const std::string& candidate) {
      return std::any_of(function.blocks.begin(), function.blocks.end(),
                         [&](const auto& block) {
                           return block->name == candidate;
                         });
    };
    while (name_exists(name))
      name = header->name + ".licm_preheader." + std::to_string(++suffix);

    auto preheader = std::make_shared<IRBasicBlock>(name);
    std::vector<std::shared_ptr<IRValue>> forwarded_arguments;
    forwarded_arguments.reserve(header->params.size());
    for (const auto& parameter : header->params) {
      auto forwarded = preheader->AddParam(parameter->GetName() + ".licm_pre",
                                           parameter->GetType());
      forwarded_arguments.push_back(forwarded);
    }
    preheader->AddInstruction(
        std::make_shared<IRJump>(header->shared_from_this(),
                                 std::move(forwarded_arguments)));

    for (IRBasicBlock* predecessor : external_predecessors) {
      if (predecessor->insts.empty()) continue;
      const auto& terminator = predecessor->insts.back();
      if (const auto* jump = dyn_cast<IRJump>(terminator.get())) {
        if (jump->GetTarget().get() == header) {
          predecessor->insts.back() =
              std::make_shared<IRJump>(preheader, jump->GetArgs());
        }
      } else if (const auto* branch = dyn_cast<IRBranch>(terminator.get())) {
        if (branch->GetThenTarget().get() == header) {
          predecessor->insts.back() = std::make_shared<IRBranch>(
              branch->GetCond(), preheader, branch->GetElseTarget(),
              branch->GetThenArgs(), branch->GetElseArgs());
        } else if (branch->GetElseTarget().get() == header) {
          predecessor->insts.back() = std::make_shared<IRBranch>(
              branch->GetCond(), branch->GetThenTarget(), preheader,
              branch->GetThenArgs(), branch->GetElseArgs());
        }
      }
    }

    const auto header_position =
        std::find_if(function.blocks.begin(), function.blocks.end(),
                     [&](const auto& block) { return block.get() == header; });
    function.blocks.insert(header_position, preheader);
    loop.pre_header = preheader.get();
    return loop.pre_header;
  }

  bool IsInvariant(
      const LoopInfo& loop, const IRInstruction& instruction,
      const std::unordered_map<const IRValue*, IRBasicBlock*>& definitions,
      const std::unordered_set<const IRValue*>& invariants) const {
    for (const auto& operand : instruction.GetUses()) {
      if (!operand) continue;
      if (isa<IRConstant>(operand) || isa<IRGlobalAlloc>(operand) ||
          isa<IRFuncParam>(operand))
        continue;

      const auto definition = definitions.find(operand.get());
      if (definition != definitions.end() &&
          (loop.blocks.count(definition->second) == 0 ||
           invariants.count(operand.get()) != 0))
        continue;
      return false;
    }
    return true;
  }

  bool IsSafeToSpeculate(const IRInstruction& instruction) const {
    // 当前阶段只外提不访问内存、不会改变控制流的纯计算。load 即使没有
    // 别名写，也可能来自条件路径上的无效地址；等指针来源和 must-execute
    // 分析完善后再单独开放 load 外提。
    switch (instruction.inst_type) {
      case IRInstructionType::IR_INST_BINARY:
      case IRInstructionType::IR_INST_CAST:
      case IRInstructionType::IR_INST_GETPTR:
      case IRInstructionType::IR_INST_GETELEMPTR:
        return true;
      default:
        return false;
    }
  }

  bool IsMustExecuteBlock(const LoopInfo& loop, IRBasicBlock* block,
                          const DominatorTree& dom_tree) const {
    if (!block || loop.latches.empty()) return false;
    // 只有支配每条回边源的块，才是每轮回到循环头之前必经的块。
    return std::all_of(loop.latches.begin(), loop.latches.end(),
                       [&](IRBasicBlock* latch) {
                         return dom_tree.Dominates(block, latch);
                       });
  }

  bool OptimizeLoop(
      IRFunction& function, LoopInfo& loop, const DominatorTree& dom_tree,
      const std::unordered_map<const IRValue*, IRBasicBlock*>& definitions) {
    std::unordered_set<const IRValue*> invariants;
    bool changed = true;
    while (changed) {
      changed = false;
      // 使用函数中的稳定基本块顺序，避免 unordered_set 遍历使结果和
      // 指令依赖顺序随机变化。
      for (const auto& block_ptr : function.blocks) {
        IRBasicBlock* block = block_ptr.get();
        if (loop.blocks.count(block) == 0 ||
            !IsMustExecuteBlock(loop, block, dom_tree))
          continue;
        for (const auto& instruction : block->insts) {
          if (invariants.count(instruction.get()) != 0 ||
              !IsSafeToSpeculate(*instruction))
            continue;
          if (IsInvariant(loop, *instruction, definitions, invariants)) {
            invariants.insert(instruction.get());
            changed = true;
          }
        }
      }
    }
    if (invariants.empty()) return false;
    if (!EnsurePreheader(function, loop)) return false;

    // 以“依赖已排入”作为移动条件，得到稳定的拓扑顺序。这样即便函数的
    // 基本块容器顺序不是严格 RPO，定义也一定先于使用进入 preheader。
    std::vector<std::shared_ptr<IRInstruction>> ordered;
    std::unordered_set<const IRValue*> scheduled;
    while (scheduled.size() < invariants.size()) {
      bool progress = false;
      for (const auto& block_ptr : function.blocks) {
        if (loop.blocks.count(block_ptr.get()) == 0) continue;
        for (const auto& instruction : block_ptr->insts) {
          if (invariants.count(instruction.get()) == 0 ||
              scheduled.count(instruction.get()) != 0)
            continue;
          bool dependencies_ready = true;
          for (const auto& operand : instruction->GetUses()) {
            if (operand && invariants.count(operand.get()) != 0 &&
                scheduled.count(operand.get()) == 0) {
              dependencies_ready = false;
              break;
            }
          }
          if (!dependencies_ready) continue;
          ordered.push_back(instruction);
          scheduled.insert(instruction.get());
          progress = true;
        }
      }
      if (!progress) break;
    }
    if (ordered.empty()) return false;

    for (const auto& block_ptr : function.blocks) {
      if (loop.blocks.count(block_ptr.get()) == 0) continue;
      auto& instructions = block_ptr->insts;
      instructions.erase(
          std::remove_if(instructions.begin(), instructions.end(),
                         [&](const auto& instruction) {
                           return scheduled.count(instruction.get()) != 0;
                         }),
          instructions.end());
    }

    auto& preheader_instructions = loop.pre_header->insts;
    const auto insertion_point =
        preheader_instructions.empty() ? preheader_instructions.end()
                                       : std::prev(preheader_instructions.end());
    preheader_instructions.insert(insertion_point, ordered.begin(),
                                  ordered.end());
    return true;
  }

  void Run(IRProgram* program) {
    if (!program) return;
    for (auto& function : program->funcs) {
      if (!function || function->blocks.empty()) continue;

      DominatorTree dominator_tree;
      dominator_tree.Analyze(*function);
      auto loops = FindLoops(*function, dominator_tree);

      std::unordered_map<const IRValue*, IRBasicBlock*> definitions;
      for (const auto& block : function->blocks) {
        for (const auto& parameter : block->params)
          definitions[parameter.get()] = block.get();
        for (const auto& instruction : block->insts)
          definitions[instruction.get()] = block.get();
      }

      for (auto& loop : loops)
        OptimizeLoop(*function, loop, dominator_tree, definitions);
    }
  }
};

}  // namespace Pass
}  // namespace IR
