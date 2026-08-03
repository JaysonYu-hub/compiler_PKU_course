#pragma once

#include "../custom_ir.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace IR {
namespace Pass {

class ConstantFoldAndPropagation {
public:
  // 计算两个常量操作数的二元运算结果，返回折叠后的 IRConstant 指针
  static std::shared_ptr<IRConstant> EvaluateBinary(IRBinaryOp op,
                                                    const IRConstant* lhs,
                                                    const IRConstant* rhs) {
    if (!lhs || !rhs) return nullptr;

    bool is_float = (lhs->GetType() == IRType::IR_FLOAT || rhs->GetType() == IRType::IR_FLOAT);

    if (!is_float) {
      // 整型二元运算求值
      int32_t a = std::get<int32_t>(lhs->GetValue());
      int32_t b = std::get<int32_t>(rhs->GetValue());
      int32_t res = 0;
      const uint32_t ua = static_cast<uint32_t>(a);
      const uint32_t ub = static_cast<uint32_t>(b);
      const uint32_t shift = ub & 31U;

      switch (op) {
        case IRBinaryOp::ADD: res = static_cast<int32_t>(ua + ub); break;
        case IRBinaryOp::SUB: res = static_cast<int32_t>(ua - ub); break;
        case IRBinaryOp::MUL: res = static_cast<int32_t>(ua * ub); break;
        case IRBinaryOp::DIV:
          // 除零不能被优化成任意常量；INT_MIN / -1 按 RISC-V 结果处理。
          if (b == 0) return nullptr;
          res = (a == std::numeric_limits<int32_t>::min() && b == -1)
                  ? std::numeric_limits<int32_t>::min()
                  : a / b;
          break;
        case IRBinaryOp::MOD:
          if (b == 0) return nullptr;
          res = (a == std::numeric_limits<int32_t>::min() && b == -1)
                  ? 0
                  : a % b;
          break;
        case IRBinaryOp::AND: res = a & b; break;
        case IRBinaryOp::OR:  res = a | b; break;
        case IRBinaryOp::XOR: res = a ^ b; break;
        case IRBinaryOp::SHL: res = static_cast<int32_t>(ua << shift); break;
        case IRBinaryOp::SHR: res = static_cast<int32_t>(ua >> shift); break;
        case IRBinaryOp::SAR: res = a >> shift; break;
        case IRBinaryOp::EQ:  res = (a == b) ? 1 : 0; break;
        case IRBinaryOp::NE:  res = (a != b) ? 1 : 0; break;
        case IRBinaryOp::LT:  res = (a < b)  ? 1 : 0; break;
        case IRBinaryOp::LE:  res = (a <= b) ? 1 : 0; break;
        case IRBinaryOp::GT:  res = (a > b)  ? 1 : 0; break;
        case IRBinaryOp::GE:  res = (a >= b) ? 1 : 0; break;
        default: return nullptr;
      }
      return std::make_shared<IRConstant>(res);
    } else {
      // 浮点二元运算求值
      float a = (lhs->GetType() == IRType::IR_FLOAT) ? std::get<float>(lhs->GetValue()) : static_cast<float>(std::get<int32_t>(lhs->GetValue()));
      float b = (rhs->GetType() == IRType::IR_FLOAT) ? std::get<float>(rhs->GetValue()) : static_cast<float>(std::get<int32_t>(rhs->GetValue()));

      switch (op) {
        case IRBinaryOp::ADD: return std::make_shared<IRConstant>(a + b);
        case IRBinaryOp::SUB: return std::make_shared<IRConstant>(a - b);
        case IRBinaryOp::MUL: return std::make_shared<IRConstant>(a * b);
        // 暂不把浮点除零折成 inf/nan 文本，保留原指令交给目标机执行。
        case IRBinaryOp::DIV:
          return b != 0.0f ? std::make_shared<IRConstant>(a / b) : nullptr;
        case IRBinaryOp::EQ:  return std::make_shared<IRConstant>(static_cast<int32_t>(a == b ? 1 : 0));
        case IRBinaryOp::NE:  return std::make_shared<IRConstant>(static_cast<int32_t>(a != b ? 1 : 0));
        case IRBinaryOp::LT:  return std::make_shared<IRConstant>(static_cast<int32_t>(a < b ? 1 : 0));
        case IRBinaryOp::LE:  return std::make_shared<IRConstant>(static_cast<int32_t>(a <= b ? 1 : 0));
        case IRBinaryOp::GT:  return std::make_shared<IRConstant>(static_cast<int32_t>(a > b ? 1 : 0));
        case IRBinaryOp::GE:  return std::make_shared<IRConstant>(static_cast<int32_t>(a >= b ? 1 : 0));
        default: return nullptr;
      }
    }
  }

