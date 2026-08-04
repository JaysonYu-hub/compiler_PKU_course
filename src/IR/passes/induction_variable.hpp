#pragma once

#include "loop_utils.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace IR {
namespace Pass {

// 基本归纳变量：header 参数从 preheader 接收 initial，并在唯一回边上以
// 固定 step 更新。当前只识别整数 add/sub 常量，便于首先保证正确性。
struct BasicInductionVariable {
  std::shared_ptr<IRBlockParam> parameter;
  std::shared_ptr<IRValue> initial;
  std::shared_ptr<IRValue> backedge_value;
  const IRBinary* update = nullptr;
  std::int32_t step = 0;
  std::size_t parameter_index = 0;
};

class InductionVariableAnalysis {
 public:
  std::vector<BasicInductionVariable> Analyze(
      const CanonicalLoopInfo& loop) const {
    std::vector<BasicInductionVariable> result;
    if (!loop.header || !loop.preheader || loop.latches.size() != 1)
      return result;
    IRBasicBlock* latch = *loop.latches.begin();
    const auto* incoming = GetJumpEdgeArguments(*loop.preheader, loop.header);
    const auto* backedge = GetJumpEdgeArguments(*latch, loop.header);
    if (!incoming || !backedge ||
        incoming->size() != loop.header->params.size() ||
        backedge->size() != loop.header->params.size())
      return result;

    for (std::size_t index = 0; index < loop.header->params.size(); ++index) {
      const auto& parameter = loop.header->params[index];
      if (!parameter || parameter->GetType() != IRType::IR_INT32) continue;
      const auto* update = dyn_cast<IRBinary>((*backedge)[index].get());
      if (!update || update->GetType() != IRType::IR_INT32) continue;
      const auto step = GetStep(*update, parameter.get());
      if (!step || *step == 0) continue;
      result.push_back({parameter, (*incoming)[index], (*backedge)[index],
                        update, *step, index});
    }
    return result;
  }

 private:
  static std::optional<std::int32_t> IntegerConstant(const IRValue* value) {
    const auto* constant = dyn_cast<IRConstant>(value);
    if (!constant || constant->GetType() != IRType::IR_INT32)
      return std::nullopt;
    return std::get<std::int32_t>(constant->GetValue());
  }

  static std::optional<std::int32_t> GetStep(const IRBinary& update,
                                             const IRValue* parameter) {
    const auto lhs = update.GetLhs();
    const auto rhs = update.GetRhs();
    if (update.GetOp() == IRBinaryOp::ADD) {
      if (lhs.get() == parameter) return IntegerConstant(rhs.get());
      if (rhs.get() == parameter) return IntegerConstant(lhs.get());
    }
    if (update.GetOp() == IRBinaryOp::SUB && lhs.get() == parameter) {
      const auto constant = IntegerConstant(rhs.get());
      if (!constant || *constant == INT32_MIN) return std::nullopt;
      return -*constant;
    }
    return std::nullopt;
  }
};

}  // namespace Pass
}  // namespace IR
