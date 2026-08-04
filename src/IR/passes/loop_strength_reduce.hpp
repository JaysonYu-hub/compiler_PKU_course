#pragma once

#include "induction_variable.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace IR {
namespace Pass {

struct LoopStrengthReduceStats {
  std::size_t loops_seen = 0;
  std::size_t preheaders_created = 0;
  std::size_t induction_variables = 0;
  std::size_t addresses_rewritten = 0;
};

// 将 base + iv * element_size 改写为循环携带的递增指针：
//
//   p = gep base, iv              ptr = phi(gep(base, init), ptr.next)
//                                 ptr.next = gep ptr, step
//
// 这仍然每轮执行一次地址更新，但动态索引的乘法/移位变成常量步长加法。
class LoopStrengthReduction {
 public:
  bool Run(IRProgram* program) {
    stats_ = {};
    if (!program) return false;
    bool changed = false;
    for (const auto& function : program->funcs) {
      if (!function || function->blocks.empty()) continue;
      changed |= RunFunction(*function);
    }
    return changed;
  }

  const LoopStrengthReduceStats& GetStats() const { return stats_; }

 private:
  struct AddressCandidate {
    std::shared_ptr<IRInstruction> instruction;
    std::shared_ptr<IRValue> base;
    int element_size = 0;
    bool is_element_pointer = false;
  };

  static void InsertBeforeTerminator(
      IRBasicBlock& block, std::shared_ptr<IRInstruction> instruction) {
    const auto position = block.insts.empty() ? block.insts.end()
                                               : std::prev(block.insts.end());
    block.insts.insert(position, std::move(instruction));
  }

  static std::unordered_map<const IRValue*, IRBasicBlock*> BuildDefinitions(
      const IRFunction& function) {
    std::unordered_map<const IRValue*, IRBasicBlock*> definitions;
    for (const auto& block : function.blocks) {
      for (const auto& parameter : block->params)
        definitions[parameter.get()] = block.get();
      for (const auto& instruction : block->insts)
        definitions[instruction.get()] = block.get();
    }
    return definitions;
  }

  static bool HasPotentialAddressCandidate(const IRFunction& function) {
    for (const auto& block : function.blocks) {
      for (const auto& instruction : block->insts) {
        if (const auto* pointer = dyn_cast<IRGetElemPtr>(instruction.get())) {
          if (isa<IRBlockParam>(pointer->GetIndex())) return true;
        } else if (const auto* pointer =
                       dyn_cast<IRGetPtr>(instruction.get())) {
          if (isa<IRBlockParam>(pointer->GetIndex())) return true;
        }
      }
    }
    return false;
  }

  static bool IsLoopInvariant(
      const std::shared_ptr<IRValue>& value, const CanonicalLoopInfo& loop,
      const std::unordered_map<const IRValue*, IRBasicBlock*>& definitions) {
    if (!value) return false;
    if (isa<IRConstant>(value) || isa<IRGlobalAlloc>(value) ||
        isa<IRFuncParam>(value) || isa<IRAlloc>(value))
      return true;
    const auto definition = definitions.find(value.get());
    return definition != definitions.end() &&
           loop.blocks.count(definition->second) == 0;
  }

  static std::vector<AddressCandidate> FindCandidates(
      const CanonicalLoopInfo& loop, const BasicInductionVariable& induction,
      const std::unordered_map<const IRValue*, IRBasicBlock*>& definitions) {
    std::vector<AddressCandidate> candidates;
    for (IRBasicBlock* block : loop.blocks) {
      if (!block) continue;
      for (const auto& instruction : block->insts) {
        if (const auto* pointer = dyn_cast<IRGetElemPtr>(instruction.get())) {
          if (pointer->GetIndex().get() != induction.parameter.get() ||
              pointer->GetElemSize() <= 0 ||
              !IsLoopInvariant(pointer->GetArr(), loop, definitions))
            continue;
          candidates.push_back({instruction, pointer->GetArr(),
                                pointer->GetElemSize(), true});
        } else if (const auto* pointer =
                       dyn_cast<IRGetPtr>(instruction.get())) {
          if (pointer->GetIndex().get() != induction.parameter.get() ||
              pointer->GetElemSize() <= 0 ||
              !IsLoopInvariant(pointer->GetPtr(), loop, definitions))
            continue;
          candidates.push_back({instruction, pointer->GetPtr(),
                                pointer->GetElemSize(), false});
        }
      }
    }
    return candidates;
  }

