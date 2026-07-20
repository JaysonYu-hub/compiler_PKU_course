#include "ir_pass.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace IR {
namespace {

bool IsComparison(IRBinaryOp op) {
  return op == IRBinaryOp::EQ || op == IRBinaryOp::NE ||
         op == IRBinaryOp::LT || op == IRBinaryOp::LE ||
         op == IRBinaryOp::GT || op == IRBinaryOp::GE;
}

bool IsTerminator(IRInstructionType type) {
  return type == IRInstructionType::IR_INST_RET ||
         type == IRInstructionType::IR_INST_BR ||
         type == IRInstructionType::IR_INST_JUMP;
}

bool ProducesValue(const std::shared_ptr<IRInstruction>& inst) {
  if (isa<IRNameInstruction>(inst)) return true;
  const auto call = dynamic_cast_ptr<IRCall>(inst);
  return call && call->HasReturnValue();
}

int32_t BitsToInt32(uint32_t bits) {
  int32_t value = 0;
  std::memcpy(&value, &bits, sizeof(value));
  return value;
}

std::shared_ptr<IRConstant> AsConstant(
    const std::shared_ptr<IRValue>& value) {
  return std::dynamic_pointer_cast<IRConstant>(value);
}

bool GetIntConstant(const std::shared_ptr<IRValue>& value, int32_t& result) {
  const auto constant = AsConstant(value);
  if (!constant || constant->GetType() != IRType::IR_INT32) return false;
  result = std::get<int32_t>(constant->GetValue());
  return true;
}

std::shared_ptr<IRConstant> FoldIntegerBinary(const IRBinary& binary,
                                             int32_t lhs, int32_t rhs) {
  const uint32_t ulhs = static_cast<uint32_t>(lhs);
  const uint32_t urhs = static_cast<uint32_t>(rhs);
  const unsigned shift = urhs & 31U;

  switch (binary.GetOp()) {
    case IRBinaryOp::ADD:
      return std::make_shared<IRConstant>(BitsToInt32(ulhs + urhs));
    case IRBinaryOp::SUB:
      return std::make_shared<IRConstant>(BitsToInt32(ulhs - urhs));
    case IRBinaryOp::MUL:
      return std::make_shared<IRConstant>(BitsToInt32(ulhs * urhs));
    case IRBinaryOp::DIV:
      if (rhs == 0 ||
          (lhs == std::numeric_limits<int32_t>::min() && rhs == -1)) {
        return nullptr;
      }
      return std::make_shared<IRConstant>(lhs / rhs);
    case IRBinaryOp::MOD:
      if (rhs == 0 ||
          (lhs == std::numeric_limits<int32_t>::min() && rhs == -1)) {
        return nullptr;
      }
      return std::make_shared<IRConstant>(lhs % rhs);
    case IRBinaryOp::AND:
      return std::make_shared<IRConstant>(BitsToInt32(ulhs & urhs));
    case IRBinaryOp::OR:
      return std::make_shared<IRConstant>(BitsToInt32(ulhs | urhs));
    case IRBinaryOp::XOR:
      return std::make_shared<IRConstant>(BitsToInt32(ulhs ^ urhs));
    case IRBinaryOp::SHL:
      return std::make_shared<IRConstant>(BitsToInt32(ulhs << shift));
    case IRBinaryOp::SHR:
      return std::make_shared<IRConstant>(BitsToInt32(ulhs >> shift));
    case IRBinaryOp::SAR: {
      uint32_t result = ulhs >> shift;
      if (lhs < 0 && shift != 0) result |= (~uint32_t{0}) << (32 - shift);
      return std::make_shared<IRConstant>(BitsToInt32(result));
    }
    case IRBinaryOp::EQ:
      return std::make_shared<IRConstant>(static_cast<int32_t>(lhs == rhs));
    case IRBinaryOp::NE:
      return std::make_shared<IRConstant>(static_cast<int32_t>(lhs != rhs));
    case IRBinaryOp::LT:
      return std::make_shared<IRConstant>(static_cast<int32_t>(lhs < rhs));
    case IRBinaryOp::LE:
      return std::make_shared<IRConstant>(static_cast<int32_t>(lhs <= rhs));
    case IRBinaryOp::GT:
      return std::make_shared<IRConstant>(static_cast<int32_t>(lhs > rhs));
    case IRBinaryOp::GE:
      return std::make_shared<IRConstant>(static_cast<int32_t>(lhs >= rhs));
  }
  return nullptr;
}

std::shared_ptr<IRConstant> FoldFloatBinary(const IRBinary& binary, float lhs,
                                           float rhs) {
  switch (binary.GetOp()) {
    case IRBinaryOp::ADD:
      return std::make_shared<IRConstant>(lhs + rhs);
    case IRBinaryOp::SUB:
      return std::make_shared<IRConstant>(lhs - rhs);
    case IRBinaryOp::MUL:
      return std::make_shared<IRConstant>(lhs * rhs);
    case IRBinaryOp::DIV:
      return std::make_shared<IRConstant>(lhs / rhs);
    case IRBinaryOp::EQ:
      return std::make_shared<IRConstant>(static_cast<int32_t>(lhs == rhs));
    case IRBinaryOp::NE:
      return std::make_shared<IRConstant>(static_cast<int32_t>(lhs != rhs));
    case IRBinaryOp::LT:
      return std::make_shared<IRConstant>(static_cast<int32_t>(lhs < rhs));
    case IRBinaryOp::LE:
      return std::make_shared<IRConstant>(static_cast<int32_t>(lhs <= rhs));
    case IRBinaryOp::GT:
      return std::make_shared<IRConstant>(static_cast<int32_t>(lhs > rhs));
    case IRBinaryOp::GE:
      return std::make_shared<IRConstant>(static_cast<int32_t>(lhs >= rhs));
    default:
      return nullptr;
  }
}

std::shared_ptr<IRConstant> FoldBinary(const IRBinary& binary) {
  const auto lhs = AsConstant(binary.GetLhs());
  const auto rhs = AsConstant(binary.GetRhs());
  if (!lhs || !rhs || lhs->GetType() != binary.GetOperandType() ||
      rhs->GetType() != binary.GetOperandType()) {
    return nullptr;
  }

  if (binary.GetOperandType() == IRType::IR_INT32) {
    return FoldIntegerBinary(binary, std::get<int32_t>(lhs->GetValue()),
                             std::get<int32_t>(rhs->GetValue()));
  }
  if (binary.GetOperandType() == IRType::IR_FLOAT) {
    return FoldFloatBinary(binary, std::get<float>(lhs->GetValue()),
                           std::get<float>(rhs->GetValue()));
  }
  return nullptr;
}

std::shared_ptr<IRConstant> FoldCast(const IRCast& cast) {
  const auto value = AsConstant(cast.GetValue());
  if (!value) return nullptr;

  if (cast.GetOp() == IRCastOp::INT_TO_FLOAT &&
      value->GetType() == IRType::IR_INT32) {
    return std::make_shared<IRConstant>(
        static_cast<float>(std::get<int32_t>(value->GetValue())));
  }
  if (cast.GetOp() == IRCastOp::FLOAT_TO_INT &&
      value->GetType() == IRType::IR_FLOAT) {
    const float raw = std::get<float>(value->GetValue());
    const double numeric = static_cast<double>(raw);
    if (!std::isfinite(raw) ||
        numeric < static_cast<double>(std::numeric_limits<int32_t>::min()) ||
        numeric > static_cast<double>(std::numeric_limits<int32_t>::max())) {
      return nullptr;
    }
    return std::make_shared<IRConstant>(static_cast<int32_t>(raw));
  }
  return nullptr;
}

std::shared_ptr<IRValue> SimplifyIntegerBinary(const IRBinary& binary) {
  if (binary.GetOperandType() != IRType::IR_INT32) return nullptr;

  const auto lhs = binary.GetLhs();
  const auto rhs = binary.GetRhs();
  int32_t lhs_constant = 0;
  int32_t rhs_constant = 0;
  const bool lhs_is_constant = GetIntConstant(lhs, lhs_constant);
  const bool rhs_is_constant = GetIntConstant(rhs, rhs_constant);

  if (lhs.get() == rhs.get()) {
    switch (binary.GetOp()) {
      case IRBinaryOp::SUB:
      case IRBinaryOp::XOR:
        return std::make_shared<IRConstant>(int32_t{0});
      case IRBinaryOp::EQ:
      case IRBinaryOp::LE:
      case IRBinaryOp::GE:
        return std::make_shared<IRConstant>(int32_t{1});
      case IRBinaryOp::NE:
      case IRBinaryOp::LT:
      case IRBinaryOp::GT:
        return std::make_shared<IRConstant>(int32_t{0});
      default:
        break;
    }
  }

  switch (binary.GetOp()) {
    case IRBinaryOp::ADD:
      if (rhs_is_constant && rhs_constant == 0) return lhs;
      if (lhs_is_constant && lhs_constant == 0) return rhs;
      break;
    case IRBinaryOp::SUB:
      if (rhs_is_constant && rhs_constant == 0) return lhs;
      break;
    case IRBinaryOp::MUL:
      if ((lhs_is_constant && lhs_constant == 0) ||
          (rhs_is_constant && rhs_constant == 0)) {
        return std::make_shared<IRConstant>(int32_t{0});
      }
      if (rhs_is_constant && rhs_constant == 1) return lhs;
      if (lhs_is_constant && lhs_constant == 1) return rhs;
      break;
    case IRBinaryOp::DIV:
      if (rhs_is_constant && rhs_constant == 1) return lhs;
      break;
    case IRBinaryOp::MOD:
      if (rhs_is_constant && (rhs_constant == 1 || rhs_constant == -1)) {
        return std::make_shared<IRConstant>(int32_t{0});
      }
      break;
    case IRBinaryOp::AND:
      if ((lhs_is_constant && lhs_constant == 0) ||
          (rhs_is_constant && rhs_constant == 0)) {
        return std::make_shared<IRConstant>(int32_t{0});
      }
      if (rhs_is_constant && rhs_constant == -1) return lhs;
      if (lhs_is_constant && lhs_constant == -1) return rhs;
      break;
    case IRBinaryOp::OR:
    case IRBinaryOp::XOR:
      if (rhs_is_constant && rhs_constant == 0) return lhs;
      if (lhs_is_constant && lhs_constant == 0) return rhs;
      break;
    case IRBinaryOp::SHL:
    case IRBinaryOp::SHR:
    case IRBinaryOp::SAR:
      if (rhs_is_constant && (static_cast<uint32_t>(rhs_constant) & 31U) == 0)
        return lhs;
      break;
    default:
      break;
  }
  return nullptr;
}

bool FoldConstants(IRProgram& program, IROptimizationStats& stats) {
  bool changed = false;
  for (auto& function : program.funcs) {
    for (auto& block : function->blocks) {
      for (auto it = block->insts.begin(); it != block->insts.end();) {
        std::shared_ptr<IRConstant> replacement;
        if (const auto binary = dynamic_cast_ptr<IRBinary>(*it)) {
          replacement = FoldBinary(*binary);
        } else if (const auto cast = dynamic_cast_ptr<IRCast>(*it)) {
          replacement = FoldCast(*cast);
        }

        if (!replacement) {
          ++it;
          continue;
        }
        ReplaceAllUses(program, it->get(), replacement);
        it = block->insts.erase(it);
        ++stats.constants_folded;
        changed = true;
      }
    }
  }
  return changed;
}

bool SimplifyAlgebra(IRProgram& program, IROptimizationStats& stats) {
  bool changed = false;
  for (auto& function : program.funcs) {
    for (auto& block : function->blocks) {
      for (auto it = block->insts.begin(); it != block->insts.end();) {
        const auto binary = dynamic_cast_ptr<IRBinary>(*it);
        const auto replacement =
            binary ? SimplifyIntegerBinary(*binary) : nullptr;
        if (!replacement) {
          ++it;
          continue;
        }
        ReplaceAllUses(program, it->get(), replacement);
        it = block->insts.erase(it);
        ++stats.algebraic_simplifications;
        changed = true;
      }
    }
  }
  return changed;
}

bool EliminateDeadCode(IRProgram& program, IROptimizationStats& stats) {
  bool any_changed = false;
  bool changed = true;
  while (changed) {
    changed = false;
    IRUseDefAnalysis analysis;
    analysis.Rebuild(program);
    for (auto& function : program.funcs) {
      for (auto& block : function->blocks) {
        for (auto it = block->insts.begin(); it != block->insts.end();) {
          const auto& inst = *it;
          if (ProducesValue(inst) && !inst->HasSideEffect() &&
              analysis.GetUseCount(inst.get()) == 0) {
            it = block->insts.erase(it);
            ++stats.dead_instructions_removed;
            changed = true;
            any_changed = true;
          } else {
            ++it;
          }
        }
      }
    }
  }
  return any_changed;
}

bool ConstantIsTrue(const IRConstant& constant) {
  if (constant.GetType() == IRType::IR_INT32) {
    return std::get<int32_t>(constant.GetValue()) != 0;
  }
  if (constant.GetType() == IRType::IR_FLOAT) {
    return std::get<float>(constant.GetValue()) != 0.0f;
  }
  return false;
}

bool SimplifyCFG(IRProgram& program, IROptimizationStats& stats) {
  bool changed = false;
  for (auto& function : program.funcs) {
    for (auto& block : function->blocks) {
      if (block->insts.empty()) continue;
      const auto branch = dynamic_cast_ptr<IRBranch>(block->insts.back());
      if (!branch) continue;
      const auto condition = AsConstant(branch->GetCond());
      if (!condition) continue;

      const auto selected_value = ConstantIsTrue(*condition)
                                      ? branch->GetThenTarget()
                                      : branch->GetElseTarget();
      const auto selected =
          std::dynamic_pointer_cast<IRBasicBlock>(selected_value);
      if (!selected) continue;
      block->insts.back() = std::make_shared<IRJump>(selected);
      block->next_1 = selected;
      block->next_2.reset();
      ++stats.constant_branches_simplified;
      changed = true;
    }

    if (function->blocks.empty()) continue;
    std::unordered_set<const IRBasicBlock*> reachable;
    std::queue<std::shared_ptr<IRBasicBlock>> worklist;
    worklist.push(function->blocks.front());
    while (!worklist.empty()) {
      auto block = worklist.front();
      worklist.pop();
      if (!block || !reachable.insert(block.get()).second) continue;
      if (block->next_1) worklist.push(block->next_1);
      if (block->next_2) worklist.push(block->next_2);
    }

    const std::size_t old_size = function->blocks.size();
    function->blocks.erase(
        std::remove_if(function->blocks.begin(), function->blocks.end(),
                       [&](const std::shared_ptr<IRBasicBlock>& block) {
                         return !block || reachable.count(block.get()) == 0;
                       }),
        function->blocks.end());
    const std::size_t removed = old_size - function->blocks.size();
    if (removed != 0) {
      stats.unreachable_blocks_removed += removed;
      changed = true;
    }
  }
  return changed;
}

bool TagMatchesDynamicType(const IRInstruction& inst) {
  switch (inst.inst_type) {
    case IRInstructionType::IR_INST_BINARY:
      return isa<IRBinary>(&inst);
    case IRInstructionType::IR_INST_CAST:
      return isa<IRCast>(&inst);
    case IRInstructionType::IR_INST_ALLOC:
      return isa<IRAlloc>(&inst);
    case IRInstructionType::IR_INST_STORE:
      return isa<IRStore>(&inst);
    case IRInstructionType::IR_INST_ZERO_INIT:
      return isa<IRZeroInit>(&inst);
    case IRInstructionType::IR_INST_LOAD:
      return isa<IRLoad>(&inst);
    case IRInstructionType::IR_INST_RET:
      return isa<IRReturn>(&inst);
    case IRInstructionType::IR_INST_BR:
      return isa<IRBranch>(&inst);
    case IRInstructionType::IR_INST_CALL:
      return isa<IRCall>(&inst);
    case IRInstructionType::IR_INST_GETPTR:
      return isa<IRGetPtr>(&inst);
    case IRInstructionType::IR_INST_GETELEMPTR:
      return isa<IRGetElemPtr>(&inst);
    case IRInstructionType::IR_INST_JUMP:
      return isa<IRJump>(&inst);
    case IRInstructionType::IR_INST_GLOBAL_ALLOC:
    case IRInstructionType::IR_INST_UNKNOWN:
      return false;
  }
  return false;
}

std::string Location(const IRFunction& function, const IRBasicBlock& block) {
  return "function @" + function.name + ", block " + block.name + ": ";
}

}  // namespace

