#pragma once

#include "induction_variable.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
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
  std::size_t affine_candidates = 0;
  std::size_t rejected_by_cost = 0;
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
    std::shared_ptr<IRValue> index;
    std::int64_t induction_coefficient = 0;
    int removable_cost = 0;
    bool is_direct_induction = false;
    int element_size = 0;
    bool is_element_pointer = false;
  };

  static constexpr std::size_t kMaxAddressesPerInduction = 8;
  static constexpr std::size_t kMaxAffineAddressesPerFunction = 6;

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

  static std::unordered_map<const IRValue*, std::size_t> BuildUseCounts(
      const IRFunction& function) {
    std::unordered_map<const IRValue*, std::size_t> use_counts;
    for (const auto& block : function.blocks) {
      for (const auto& instruction : block->insts) {
        for (const auto& operand : instruction->GetUses()) {
          if (operand) ++use_counts[operand.get()];
        }
      }
    }
    return use_counts;
  }

  static std::size_t MaxBlockParameterCount(const IRFunction& function) {
    std::size_t maximum = 0;
    for (const auto& block : function.blocks)
      maximum = std::max(maximum, block->params.size());
    return maximum;
  }

  static bool ContainsBlockParameter(
      const std::shared_ptr<IRValue>& value,
      std::unordered_set<const IRValue*>& visited) {
    if (!value || !visited.insert(value.get()).second) return false;
    if (isa<IRBlockParam>(value)) return true;
    const auto* binary = dyn_cast<IRBinary>(value.get());
    return binary &&
           (ContainsBlockParameter(binary->GetLhs(), visited) ||
            ContainsBlockParameter(binary->GetRhs(), visited));
  }

  static bool HasPotentialAddressCandidate(const IRFunction& function) {
    for (const auto& block : function.blocks) {
      for (const auto& instruction : block->insts) {
        std::shared_ptr<IRValue> index;
        if (const auto* pointer = dyn_cast<IRGetElemPtr>(instruction.get())) {
          index = pointer->GetIndex();
        } else if (const auto* pointer =
                       dyn_cast<IRGetPtr>(instruction.get())) {
          index = pointer->GetIndex();
        }
        std::unordered_set<const IRValue*> visited;
        if (ContainsBlockParameter(index, visited)) return true;
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

  static std::optional<std::int64_t> GetIntegerConstant(
      const std::shared_ptr<IRValue>& value) {
    const auto* constant = dyn_cast<IRConstant>(value.get());
    if (!constant || constant->GetType() != IRType::IR_INT32)
      return std::nullopt;
    return std::get<std::int32_t>(constant->GetValue());
  }

  static std::optional<std::int64_t> CheckedAdd(std::int64_t lhs,
                                                 std::int64_t rhs) {
    if ((rhs > 0 && lhs > std::numeric_limits<std::int64_t>::max() - rhs) ||
        (rhs < 0 && lhs < std::numeric_limits<std::int64_t>::min() - rhs))
      return std::nullopt;
    return lhs + rhs;
  }

  static std::optional<std::int64_t> CheckedMultiply(std::int64_t lhs,
                                                      std::int64_t rhs) {
    if (lhs == 0 || rhs == 0) return 0;
    const auto minimum = std::numeric_limits<std::int64_t>::min();
    const auto maximum = std::numeric_limits<std::int64_t>::max();
    if (lhs > 0) {
      if ((rhs > 0 && lhs > maximum / rhs) ||
          (rhs < 0 && rhs < minimum / lhs))
        return std::nullopt;
    } else {
      if ((rhs > 0 && lhs < minimum / rhs) ||
          (rhs < 0 && lhs < maximum / rhs))
        return std::nullopt;
    }
    return lhs * rhs;
  }

  // 返回表达式中基本归纳变量的常量系数。nullopt 表示表达式不是
  // 可安全移到 preheader 的仿射表达式；循环外定义相对当前循环是常量。
  static std::optional<std::int64_t> AnalyzeAffineCoefficient(
      const std::shared_ptr<IRValue>& value,
      const BasicInductionVariable& induction, const CanonicalLoopInfo& loop,
      const std::unordered_map<const IRValue*, IRBasicBlock*>& definitions,
      std::unordered_set<const IRValue*>& visiting) {
    if (!value) return std::nullopt;
    if (value.get() == induction.parameter.get()) return 1;
    if (IsLoopInvariant(value, loop, definitions)) return 0;
    if (!visiting.insert(value.get()).second) return std::nullopt;

    const auto* binary = dyn_cast<IRBinary>(value.get());
    if (!binary || binary->GetOperandType() != IRType::IR_INT32) {
      visiting.erase(value.get());
      return std::nullopt;
    }
    const auto lhs = AnalyzeAffineCoefficient(
        binary->GetLhs(), induction, loop, definitions, visiting);
    const auto rhs = AnalyzeAffineCoefficient(
        binary->GetRhs(), induction, loop, definitions, visiting);
    visiting.erase(value.get());
    if (!lhs || !rhs) return std::nullopt;

    switch (binary->GetOp()) {
      case IRBinaryOp::ADD:
        return CheckedAdd(*lhs, *rhs);
      case IRBinaryOp::SUB:
        if (*rhs == std::numeric_limits<std::int64_t>::min())
          return std::nullopt;
        return CheckedAdd(*lhs, -*rhs);
      case IRBinaryOp::MUL: {
        // 完全不含当前 iv 的纯整数乘法仍是循环不变量。含 iv 时，另一侧
        // 必须是整数常量，避免把 iv * runtime_value 错当成固定步长。
        if (*lhs == 0 && *rhs == 0) return 0;
        if (*lhs != 0) {
          const auto constant = GetIntegerConstant(binary->GetRhs());
          return constant ? CheckedMultiply(*lhs, *constant) : std::nullopt;
        }
        const auto constant = GetIntegerConstant(binary->GetLhs());
        return constant ? CheckedMultiply(*rhs, *constant) : std::nullopt;
      }
      default:
        return std::nullopt;
    }
  }

  static bool IsPowerOfTwoMagnitude(std::int64_t value) {
    if (value == 0) return false;
    const std::uint64_t magnitude =
        value < 0 ? static_cast<std::uint64_t>(-(value + 1)) + 1
                  : static_cast<std::uint64_t>(value);
    return (magnitude & (magnitude - 1)) == 0;
  }

  static int BinaryRemovalCost(const IRBinary& binary) {
    switch (binary.GetOp()) {
      case IRBinaryOp::ADD:
      case IRBinaryOp::SUB:
        return 1;
      case IRBinaryOp::MUL: {
        auto constant = GetIntegerConstant(binary.GetLhs());
        if (!constant) constant = GetIntegerConstant(binary.GetRhs());
        // 2 的幂常量乘法通常会降为移位；其他乘法在目标机上更昂贵。
        return constant && IsPowerOfTwoMagnitude(*constant) ? 2 : 4;
      }
      default:
        return 0;
    }
  }

  // 只统计改写当前 GEP 后会真正成为死代码的、依赖 iv 的算术链。
  // use 次数大于 1 时，表达式仍被其他 GEP/指令使用，不能把它算作收益。
  static int EstimateRemovableCost(
      const std::shared_ptr<IRValue>& value,
      const BasicInductionVariable& induction, const CanonicalLoopInfo& loop,
      const std::unordered_map<const IRValue*, IRBasicBlock*>& definitions,
      const std::unordered_map<const IRValue*, std::size_t>& use_counts,
      std::unordered_set<const IRValue*>& visiting) {
    if (!value || value.get() == induction.parameter.get() ||
        IsLoopInvariant(value, loop, definitions))
      return 0;
    const auto use = use_counts.find(value.get());
    if (use == use_counts.end() || use->second != 1 ||
        !visiting.insert(value.get()).second)
      return 0;

    const auto* binary = dyn_cast<IRBinary>(value.get());
    if (!binary) {
      visiting.erase(value.get());
      return 0;
    }
    std::unordered_set<const IRValue*> coefficient_visiting;
    const auto coefficient = AnalyzeAffineCoefficient(
        value, induction, loop, definitions, coefficient_visiting);
    if (!coefficient || *coefficient == 0) {
      visiting.erase(value.get());
      return 0;
    }

    const int cost = BinaryRemovalCost(*binary) +
                     EstimateRemovableCost(binary->GetLhs(), induction, loop,
                                           definitions, use_counts, visiting) +
                     EstimateRemovableCost(binary->GetRhs(), induction, loop,
                                           definitions, use_counts, visiting);
    visiting.erase(value.get());
    return cost;
  }

  static int RegisterPressurePenalty(std::size_t header_parameters) {
    if (header_parameters <= 6) return 1;
    if (header_parameters <= 9) return 3;
    if (header_parameters <= 12) return 8;
    return 12;
  }

  static int DynamicAddressScaleCost(int element_size) {
    if (element_size <= 1) return 0;
    // 动态 GEP 原本需要 scale + add；递推地址只需常量步长 add。
    // 2 的幂比例对应移位，其他比例通常对应 li + mul。
    return IsPowerOfTwoMagnitude(element_size) ? 2 : 4;
  }

  static std::vector<AddressCandidate> FindCandidates(
      const CanonicalLoopInfo& loop, const BasicInductionVariable& induction,
      const std::unordered_map<const IRValue*, IRBasicBlock*>& definitions,
      const std::unordered_map<const IRValue*, std::size_t>& use_counts) {
    std::vector<AddressCandidate> candidates;
    for (IRBasicBlock* block : loop.blocks) {
      if (!block) continue;
      for (const auto& instruction : block->insts) {
        std::shared_ptr<IRValue> base;
        std::shared_ptr<IRValue> index;
        int element_size = 0;
        bool is_element_pointer = false;
        if (const auto* pointer = dyn_cast<IRGetElemPtr>(instruction.get())) {
          base = pointer->GetArr();
          index = pointer->GetIndex();
          element_size = pointer->GetElemSize();
          is_element_pointer = true;
        } else if (const auto* pointer =
                       dyn_cast<IRGetPtr>(instruction.get())) {
          base = pointer->GetPtr();
          index = pointer->GetIndex();
          element_size = pointer->GetElemSize();
        }
        if (!base || !index || element_size <= 0 ||
            !IsLoopInvariant(base, loop, definitions))
          continue;

        std::unordered_set<const IRValue*> visiting;
        const auto coefficient = AnalyzeAffineCoefficient(
            index, induction, loop, definitions, visiting);
        if (!coefficient || *coefficient == 0) continue;
        const auto address_step =
            CheckedMultiply(*coefficient, induction.step);
        if (!address_step ||
            *address_step < std::numeric_limits<std::int32_t>::min() ||
            *address_step > std::numeric_limits<std::int32_t>::max())
          continue;
        const bool is_direct = index.get() == induction.parameter.get();
        std::unordered_set<const IRValue*> cost_visiting;
        const int removable_cost =
            is_direct
                ? 0
                : DynamicAddressScaleCost(element_size) +
                      EstimateRemovableCost(index, induction, loop,
                                            definitions, use_counts,
                                            cost_visiting);
        candidates.push_back({instruction, base, index, *coefficient,
                              removable_cost, is_direct, element_size,
                              is_element_pointer});
      }
    }
    // 基本归纳地址保持原有优先级；派生地址按可删除算术收益排序。
    std::stable_sort(candidates.begin(), candidates.end(),
                     [](const AddressCandidate& lhs,
                        const AddressCandidate& rhs) {
                       if (lhs.is_direct_induction != rhs.is_direct_induction)
                         return lhs.is_direct_induction;
                       if (lhs.removable_cost != rhs.removable_cost)
                         return lhs.removable_cost > rhs.removable_cost;
                       return lhs.instruction->GetOperandName() <
                              rhs.instruction->GetOperandName();
                     });
    return candidates;
  }

  std::vector<AddressCandidate> SelectProfitableCandidates(
      const CanonicalLoopInfo& loop,
      const std::vector<AddressCandidate>& candidates,
      std::size_t function_parameter_pressure,
      std::size_t& affine_selected_in_function) {
    std::vector<AddressCandidate> selected;
    std::size_t current_header_parameters =
        loop.header ? loop.header->params.size() : 0;
    for (const auto& candidate : candidates) {
      if (selected.size() == kMaxAddressesPerInduction) break;
      if (!candidate.is_direct_induction) {
        ++stats_.affine_candidates;
        if (affine_selected_in_function >=
            kMaxAffineAddressesPerFunction) {
          ++stats_.rejected_by_cost;
          continue;
        }
        const std::size_t pressure_after_rewrite = std::max(
            function_parameter_pressure, current_header_parameters + 1);
        const int pressure_penalty =
            RegisterPressurePenalty(pressure_after_rewrite);
        // 相等时也拒绝：只有收益明确大于长存活指针成本才执行派生改写。
        if (candidate.removable_cost <= pressure_penalty) {
          ++stats_.rejected_by_cost;
          continue;
        }
      }
      selected.push_back(candidate);
      if (!candidate.is_direct_induction) ++affine_selected_in_function;
      ++current_header_parameters;
      function_parameter_pressure =
          std::max(function_parameter_pressure, current_header_parameters);
    }
    return selected;
  }

  static std::shared_ptr<IRValue> BuildInitialIndex(
      const std::shared_ptr<IRValue>& value,
      const BasicInductionVariable& induction, const CanonicalLoopInfo& loop,
      const std::unordered_map<const IRValue*, IRBasicBlock*>& definitions,
      IRBasicBlock& preheader, LoopValueNameGenerator& names,
      std::unordered_map<const IRValue*, std::shared_ptr<IRValue>>& memo) {
    if (!value) return nullptr;
    if (value.get() == induction.parameter.get()) return induction.initial;
    if (IsLoopInvariant(value, loop, definitions)) return value;
    const auto found = memo.find(value.get());
    if (found != memo.end()) return found->second;

    const auto* binary = dyn_cast<IRBinary>(value.get());
    if (!binary) return nullptr;
    auto lhs = BuildInitialIndex(binary->GetLhs(), induction, loop,
                                 definitions, preheader, names, memo);
    auto rhs = BuildInitialIndex(binary->GetRhs(), induction, loop,
                                 definitions, preheader, names, memo);
    if (!lhs || !rhs) return nullptr;
    auto clone = std::make_shared<IRBinary>(
        names.Next("%ivindex_init"), binary->GetOp(), std::move(lhs),
        std::move(rhs), binary->GetOperandType(), binary->GetType());
    InsertBeforeTerminator(preheader, clone);
    memo[value.get()] = clone;
    return clone;
  }

  static std::shared_ptr<IRInstruction> MakeInitialAddress(
      const AddressCandidate& candidate,
      const std::shared_ptr<IRValue>& initial_index,
      const std::string& name) {
    if (candidate.is_element_pointer)
      return std::make_shared<IRGetElemPtr>(
          name, candidate.base, initial_index, candidate.element_size);
    return std::make_shared<IRGetPtr>(name, candidate.base,
                                      initial_index,
                                      candidate.element_size);
  }

  bool RewriteCandidate(IRFunction& function, CanonicalLoopInfo& loop,
                        const BasicInductionVariable& induction,
                        const AddressCandidate& candidate,
                        const std::unordered_map<const IRValue*,
                                                 IRBasicBlock*>& definitions,
                        LoopValueNameGenerator& names) {
    if (!loop.preheader || loop.latches.size() != 1 ||
        !candidate.instruction)
      return false;
    IRBasicBlock* latch = *loop.latches.begin();
    if (!GetJumpEdgeArguments(*loop.preheader, loop.header) ||
        !GetJumpEdgeArguments(*latch, loop.header))
      return false;

    // 将仿射索引中的 iv 替换为初值，其余纯整数表达式只在 preheader
    // 计算一次。这里只构造指针，不访问内存，所以零次循环同样安全。
    std::unordered_map<const IRValue*, std::shared_ptr<IRValue>> memo;
    auto initial_index = BuildInitialIndex(
        candidate.index, induction, loop, definitions, *loop.preheader, names,
        memo);
    if (!initial_index) return false;
    auto initial_address = MakeInitialAddress(
        candidate, initial_index, names.Next("%ivaddr_init"));
    InsertBeforeTerminator(*loop.preheader, initial_address);

    auto pointer_parameter =
        loop.header->AddParam(names.Next("%ivaddr"), IRType::IR_POINTER);
    if (!AppendEdgeArgument(*loop.preheader, loop.header, initial_address))
      return false;

    const auto address_step = static_cast<std::int32_t>(
        candidate.induction_coefficient * induction.step);
    auto step = std::make_shared<IRConstant>(address_step);
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
    std::size_t affine_selected_in_function = 0;

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
        const auto use_counts = BuildUseCounts(function);
        const auto candidates = FindCandidates(loop, induction, definitions,
                                               use_counts);
        const auto selected = SelectProfitableCandidates(
            loop, candidates, MaxBlockParameterCount(function),
            affine_selected_in_function);
        for (const auto& candidate : selected)
          changed |= RewriteCandidate(function, loop, induction, candidate,
                                      definitions, names);
      }
    }
    return changed;
  }

  LoopStrengthReduceStats stats_;
};

}  // namespace Pass
}  // namespace IR