  static std::shared_ptr<IRInstruction> MakeInitialAddress(
      const AddressCandidate& candidate,
      const BasicInductionVariable& induction, const std::string& name) {
    if (candidate.is_element_pointer)
      return std::make_shared<IRGetElemPtr>(
          name, candidate.base, induction.initial, candidate.element_size);
    return std::make_shared<IRGetPtr>(name, candidate.base,
                                      induction.initial,
                                      candidate.element_size);
  }

  bool RewriteCandidate(IRFunction& function, CanonicalLoopInfo& loop,
                        const BasicInductionVariable& induction,
                        const AddressCandidate& candidate,
                        LoopValueNameGenerator& names) {
    if (!loop.preheader || loop.latches.size() != 1 ||
        !candidate.instruction)
      return false;
    IRBasicBlock* latch = *loop.latches.begin();
    if (!GetJumpEdgeArguments(*loop.preheader, loop.header) ||
        !GetJumpEdgeArguments(*latch, loop.header))
      return false;

    // 初值只在 preheader 计算一次。即使循环零次执行，纯指针运算也不会
    // 访问内存，因此可以安全提前计算。
    auto initial_address = MakeInitialAddress(
        candidate, induction, names.Next("%ivaddr_init"));
    InsertBeforeTerminator(*loop.preheader, initial_address);

    auto pointer_parameter =
        loop.header->AddParam(names.Next("%ivaddr"), IRType::IR_POINTER);
    if (!AppendEdgeArgument(*loop.preheader, loop.header, initial_address))
      return false;

    auto step = std::make_shared<IRConstant>(induction.step);
    auto next_address = std::make_shared<IRGetPtr>(
        names.Next("%ivaddr_next"), pointer_parameter, step,
        candidate.element_size);
    InsertBeforeTerminator(*latch, next_address);
    if (!AppendEdgeArgument(*latch, loop.header, next_address)) return false;

    // 新指针块参数支配整个自然循环。替换全函数 use 后删除旧动态索引
    // 地址，后续 DCE 会清理因此失去用途的中间算术。
    function.ReplaceAllUses(candidate.instruction.get(), pointer_parameter);
    for (IRBasicBlock* block : loop.blocks) {
      if (block && block->RemoveInstruction(candidate.instruction.get()))
        break;
    }
    ++stats_.addresses_rewritten;
    return true;
  }

  bool RunFunction(IRFunction& function) {
    // 超长控制流函数可能完全没有数组归纳地址。先做一次线性筛选，避免
    // 无收益地重复构建支配树和自然循环，控制编译器自身运行时间。
    if (!HasPotentialAddressCandidate(function)) return false;

    DominatorTree dominator_tree;
    dominator_tree.Analyze(function);
    LoopAnalysis loop_analysis;
    auto loops = loop_analysis.FindLoops(function, dominator_tree);
    LoopValueNameGenerator names(function);
    InductionVariableAnalysis induction_analysis;
    bool changed = false;

    for (auto& loop : loops) {
      ++stats_.loops_seen;
      std::vector<IRBasicBlock*> external;
      for (IRBasicBlock* predecessor :
           function.GetPredecessors(loop.header)) {
        if (loop.blocks.count(predecessor) == 0)
          external.push_back(predecessor);
      }
      const bool already_canonical =
          external.size() == 1 &&
          IsDedicatedPreheader(external.front(), loop.header);
      loop.preheader = EnsureDedicatedPreheader(function, loop, names);
      if (!loop.preheader) continue;
      if (!already_canonical) ++stats_.preheaders_created;

      const auto inductions = induction_analysis.Analyze(loop);
      stats_.induction_variables += inductions.size();
      for (const auto& induction : inductions) {
        // 每次改写后重新建立定义表；前一个候选可能已经把内层循环的
        // base 替换成新块参数。
        const auto definitions = BuildDefinitions(function);
        const auto candidates =
            FindCandidates(loop, induction, definitions);
        for (const auto& candidate : candidates)
          changed |= RewriteCandidate(function, loop, induction, candidate,
                                      names);
      }
    }
    return changed;
  }

  LoopStrengthReduceStats stats_;
};

}  // namespace Pass
}  // namespace IR