  // 对单条函数进行常量折叠与传播
  void ConstantFold(IRFunction& function) {
    bool changed = true;
    while (changed) {
      changed = false;
      std::unordered_map<const IRValue*, std::shared_ptr<IRConstant>> const_map;

      for (auto& block : function.blocks) {
        std::vector<std::shared_ptr<IRInstruction>> new_insts;
        new_insts.reserve(block->insts.size());

        for (auto& inst : block->insts) {
          // 先传播已知的常量操作数
          for (const auto& use : inst->GetUses()) {
            if (use && const_map.count(use.get())) {
              inst->ReplaceUse(use.get(), const_map[use.get()]);
              changed = true;
            }
          }

          // 尝试折叠二元运算指令
          if (isa<IRBinary>(inst)) {
            auto binary = cast<IRBinary>(inst);
            auto lhs_val = binary->GetLhs();
            auto rhs_val = binary->GetRhs();

            auto lhs_const = isa<IRConstant>(lhs_val) ? cast<IRConstant>(lhs_val).get() : (const_map.count(lhs_val.get()) ? const_map[lhs_val.get()].get() : nullptr);
            auto rhs_const = isa<IRConstant>(rhs_val) ? cast<IRConstant>(rhs_val).get() : (const_map.count(rhs_val.get()) ? const_map[rhs_val.get()].get() : nullptr);

            if (lhs_const && rhs_const) {
              auto folded = EvaluateBinary(binary->GetOp(), lhs_const, rhs_const);
              if (folded) {
                const_map[binary.get()] = folded;
                function.ReplaceAllUses(binary.get(), folded);
                changed = true;
                continue; // 成功折叠，物理剔除该二进制指令
              }
            }
          }
          // 尝试折叠类型转换指令
          else if (isa<IRCast>(inst)) {
            auto cast_inst = cast<IRCast>(inst);
            auto val = cast_inst->GetValue();
            auto val_const = isa<IRConstant>(val) ? cast<IRConstant>(val).get() : (const_map.count(val.get()) ? const_map[val.get()].get() : nullptr);

            if (val_const) {
              std::shared_ptr<IRConstant> folded = nullptr;
              if (cast_inst->GetOp() == IRCastOp::INT_TO_FLOAT) {
                int32_t iv = std::get<int32_t>(val_const->GetValue());
                folded = std::make_shared<IRConstant>(static_cast<float>(iv));
              } else if (cast_inst->GetOp() == IRCastOp::FLOAT_TO_INT) {
                float fv = std::get<float>(val_const->GetValue());
                const double wide = static_cast<double>(fv);
                // 浮点转整数越界在宿主 C++ 中没有可移植结果，不能折叠。
                if (std::isfinite(fv) &&
                    wide >= static_cast<double>(std::numeric_limits<int32_t>::min()) &&
                    wide <= static_cast<double>(std::numeric_limits<int32_t>::max())) {
                  folded = std::make_shared<IRConstant>(static_cast<int32_t>(fv));
                }
              }

              if (folded) {
                const_map[cast_inst.get()] = folded;
                function.ReplaceAllUses(cast_inst.get(), folded);
                changed = true;
                continue; // 成功折叠，剔除类型转换指令
              }
            }
          }

          new_insts.push_back(inst);
        }

        block->insts = std::move(new_insts);
      }
    }
  }

  void Run(IRProgram *program) {
    if (!program) return;
    for (auto &func : program->funcs) {
      if (func) {
        ConstantFold(*func);
      }
    }
  }
};

}
}