void IRUseDefAnalysis::Rebuild(const IRProgram& program) {
  users_.clear();
  for (const auto& function : program.funcs) {
    for (const auto& block : function->blocks) {
      for (const auto& inst : block->insts) {
        if (!inst) continue;
        for (const auto& use : inst->GetUses()) {
          if (use) users_[use.get()].push_back(inst.get());
        }
      }
    }
  }
}

std::size_t IRUseDefAnalysis::GetUseCount(const IRValue* value) const {
  const auto it = users_.find(value);
  return it == users_.end() ? 0 : it->second.size();
}

const std::vector<const IRInstruction*>& IRUseDefAnalysis::GetUsers(
    const IRValue* value) const {
  static const std::vector<const IRInstruction*> empty;
  const auto it = users_.find(value);
  return it == users_.end() ? empty : it->second;
}

std::size_t ReplaceAllUses(IRProgram& program, const IRValue* old_value,
                           std::shared_ptr<IRValue> new_value) {
  if (!old_value || !new_value || old_value == new_value.get()) return 0;
  std::size_t changed_users = 0;
  for (auto& function : program.funcs) {
    for (auto& block : function->blocks) {
      for (auto& inst : block->insts) {
        if (inst && inst->ReplaceUse(old_value, new_value)) ++changed_users;
      }
    }
  }
  return changed_users;
}

