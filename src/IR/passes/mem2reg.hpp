#pragma once

#include "../custom_ir.hpp"
#include "tool.hpp"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace IR {
namespace Pass {

class MemToReg {
private:
  using AllocPtr = const IRAlloc*;

public:
  void PromoteFunction(IRFunction& function) {
    if (function.blocks.empty()) return;

    DominatorTree dominator_tree;
    dominator_tree.Analyze(function);

    // 第一次扫描只收集标量 alloc；第二次扫描统一验证所有 use。
    // 所有 alloc 共享扫描结果，避免局部变量较多时出现 O(A * I) 复杂度。
    std::vector<std::shared_ptr<IRAlloc>> candidates;
    std::unordered_map<const IRValue*, std::shared_ptr<IRAlloc>> candidate_owner;
    for (const auto& block : function.blocks) {
      for (const auto& instruction : block->insts) {
        if (isa<IRAlloc>(instruction)) {
          auto alloc = cast<IRAlloc>(instruction);
          if (!alloc->IsArray()) {
            candidates.push_back(alloc);
            candidate_owner.emplace(alloc.get(), alloc);
          }
        }
      }
    }
    if (candidates.empty()) return;

    std::unordered_set<AllocPtr> invalid;
    for (const auto& block : function.blocks) {
      for (const auto& instruction : block->insts) {
        for (const auto& use : instruction->GetUses()) {
          const auto found = candidate_owner.find(use.get());
          if (found == candidate_owner.end()) continue;
          AllocPtr alloc = found->second.get();
          const auto* load = dyn_cast<IRLoad>(instruction.get());
          const auto* store = dyn_cast<IRStore>(instruction.get());
          const bool direct_load = load && load->GetPtr().get() == alloc;
          const bool direct_store = store && store->GetPtr().get() == alloc;
          if (!direct_load && !direct_store) invalid.insert(alloc);
        }
      }
    }

    std::vector<std::shared_ptr<IRAlloc>> allocas;
    allocas.reserve(candidates.size());
    for (auto& candidate : candidates) {
      if (invalid.count(candidate.get()) == 0) allocas.push_back(candidate);
    }
    if (allocas.empty()) return;

    std::unordered_set<AllocPtr> promotable;
    for (const auto& alloc : allocas) promotable.insert(alloc.get());

    // 一次扫描同时建立每个变量的定义块集合。
    std::unordered_map<AllocPtr, std::unordered_set<IRBasicBlock*>> def_blocks;
    for (const auto& block : function.blocks) {
      for (const auto& instruction : block->insts) {
        const auto* store = dyn_cast<IRStore>(instruction.get());
        if (!store) continue;
        const auto* alloc = dyn_cast<IRAlloc>(store->GetPtr().get());
        if (alloc && promotable.count(alloc) != 0)
          def_blocks[alloc].insert(block.get());
      }
    }

    // 计算迭代支配边界并插入块参数（本 IR 中块参数等价于 phi）。
    std::unordered_map<AllocPtr,
                       std::unordered_map<IRBasicBlock*,
                                          std::shared_ptr<IRBlockParam>>>
        phi_params;
    std::unordered_map<
        IRBasicBlock*,
        std::vector<std::pair<AllocPtr, std::shared_ptr<IRBlockParam>>>>
        params_by_block;
    int parameter_counter = 0;
    for (const auto& alloc : allocas) {
      std::vector<IRBasicBlock*> worklist(def_blocks[alloc.get()].begin(),
                                          def_blocks[alloc.get()].end());
      std::unordered_set<IRBasicBlock*> queued(worklist.begin(),
                                                worklist.end());
      std::unordered_set<IRBasicBlock*> has_parameter;
      std::size_t head = 0;
      while (head < worklist.size()) {
        IRBasicBlock* block = worklist[head++];
        const auto frontier = dominator_tree.dom_frontier.find(block);
        if (frontier == dominator_tree.dom_frontier.end()) continue;
        for (IRBasicBlock* target : frontier->second) {
          if (!has_parameter.insert(target).second) continue;
          if (queued.insert(target).second) worklist.push_back(target);
        }
      }

      // 按 alloc 在函数中的稳定顺序添加，跳转实参也使用相同顺序追加。
      for (const auto& block : function.blocks) {
        if (has_parameter.count(block.get()) == 0) continue;
        auto parameter = block->AddParam(
            alloc->GetName() + "_" + std::to_string(parameter_counter++),
            alloc->GetAllocatedType());
        phi_params[alloc.get()][block.get()] = parameter;
        params_by_block[block.get()].push_back({alloc.get(), parameter});
      }
    }

    std::unordered_map<AllocPtr, std::vector<std::shared_ptr<IRValue>>> stacks;
    for (const auto& alloc : allocas) {
      std::shared_ptr<IRValue> initial;
      if (alloc->GetAllocatedType() == IRType::IR_FLOAT)
        initial = std::make_shared<IRConstant>(0.0f);
      else
        initial = std::make_shared<IRConstant>(int32_t{0});
      stacks[alloc.get()].push_back(std::move(initial));
    }

    std::unordered_map<const IRValue*, std::shared_ptr<IRValue>> replacements;
    auto resolve = [&](std::shared_ptr<IRValue> value) {
      std::unordered_set<const IRValue*> visited;
      while (value && replacements.count(value.get()) != 0 &&
             visited.insert(value.get()).second)
        value = replacements.at(value.get());
      return value;
    };

    // 所有变量共用一次支配树 DFS。每个变量各自维护值栈，进入/离开块时
    // 同步 push/pop，语义与经典 Cytron SSA 重命名算法一致。
    std::function<void(IRBasicBlock*)> rename = [&](IRBasicBlock* block) {
      std::unordered_map<AllocPtr, std::size_t> pushed;
      const auto block_parameters = params_by_block.find(block);
      if (block_parameters != params_by_block.end()) {
        for (const auto& [alloc, parameter] : block_parameters->second) {
          stacks[alloc].push_back(parameter);
          ++pushed[alloc];
        }
      }

      std::vector<std::shared_ptr<IRInstruction>> kept;
      kept.reserve(block->insts.size());
      for (const auto& instruction : block->insts) {
        if (const auto* alloc = dyn_cast<IRAlloc>(instruction.get())) {
          if (promotable.count(alloc) != 0) continue;
        }
        if (const auto* store = dyn_cast<IRStore>(instruction.get())) {
          const auto* alloc = dyn_cast<IRAlloc>(store->GetPtr().get());
          if (alloc && promotable.count(alloc) != 0) {
            stacks[alloc].push_back(resolve(store->GetVal()));
            ++pushed[alloc];
            continue;
          }
        }
        if (const auto* load = dyn_cast<IRLoad>(instruction.get())) {
          const auto* alloc = dyn_cast<IRAlloc>(load->GetPtr().get());
          if (alloc && promotable.count(alloc) != 0) {
            replacements[instruction.get()] = resolve(stacks[alloc].back());
            continue;
          }
        }
        kept.push_back(instruction);
      }
      block->insts = std::move(kept);

      if (!block->insts.empty()) {
        const auto& terminator = block->insts.back();
        auto append_arguments = [&](IRBasicBlock* successor,
                                    std::vector<std::shared_ptr<IRValue>> args) {
          const auto parameters = params_by_block.find(successor);
          if (parameters != params_by_block.end()) {
            for (const auto& [alloc, parameter] : parameters->second) {
              (void)parameter;
              args.push_back(resolve(stacks[alloc].back()));
            }
          }
          return args;
        };

        if (auto* jump = dyn_cast<IRJump>(terminator.get())) {
          auto* successor = dyn_cast<IRBasicBlock>(jump->GetTarget().get());
          if (successor)
            jump->SetArgs(append_arguments(successor, jump->GetArgs()));
        } else if (auto* branch = dyn_cast<IRBranch>(terminator.get())) {
          auto* then_block =
              dyn_cast<IRBasicBlock>(branch->GetThenTarget().get());
          auto* else_block =
              dyn_cast<IRBasicBlock>(branch->GetElseTarget().get());
          if (then_block)
            branch->SetThenArgs(
                append_arguments(then_block, branch->GetThenArgs()));
          if (else_block)
            branch->SetElseArgs(
                append_arguments(else_block, branch->GetElseArgs()));
        }
      }

      const auto children = dominator_tree.dom_children.find(block);
      if (children != dominator_tree.dom_children.end()) {
        for (IRBasicBlock* child : children->second) rename(child);
      }
      for (const auto& [alloc, count] : pushed) {
        for (std::size_t index = 0; index < count; ++index)
          stacks[alloc].pop_back();
      }
    };
    rename(function.blocks.front().get());

    // load 的替换也集中在最后一次线性扫描完成。
    for (const auto& block : function.blocks) {
      for (const auto& instruction : block->insts) {
        for (const auto& use : instruction->GetUses()) {
          if (!use) continue;
          auto replacement = resolve(use);
          if (replacement.get() != use.get())
            instruction->ReplaceUse(use.get(), std::move(replacement));
        }
      }
    }
  }

  void Run(IRProgram* program) {
    if (!program) return;
    for (auto& function : program->funcs) {
      if (function) PromoteFunction(*function);
    }
  }
};

}  // namespace Pass
}  // namespace IR
