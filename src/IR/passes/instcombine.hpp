#pragma once

#include "../custom_ir.hpp"
#include "cfandcp.hpp"
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

namespace IR {
namespace Pass {

// 局部指令合并（InstCombine）。
//
// 该 Pass 只采用与控制流和内存无关的恒等式，主要职责是把复杂表达式
// 变成更简单的 SSA 值，并为后续 GVN、DCE 创造机会。
class InstCombine {
private:
  static bool GetIntConstant(const std::shared_ptr<IRValue>& value,
                             int32_t& result) {
    if (!isa<IRConstant>(value) || value->GetType() != IRType::IR_INT32) {
      return false;
    }
    result = std::get<int32_t>(cast<IRConstant>(value)->GetValue());
    return true;
  }

  static std::shared_ptr<IRValue> SimplifyBinary(
      const std::shared_ptr<IRBinary>& binary) {
    const auto lhs = binary->GetLhs();
    const auto rhs = binary->GetRhs();

    // 两个常量直接求值。除零等不能安全折叠的情况会返回 nullptr。
    if (isa<IRConstant>(lhs) && isa<IRConstant>(rhs)) {
      auto folded = ConstantFoldAndPropagation::EvaluateBinary(
          binary->GetOp(), cast<IRConstant>(lhs).get(),
          cast<IRConstant>(rhs).get());
      if (folded) return folded;
    }

    // 浮点数需要保留 NaN、正负零等 IEEE 语义，第一阶段不使用整数式
    // 代数恒等式处理浮点表达式。
    if (binary->GetOperandType() != IRType::IR_INT32) return nullptr;

    int32_t lhs_const = 0;
    int32_t rhs_const = 0;
    const bool lhs_is_const = GetIntConstant(lhs, lhs_const);
    const bool rhs_is_const = GetIntConstant(rhs, rhs_const);
    const bool same_operand = lhs.get() == rhs.get();

    switch (binary->GetOp()) {
      case IRBinaryOp::ADD:
        if (rhs_is_const && rhs_const == 0) return lhs;
        if (lhs_is_const && lhs_const == 0) return rhs;
        break;
      case IRBinaryOp::SUB:
        if (rhs_is_const && rhs_const == 0) return lhs;
        if (same_operand) return std::make_shared<IRConstant>(int32_t(0));
        break;
      case IRBinaryOp::MUL: {
        if ((lhs_is_const && lhs_const == 0) ||
            (rhs_is_const && rhs_const == 0)) {
          return std::make_shared<IRConstant>(int32_t(0));
        }
        if (rhs_is_const && rhs_const == 1) return lhs;
        if (lhs_is_const && lhs_const == 1) return rhs;

        // 乘 2 次幂的目标指令选择已由 RISC-V 后端处理。IR 层保留 MUL，
        // 避免生成当前后端尚未通用支持的 SHL 节点。
        break;
      }
      case IRBinaryOp::DIV:
        if (rhs_is_const && rhs_const == 1) return lhs;
        break;
      case IRBinaryOp::MOD:
        if (rhs_is_const && (rhs_const == 1 || rhs_const == -1)) {
          return std::make_shared<IRConstant>(int32_t(0));
        }
        break;
      case IRBinaryOp::AND:
        if ((lhs_is_const && lhs_const == 0) ||
            (rhs_is_const && rhs_const == 0)) {
          return std::make_shared<IRConstant>(int32_t(0));
        }
        if (rhs_is_const && rhs_const == -1) return lhs;
        if (lhs_is_const && lhs_const == -1) return rhs;
        if (same_operand) return lhs;
        break;
      case IRBinaryOp::OR:
        if (rhs_is_const && rhs_const == 0) return lhs;
        if (lhs_is_const && lhs_const == 0) return rhs;
        if (same_operand) return lhs;
        break;
      case IRBinaryOp::XOR:
        if (rhs_is_const && rhs_const == 0) return lhs;
        if (lhs_is_const && lhs_const == 0) return rhs;
        if (same_operand) return std::make_shared<IRConstant>(int32_t(0));
        break;
      case IRBinaryOp::SHL:
      case IRBinaryOp::SHR:
      case IRBinaryOp::SAR:
        if (rhs_is_const && (static_cast<uint32_t>(rhs_const) & 31U) == 0) {
          return lhs;
        }
        break;
      case IRBinaryOp::EQ:
      case IRBinaryOp::LE:
      case IRBinaryOp::GE:
        if (same_operand) return std::make_shared<IRConstant>(int32_t(1));
        break;
      case IRBinaryOp::NE:
      case IRBinaryOp::LT:
      case IRBinaryOp::GT:
        if (same_operand) return std::make_shared<IRConstant>(int32_t(0));
        break;
    }
    return nullptr;
  }

  static std::shared_ptr<IRValue> SimplifyCast(
      const std::shared_ptr<IRCast>& cast_inst) {
    const auto value = cast_inst->GetValue();
    if (!isa<IRConstant>(value)) return nullptr;

    const auto constant = cast<IRConstant>(value);
    if (cast_inst->GetOp() == IRCastOp::INT_TO_FLOAT &&
        constant->GetType() == IRType::IR_INT32) {
      return std::make_shared<IRConstant>(
          static_cast<float>(std::get<int32_t>(constant->GetValue())));
    }
    if (cast_inst->GetOp() == IRCastOp::FLOAT_TO_INT &&
        constant->GetType() == IRType::IR_FLOAT) {
      const float value = std::get<float>(constant->GetValue());
      const double wide = static_cast<double>(value);
      if (!std::isfinite(value) ||
          wide < static_cast<double>(std::numeric_limits<int32_t>::min()) ||
          wide > static_cast<double>(std::numeric_limits<int32_t>::max())) {
        return nullptr;
      }
      return std::make_shared<IRConstant>(static_cast<int32_t>(value));
    }
    return nullptr;
  }

public:
  bool OptimizeFunction(IRFunction& function) {
    bool any_changed = false;
    bool changed = true;
    while (changed) {
      changed = false;
      for (const auto& block : function.blocks) {
        std::vector<std::shared_ptr<IRInstruction>> new_insts;
        new_insts.reserve(block->insts.size());

        for (const auto& inst : block->insts) {
          std::shared_ptr<IRValue> replacement;
          if (isa<IRBinary>(inst)) {
            replacement = SimplifyBinary(cast<IRBinary>(inst));
          } else if (isa<IRCast>(inst)) {
            replacement = SimplifyCast(cast<IRCast>(inst));
          }

          if (replacement && replacement.get() != inst.get()) {
            function.ReplaceAllUses(inst.get(), replacement);
            // replacement 通常是当前指令已有的操作数（例如 x * 1 -> x）。
            // 它已经属于原来的基本块，绝不能在当前位置再次插入，否则会
            // 形成两个同名 SSA 定义并破坏后端活跃区间。
            changed = true;
            any_changed = true;
            continue;
          }
          new_insts.push_back(inst);
        }
        block->insts = std::move(new_insts);
      }
    }
    return any_changed;
  }

  bool Run(IRProgram* program) {
    if (!program) return false;
    bool changed = false;
    for (auto& function : program->funcs) {
      if (function) changed |= OptimizeFunction(*function);
    }
    return changed;
  }
};

}  // namespace Pass
}  // namespace IR
