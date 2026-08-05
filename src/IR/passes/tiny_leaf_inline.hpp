#pragma once

#include "../analysis/function_summary.hpp"
#include "../custom_ir.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace IR {
namespace Pass {

struct TinyLeafInlineStats {
  std::size_t examined_calls = 0;
  std::size_t eligible_calls = 0;
  std::size_t inlined_calls = 0;
  std::size_t cloned_instructions = 0;
};

// 该内联器有意保持保守：只处理单基本块、唯一 return、无递归、
// 无内存访问且无嵌套调用的小函数。
class TinyLeafInline {
 public:
  explicit TinyLeafInline(std::size_t instruction_limit = 12)
      : instruction_limit_(instruction_limit) {}

  bool Run(IRProgram* program) {
    stats_ = {};
    if (!program) return false;

    Analysis::FunctionSummaryAnalysis analysis;
    analysis.Run(*program);

    bool changed = false;
    for (const auto& caller : program->funcs) {
      if (!caller) continue;
      changed |= InlineInFunction(*caller, analysis);
    }
    return changed;
  }

  const TinyLeafInlineStats& GetStats() const { return stats_; }

 private:
  struct InlinePlan {
    std::vector<std::shared_ptr<IRInstruction>> cloned;
    std::shared_ptr<IRValue> return_value;
  };

  class NameGenerator {
   public:
    explicit NameGenerator(const IRFunction& function) {
      for (const auto& param : function.params) {
        if (param) used_.insert(param->GetOperandName());
      }
      for (const auto& block : function.blocks) {
        if (!block) continue;
        for (const auto& param : block->params) {
          if (param) used_.insert(param->GetOperandName());
        }
        for (const auto& inst : block->insts) {
          if (!inst) continue;
          if (isa<IRNameInstruction>(inst) ||
              (isa<IRCall>(inst) && cast<IRCall>(inst)->HasReturnValue())) {
            used_.insert(inst->GetOperandName());
          }
        }
      }
    }

    std::string Next() {
      while (true) {
        const std::string candidate = "%inl_" + std::to_string(next_++);
        if (used_.insert(candidate).second) return candidate;
      }
    }

   private:
    std::size_t next_ = 0;
    std::unordered_set<std::string> used_;
  };

  static bool IsCloneableInstruction(const IRInstruction& inst) {
    return isa<IRBinary>(&inst) || isa<IRCast>(&inst) ||
           isa<IRGetElemPtr>(&inst) || isa<IRGetPtr>(&inst);
  }

  bool IsEligible(const IRCall& call, const IRFunction& callee,
                  const Analysis::FunctionSummary& summary) const {
    if (summary.recursive || !summary.IsEffectFree()) return false;
    if (summary.block_count != 1 || callee.blocks.empty()) return false;
    if (callee.params.size() != call.GetArgs().size()) return false;

    const auto& block = callee.blocks.front();
    if (!block || !block->params.empty() || block->insts.empty()) return false;

    std::size_t return_count = 0;
    std::size_t cloneable_count = 0;
    for (std::size_t i = 0; i < block->insts.size(); ++i) {
      const auto& inst = block->insts[i];
      if (!inst || isa<IRComment>(inst)) continue;
      if (isa<IRReturn>(inst)) {
        ++return_count;
        if (i + 1 != block->insts.size()) return false;
        continue;
      }
      if (!IsCloneableInstruction(*inst)) return false;
      ++cloneable_count;
    }

    if (return_count != 1 || cloneable_count > instruction_limit_) {
      return false;
    }

    const auto* ret = dyn_cast<IRReturn>(block->insts.back().get());
    if (!ret) return false;
    if (call.HasReturnValue()) {
      return callee.GetRetType() != IRType::IR_VOID && ret->GetValue() &&
             call.GetType() == callee.GetRetType();
    }
    return callee.GetRetType() == IRType::IR_VOID && !ret->GetValue();
  }

  static std::shared_ptr<IRValue> RemapValue(
      const std::shared_ptr<IRValue>& value,
      const std::unordered_map<const IRValue*, std::shared_ptr<IRValue>>& map) {
    if (!value) return nullptr;
    const auto found = map.find(value.get());
    if (found != map.end()) return found->second;

    // 常量和全局对象不属于被调函数的局部 SSA 定义，可以直接共享。
    if (isa<IRConstant>(value) || isa<IRGlobalAlloc>(value) ||
        isa<IRSymbol>(value) || isa<IRFunction>(value) ||
        isa<IRFuncDecl>(value)) {
      return value;
    }
    return nullptr;
  }

