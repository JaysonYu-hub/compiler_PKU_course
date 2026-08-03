#pragma once

#include "../custom_ir.hpp"

#include <string>
#include <unordered_set>
#include <utility>

namespace IR {
namespace Pass {

// 轻量结构校验器。它不代替完整的 SSA 支配性验证，当前主要检查内联后最容易
// 出错的三类问题：悬空操作数、非法跳转目标和块参数实参数量不一致。
class IRVerifier {
 public:
  bool Verify(const IRProgram& program, std::string* error = nullptr) const {
    for (const auto& function : program.funcs) {
      if (!function) return Fail(error, "program contains a null function");
      if (!VerifyFunction(*function, error)) return false;
    }
    return true;
  }

 private:
  static bool IsTerminator(const IRInstruction* inst) {
    return isa<IRReturn>(inst) || isa<IRJump>(inst) || isa<IRBranch>(inst);
  }

  static bool Fail(std::string* error, std::string message) {
    if (error) *error = std::move(message);
    return false;
  }

  static bool VerifyFunction(const IRFunction& function, std::string* error) {
    if (function.blocks.empty()) {
      return Fail(error, "function @" + function.name + " has no blocks");
    }

    std::unordered_set<const IRBasicBlock*> blocks;
    std::unordered_set<const IRValue*> local_values;
    for (const auto& param : function.params) {
      if (param) local_values.insert(param.get());
    }
    for (const auto& block : function.blocks) {
      if (!block) return Fail(error, "function @" + function.name +
                                      " contains a null block");
      blocks.insert(block.get());
      for (const auto& param : block->params) {
        if (param) local_values.insert(param.get());
      }
      for (const auto& inst : block->insts) {
        if (inst) local_values.insert(inst.get());
      }
    }

    for (const auto& block : function.blocks) {
      if (block->insts.empty() ||
          !IsTerminator(block->insts.back().get())) {
        return Fail(error, "block " + block->name + " in @" +
                               function.name + " has no terminator");
      }

      for (std::size_t i = 0; i < block->insts.size(); ++i) {
        const auto& inst = block->insts[i];
        if (!inst) return Fail(error, "block " + block->name +
                                      " contains a null instruction");
        if (IsTerminator(inst.get()) && i + 1 != block->insts.size()) {
          return Fail(error, "block " + block->name +
                                 " has an instruction after its terminator");
        }

        for (const auto& use : inst->GetUses()) {
          if (!use) return Fail(error, "instruction in " + block->name +
                                        " contains a null operand");
          if ((isa<IRInstruction>(use) || isa<IRFuncParam>(use) ||
               isa<IRBlockParam>(use)) &&
              !local_values.count(use.get())) {
            return Fail(error, "instruction in " + block->name +
                                   " uses a value outside @" + function.name);
          }
        }

        if (const auto* jump = dyn_cast<IRJump>(inst.get())) {
          const auto* target = dyn_cast<IRBasicBlock>(jump->GetTarget().get());
          if (!target || !blocks.count(target)) {
            return Fail(error, "jump in " + block->name +
                                   " targets a foreign block");
          }
          if (jump->GetArgs().size() != target->params.size()) {
            return Fail(error, "jump argument count mismatch for " +
                                   target->name);
          }
        } else if (const auto* branch = dyn_cast<IRBranch>(inst.get())) {
          const auto* then_target =
              dyn_cast<IRBasicBlock>(branch->GetThenTarget().get());
          const auto* else_target =
              dyn_cast<IRBasicBlock>(branch->GetElseTarget().get());
          if (!then_target || !else_target || !blocks.count(then_target) ||
              !blocks.count(else_target)) {
            return Fail(error, "branch in " + block->name +
                                   " targets a foreign block");
          }
          if (branch->GetThenArgs().size() != then_target->params.size() ||
              branch->GetElseArgs().size() != else_target->params.size()) {
            return Fail(error, "branch argument count mismatch in " +
                                   block->name);
          }
        }
      }
    }
    return true;
  }
};

}  // namespace Pass
}  // namespace IR