std::vector<std::string> IRVerifier::Verify(const IRProgram& program) {
  std::vector<std::string> errors;
  for (const auto& function : program.funcs) {
    if (!function) {
      errors.push_back("program contains a null function");
      continue;
    }
    if (function->blocks.empty()) {
      errors.push_back("function @" + function->name + " has no basic block");
      continue;
    }

    std::unordered_set<const IRBasicBlock*> blocks;
    std::unordered_set<std::string> block_names;
    for (const auto& block : function->blocks) {
      if (!block) {
        errors.push_back("function @" + function->name +
                         " contains a null basic block");
        continue;
      }
      blocks.insert(block.get());
      if (!block_names.insert(block->name).second) {
        errors.push_back("function @" + function->name +
                         " contains duplicate block name " + block->name);
      }
    }

    std::unordered_set<std::string> definitions;
    for (const auto& param : function->params) {
      if (param && !definitions.insert(param->GetOperandName()).second) {
        errors.push_back("function @" + function->name +
                         " contains duplicate parameter " +
                         param->GetOperandName());
      }
    }

    for (const auto& block : function->blocks) {
      if (!block) continue;
      const std::string location = Location(*function, *block);
      if (block->insts.empty()) {
        errors.push_back(location + "is empty and has no terminator");
        continue;
      }

      for (std::size_t i = 0; i < block->insts.size(); ++i) {
        const auto& inst = block->insts[i];
        if (!inst) {
          errors.push_back(location + "contains a null instruction");
          continue;
        }
        if (!TagMatchesDynamicType(*inst)) {
          errors.push_back(location + "instruction tag does not match its C++ type");
        }
        if (IsTerminator(inst->inst_type) && i + 1 != block->insts.size()) {
          errors.push_back(location + "has an instruction after a terminator");
        }
        for (const auto& use : inst->GetUses()) {
          if (!use) errors.push_back(location + "contains a null operand");
        }

        if (const auto named = dynamic_cast_ptr<IRNameInstruction>(inst)) {
          if (!definitions.insert(named->GetOperandName()).second) {
            errors.push_back(location + "redefines " + named->GetOperandName());
          }
        } else if (const auto call = dynamic_cast_ptr<IRCall>(inst);
                   call && call->HasReturnValue()) {
          if (!definitions.insert(call->GetOperandName()).second) {
            errors.push_back(location + "redefines " + call->GetOperandName());
          }
        }

        if (const auto binary = dynamic_cast_ptr<IRBinary>(inst)) {
          if (!binary->GetLhs() || !binary->GetRhs() ||
              binary->GetLhs()->GetType() != binary->GetOperandType() ||
              binary->GetRhs()->GetType() != binary->GetOperandType()) {
            errors.push_back(location + "binary operand type mismatch");
          }
          const IRType expected = IsComparison(binary->GetOp())
                                      ? IRType::IR_INT32
                                      : binary->GetOperandType();
          if (binary->GetType() != expected) {
            errors.push_back(location + "binary result type mismatch");
          }
        } else if (const auto cast = dynamic_cast_ptr<IRCast>(inst)) {
          const bool valid =
              cast->GetValue() &&
              ((cast->GetOp() == IRCastOp::INT_TO_FLOAT &&
                cast->GetValue()->GetType() == IRType::IR_INT32 &&
                cast->GetType() == IRType::IR_FLOAT) ||
               (cast->GetOp() == IRCastOp::FLOAT_TO_INT &&
                cast->GetValue()->GetType() == IRType::IR_FLOAT &&
                cast->GetType() == IRType::IR_INT32));
          if (!valid) errors.push_back(location + "invalid cast types");
        } else if (const auto store = dynamic_cast_ptr<IRStore>(inst)) {
          if (!store->GetPtr() || store->GetPtr()->GetType() != IRType::IR_POINTER)
            errors.push_back(location + "store destination is not a pointer");
        } else if (const auto zero = dynamic_cast_ptr<IRZeroInit>(inst)) {
          if (!zero->GetPtr() || zero->GetPtr()->GetType() != IRType::IR_POINTER ||
              zero->GetByteSize() < 0 || zero->GetByteSize() % 4 != 0) {
            errors.push_back(location + "invalid zero_init instruction");
          }
        } else if (const auto load = dynamic_cast_ptr<IRLoad>(inst)) {
          if (!load->GetPtr() || load->GetPtr()->GetType() != IRType::IR_POINTER)
            errors.push_back(location + "load source is not a pointer");
        } else if (const auto gep = dynamic_cast_ptr<IRGetElemPtr>(inst)) {
          if (!gep->GetArr() || gep->GetArr()->GetType() != IRType::IR_POINTER ||
              !gep->GetIndex() || gep->GetIndex()->GetType() != IRType::IR_INT32 ||
              gep->GetElemSize() <= 0) {
            errors.push_back(location + "invalid getelemptr instruction");
          }
        } else if (const auto getptr = dynamic_cast_ptr<IRGetPtr>(inst)) {
          if (!getptr->GetPtr() ||
              getptr->GetPtr()->GetType() != IRType::IR_POINTER ||
              !getptr->GetIndex() ||
              getptr->GetIndex()->GetType() != IRType::IR_INT32 ||
              getptr->GetElemSize() <= 0) {
            errors.push_back(location + "invalid getptr instruction");
          }
        } else if (const auto branch = dynamic_cast_ptr<IRBranch>(inst)) {
          const auto then_block =
              std::dynamic_pointer_cast<IRBasicBlock>(branch->GetThenTarget());
          const auto else_block =
              std::dynamic_pointer_cast<IRBasicBlock>(branch->GetElseTarget());
          if (!branch->GetCond() ||
              branch->GetCond()->GetType() != IRType::IR_INT32) {
            errors.push_back(location + "branch condition is not i32");
          }
          if (!then_block || !else_block || blocks.count(then_block.get()) == 0 ||
              blocks.count(else_block.get()) == 0) {
            errors.push_back(location + "branch target is outside the function");
          }
        } else if (const auto jump = dynamic_cast_ptr<IRJump>(inst)) {
          const auto target =
              std::dynamic_pointer_cast<IRBasicBlock>(jump->GetTarget());
          if (!target || blocks.count(target.get()) == 0) {
            errors.push_back(location + "jump target is outside the function");
          }
        } else if (const auto ret = dynamic_cast_ptr<IRReturn>(inst)) {
          if (function->GetRetType() == IRType::IR_VOID) {
            if (ret->GetValue()) errors.push_back(location + "void return has a value");
          } else if (!ret->GetValue() ||
                     ret->GetValue()->GetType() != function->GetRetType()) {
            errors.push_back(location + "return type mismatch");
          }
        }
      }

      if (!block->insts.empty() &&
          !IsTerminator(block->insts.back()->inst_type)) {
        errors.push_back(location + "does not end with a terminator");
      }
    }
  }
  return errors;
}

void IRVerifier::VerifyOrThrow(const IRProgram& program) {
  const auto errors = Verify(program);
  if (errors.empty()) return;
  std::ostringstream message;
  message << "Custom IR verification failed with " << errors.size()
          << " error(s):";
  for (const auto& error : errors) message << "\n  - " << error;
  throw std::runtime_error(message.str());
}

IROptimizationStats RunO1Passes(IRProgram& program) {
  IRVerifier::VerifyOrThrow(program);
  IROptimizationStats stats;
  for (int iteration = 0; iteration < 8; ++iteration) {
    bool changed = false;
    changed = FoldConstants(program, stats) || changed;
    changed = SimplifyAlgebra(program, stats) || changed;
    changed = SimplifyCFG(program, stats) || changed;
    changed = EliminateDeadCode(program, stats) || changed;
    if (!changed) break;
  }
  IRVerifier::VerifyOrThrow(program);
  return stats;
}

}