  static std::shared_ptr<IRInstruction> CloneInstruction(
      const std::shared_ptr<IRInstruction>& inst,
      const std::unordered_map<const IRValue*, std::shared_ptr<IRValue>>& map,
      NameGenerator& names) {
    if (const auto binary = dynamic_cast_ptr<IRBinary>(inst)) {
      auto lhs = RemapValue(binary->GetLhs(), map);
      auto rhs = RemapValue(binary->GetRhs(), map);
      if (!lhs || !rhs) return nullptr;
      return std::make_shared<IRBinary>(names.Next(), binary->GetOp(), lhs,
                                        rhs, binary->GetOperandType(),
                                        binary->GetType());
    }
    if (const auto cast_inst = dynamic_cast_ptr<IRCast>(inst)) {
      auto value = RemapValue(cast_inst->GetValue(), map);
      if (!value) return nullptr;
      return std::make_shared<IRCast>(names.Next(), cast_inst->GetOp(), value,
                                      cast_inst->GetType());
    }
    if (const auto gep = dynamic_cast_ptr<IRGetElemPtr>(inst)) {
      auto base = RemapValue(gep->GetArr(), map);
      auto index = RemapValue(gep->GetIndex(), map);
      if (!base || !index) return nullptr;
      return std::make_shared<IRGetElemPtr>(names.Next(), base, index,
                                            gep->GetElemSize());
    }
    if (const auto getptr = dynamic_cast_ptr<IRGetPtr>(inst)) {
      auto base = RemapValue(getptr->GetPtr(), map);
      auto index = RemapValue(getptr->GetIndex(), map);
      if (!base || !index) return nullptr;
      return std::make_shared<IRGetPtr>(names.Next(), base, index,
                                        getptr->GetElemSize());
    }
    return nullptr;
  }

  static bool BuildPlan(const IRCall& call, const IRFunction& callee,
                        NameGenerator& names, InlinePlan& plan) {
    std::unordered_map<const IRValue*, std::shared_ptr<IRValue>> value_map;
    for (std::size_t i = 0; i < callee.params.size(); ++i) {
      value_map.emplace(callee.params[i].get(), call.GetArgs()[i]);
    }

    const auto& insts = callee.blocks.front()->insts;
    for (const auto& inst : insts) {
      if (!inst || isa<IRComment>(inst)) continue;
      if (const auto ret = dynamic_cast_ptr<IRReturn>(inst)) {
        plan.return_value = RemapValue(ret->GetValue(), value_map);
        return !call.HasReturnValue() || plan.return_value != nullptr;
      }

      auto cloned = CloneInstruction(inst, value_map, names);
      if (!cloned) return false;
      value_map.emplace(inst.get(), cloned);
      plan.cloned.push_back(std::move(cloned));
    }
    return false;
  }

  bool InlineInFunction(
      IRFunction& caller,
      const Analysis::FunctionSummaryAnalysis& analysis) {
    bool changed = false;
    NameGenerator names(caller);

    for (const auto& block : caller.blocks) {
      if (!block) continue;
      std::vector<std::shared_ptr<IRInstruction>> rewritten;
      rewritten.reserve(block->insts.size());

      for (const auto& inst : block->insts) {
        const auto call = dynamic_cast_ptr<IRCall>(inst);
        if (!call) {
          rewritten.push_back(inst);
          continue;
        }

        ++stats_.examined_calls;
        const IRFunction* callee = analysis.FindFunction(*call);
        const Analysis::FunctionSummary* summary = analysis.Get(callee);
        if (!callee || !summary || !IsEligible(*call, *callee, *summary)) {
          rewritten.push_back(inst);
          continue;
        }
        ++stats_.eligible_calls;

        InlinePlan plan;
        if (!BuildPlan(*call, *callee, names, plan)) {
          rewritten.push_back(inst);
          continue;
        }

        rewritten.insert(rewritten.end(), plan.cloned.begin(),
                         plan.cloned.end());
        if (call->HasReturnValue()) {
          caller.ReplaceAllUses(call.get(), plan.return_value);
        }
        stats_.cloned_instructions += plan.cloned.size();
        ++stats_.inlined_calls;
        changed = true;
      }
      block->insts = std::move(rewritten);
    }
    return changed;
  }

  std::size_t instruction_limit_;
  TinyLeafInlineStats stats_;
};

}  // namespace Pass
}  // namespace IR
