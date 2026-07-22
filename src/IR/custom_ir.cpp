#include "custom_ir.hpp"
#include <cassert>
#include <iomanip>
#include <limits>
#include <sstream>

namespace IR {

  // 辅助转换函数定义
  std::string OpToString(IRBinaryOp op) {
    switch (op) {
      case IRBinaryOp::ADD: return "add";
      case IRBinaryOp::SUB: return "sub";
      case IRBinaryOp::MUL: return "mul";
      case IRBinaryOp::DIV: return "div";
      case IRBinaryOp::MOD: return "mod";
      case IRBinaryOp::AND: return "and";
      case IRBinaryOp::OR:  return "or";
      case IRBinaryOp::XOR: return "xor";
      case IRBinaryOp::SHL: return "shl";
      case IRBinaryOp::SHR: return "shr";
      case IRBinaryOp::SAR: return "sar";
      case IRBinaryOp::EQ:  return "eq";
      case IRBinaryOp::NE:  return "ne";
      case IRBinaryOp::LT:  return "lt";
      case IRBinaryOp::LE:  return "le";
      case IRBinaryOp::GT:  return "gt";
      case IRBinaryOp::GE:  return "ge";
      default: assert(false && "Invalid binary operator");
    }
    return "";
  }

  std::string CastOpToString(IRCastOp op) {
    switch (op) {
      case IRCastOp::INT_TO_FLOAT: return "sitofp";
      case IRCastOp::FLOAT_TO_INT: return "fptosi";
      default: assert(false && "Invalid cast operator");
    }
    return "";
  }

  std::string TypeToString(IRType type) {
    switch (type) {
      case IRType::IR_INT32: return "i32";
      case IRType::IR_FLOAT: return "float";
      case IRType::IR_VOID:  return "void";
      case IRType::IR_POINTER: return "*i32";
      case IRType::IR_ARRAY: return "array";
      default: assert(false && "Invalid IR type");
    }
    return "";
  }

  IRType StringToType(const std::string &type) {
    if (type.empty()) return IRType::IR_VOID;
    if (type[0] == '*') return IRType::IR_POINTER;
    if (type[0] == '[') return IRType::IR_ARRAY;
    if (type == "i32") return IRType::IR_INT32;
    if (type == "float" || type == "f32") return IRType::IR_FLOAT;
    if (type == "void") return IRType::IR_VOID;
    if (type == "ptr") return IRType::IR_POINTER;
    if (type == "array") return IRType::IR_ARRAY;
    assert(false && "Invalid IR type");
    return IRType::IR_VOID;
  }

  std::string FloatToString(float value) {
    std::ostringstream stream;
    stream << std::setprecision(std::numeric_limits<float>::max_digits10)
           << value;
    std::string text = stream.str();

    // Keep the textual value distinguishable from an integer while retaining
    // enough digits for an exact float -> text -> float round trip.
    if (text.find('.') == std::string::npos) {
      const size_t exponent = text.find_first_of("eE");
      if (exponent == std::string::npos) {
        text += ".0";
      } else {
        text.insert(exponent, ".0");
      }
    }
    return text;
  }

  // IRConstant
  std::string IRConstant::DumpText() const {
    if (const_type == IRType::IR_FLOAT) {
      return FloatToString(std::get<float>(val));
    }
    return std::to_string(std::get<int32_t>(val));
  }
  std::string IRConstant::GetOperandName() const {
    return DumpText();
  }

  // IRSymbol
  std::string IRSymbol::DumpText() const {
    return name;
  }
  std::string IRSymbol::GetOperandName() const {
    return name;
  }

  // IRGlobalAlloc

  namespace {
  template <typename T>
  std::string FormatNestedInitializerImpl(const std::vector<T>& vals,
                                          const std::vector<int>& dims,
                                          size_t dim_idx,
                                          size_t& val_idx) {
    std::string res = "{";
    for (int i = 0; i < dims[dim_idx]; i++) {
      if (dim_idx == dims.size() - 1) {
        if (val_idx < vals.size()) {
          if constexpr (std::is_same_v<T, float>) {
            res += FloatToString(vals[val_idx++]);
          } else {
            res += std::to_string(vals[val_idx++]);
          }
        } else {
          res += std::is_same_v<T, float> ? "0.0" : "0";
        }
      } else {
        res += FormatNestedInitializerImpl(vals, dims, dim_idx + 1, val_idx);
      }
      if (i != dims[dim_idx] - 1) {
        res += ", ";
      }
    }
    res += "}";
    return res;
  }
  }

  std::string IRGlobalAlloc::FormatNestedInitializer(const std::vector<int32_t>& vals, 
                                             const std::vector<int>& dims, 
                                             size_t dim_idx, 
                                             size_t& val_idx) {
    return FormatNestedInitializerImpl(vals, dims, dim_idx, val_idx);
  }

  std::string IRGlobalAlloc::FormatNestedFloatInitializer(const std::vector<float>& vals, 
                                                  const std::vector<int>& dims, 
                                                  size_t dim_idx, 
                                                  size_t& val_idx) {
    return FormatNestedInitializerImpl(vals, dims, dim_idx, val_idx);
  }

  std::string IRGlobalAlloc::DumpText() const {
    if (is_zeroinit) {
      return "global " + name + " = alloc " + type + ", zeroinit";
    }
    if (!init_float_vals.empty()) {
      if (init_float_vals.size() == 1 && array_dims.empty()) {
        return "global " + name + " = alloc " + type + ", " + FloatToString(init_float_vals[0]);
      }
      std::string str;
      if (!array_dims.empty()) {
        size_t val_idx = 0;
        str = FormatNestedFloatInitializer(init_float_vals, array_dims, 0, val_idx);
      } else {
        str = "{";
        for (size_t i = 0; i < init_float_vals.size(); i++) {
          str += FloatToString(init_float_vals[i]);
          if (i != init_float_vals.size() - 1) {
            str += ", ";
          }
        }
        str += "}";
      }
      return "global " + name + " = alloc " + type + ", " + str;
    } else {
      if (init_vals.size() == 1 && array_dims.empty()) {
        return "global " + name + " = alloc " + type + ", " + std::to_string(init_vals[0]);
      }
      std::string str;
      if (!array_dims.empty()) {
        size_t val_idx = 0;
        str = FormatNestedInitializer(init_vals, array_dims, 0, val_idx);
      } else {
        str = "{";
        for (size_t i = 0; i < init_vals.size(); i++) {
          str += std::to_string(init_vals[i]);
          if (i != init_vals.size() - 1) {
            str += ", ";
          }
        }
        str += "}";
      }
      return "global " + name + " = alloc " + type + ", " + str;
    }
  }

  std::string IRGlobalAlloc::GetOperandName() const {
    return name;
  }

  // IRFuncParam
  std::string IRFuncParam::DumpText() const {
    return name + ": " + type_str;
  }

  std::string IRFuncParam::GetOperandName() const {
    return name;
  }

  // IRInstruction
  std::string IRInstruction::GetOperandName() const {
    assert(false && "Void instruction cannot be used as an operand");
    return "";
  }

  bool IRInstruction::ReplaceUse(const IRValue* old_value,
                                 std::shared_ptr<IRValue> new_value) {
    if (!new_value) return false;
    bool changed = false;
    for (auto& op : operands) {
      if (op.get() == old_value) {
        op = new_value;
        changed = true;
      }
    }
    return changed;
  }

  // IRNameInstruction
  std::string IRNameInstruction::GetOperandName() const {
    return name;
  }

  // IRBinary

  namespace {
  bool IsComparisonOp(IRBinaryOp op) {
    return op == IRBinaryOp::EQ || op == IRBinaryOp::NE ||
           op == IRBinaryOp::LT || op == IRBinaryOp::LE ||
           op == IRBinaryOp::GT || op == IRBinaryOp::GE;
  }
  }

  IRBinary::IRBinary(std::string name, IRBinaryOp op,
                     std::shared_ptr<IRValue> lhs,
                     std::shared_ptr<IRValue> rhs,
                     IRType operand_type, IRType result_type)
    : IRNameInstruction(IRInstructionType::IR_INST_BINARY, std::move(name)),
      op(op), operand_type(operand_type), result_type(result_type) {
    operands = {std::move(lhs), std::move(rhs)};
    assert(operands[0] != nullptr && operands[1] != nullptr);
  }

  IRBinary::IRBinary(std::string name, IRBinaryOp op,
                     std::shared_ptr<IRValue> lhs,
                     std::shared_ptr<IRValue> rhs)
    : IRNameInstruction(IRInstructionType::IR_INST_BINARY, std::move(name)),
      op(op) {
    operands = {std::move(lhs), std::move(rhs)};
    assert(operands[0] != nullptr && operands[1] != nullptr);
    operand_type = (operands[0]->GetType() == IRType::IR_FLOAT ||
                    operands[1]->GetType() == IRType::IR_FLOAT)
                     ? IRType::IR_FLOAT
                     : IRType::IR_INT32;
    result_type = IsComparisonOp(op) ? IRType::IR_INT32 : operand_type;
  }

  std::string IRBinary::DumpText() const {
    const std::string prefix = operand_type == IRType::IR_FLOAT ? "f" : "";
    return name + " = " + prefix + OpToString(op) + " " +
           operands[0]->GetOperandName() + ", " + operands[1]->GetOperandName();
  }

  IRType IRBinary::GetType() const {
    return result_type;
  }

  // IRCast

  IRCast::IRCast(std::string name, IRCastOp op, std::shared_ptr<IRValue> value,
                 IRType target_type)
    : IRNameInstruction(IRInstructionType::IR_INST_CAST, std::move(name)),
      op(op), target_type(target_type) {
    operands = {std::move(value)};
    assert(operands[0] != nullptr);
  }

  std::string IRCast::DumpText() const {
    return name + " = " + CastOpToString(op) + " " + operands[0]->GetOperandName();
  }

  // IRAlloc

  std::string IRAlloc::DumpText() const {
    if (IsArray()) {
      std::string type_str = alloc_type_str;
      for (auto it = array_dims.rbegin(); it != array_dims.rend(); ++it) {
        type_str = "[" + type_str + ", " + std::to_string(*it) + "]";
      }
      return name + " = alloc " + type_str;
    }
    return name + " = alloc " + alloc_type_str;
  }

  IRType IRAlloc::GetType() const {
    return IRType::IR_POINTER;
  }

  // store 指令类
  IRStore::IRStore(std::shared_ptr<IRValue> val, std::shared_ptr<IRValue> ptr)
    : IRInstruction(IRInstructionType::IR_INST_STORE) {
    operands = {std::move(val), std::move(ptr)};
    assert(operands[0] != nullptr && operands[1] != nullptr);
  }

  std::string IRStore::DumpText() const {
    return "store " + operands[0]->GetOperandName() + ", " + operands[1]->GetOperandName();
  }

  IRZeroInit::IRZeroInit(std::shared_ptr<IRValue> ptr, int byte_size)
    : IRInstruction(IRInstructionType::IR_INST_ZERO_INIT), byte_size(byte_size) {
    operands = {std::move(ptr)};
    assert(operands[0] != nullptr);
    assert(this->byte_size >= 0 && this->byte_size % 4 == 0);
  }

  std::string IRZeroInit::DumpText() const {
    return "zero_init " + operands[0]->GetOperandName() + ", " +
           std::to_string(byte_size);
  }

  // IRLoad

  IRLoad::IRLoad(std::string name, std::shared_ptr<IRValue> ptr, IRType type)
    : IRNameInstruction(IRInstructionType::IR_INST_LOAD, std::move(name)), loaded_type(type) {
    operands = {std::move(ptr)};
    assert(operands[0] != nullptr);
  }

  std::string IRLoad::DumpText() const {
    return name + " = load " + operands[0]->GetOperandName();
  }

  IRType IRLoad::GetType() const {
    return loaded_type;
  }

  // IRGetElemPtr
  IRGetElemPtr::IRGetElemPtr(std::string name, std::shared_ptr<IRValue> arr, std::shared_ptr<IRValue> index, int elem_size)
    : IRNameInstruction(IRInstructionType::IR_INST_GETELEMPTR, std::move(name)), elem_size(elem_size) {
    operands = {std::move(arr), std::move(index)};
    assert(operands[0] != nullptr && operands[1] != nullptr);
  }

  std::string IRGetElemPtr::DumpText() const {
    return name + " = getelemptr " + operands[0]->GetOperandName() + ", " + operands[1]->GetOperandName();
  }

  IRType IRGetElemPtr::GetType() const {
    return IRType::IR_POINTER;
  }

  // IRGetPtr
  IRGetPtr::IRGetPtr(std::string name, std::shared_ptr<IRValue> ptr, std::shared_ptr<IRValue> index, int elem_size)
    : IRNameInstruction(IRInstructionType::IR_INST_GETPTR, std::move(name)), elem_size(elem_size) {
    operands = {std::move(ptr), std::move(index)};
    assert(operands[0] != nullptr && operands[1] != nullptr);
  }

  std::string IRGetPtr::DumpText() const {
    return name + " = getptr " + operands[0]->GetOperandName() + ", " + operands[1]->GetOperandName();
  }

  IRType IRGetPtr::GetType() const {
    return IRType::IR_POINTER;
  }

  // IRJump
  IRJump::IRJump(std::shared_ptr<IRValue> target)
    : IRInstruction(IRInstructionType::IR_INST_JUMP) {
    operands = {std::move(target)};
    assert(operands[0] != nullptr);
  }

  std::string IRJump::DumpText() const {
    return "jump " + operands[0]->GetOperandName();
  }

  // IRBranch
  IRBranch::IRBranch(std::shared_ptr<IRValue> cond, std::shared_ptr<IRValue> then_target, std::shared_ptr<IRValue> else_target)
    : IRInstruction(IRInstructionType::IR_INST_BR), then_target(std::move(then_target)), else_target(std::move(else_target)) {
    operands = {std::move(cond)};
    assert(operands[0] != nullptr && this->then_target != nullptr && this->else_target != nullptr);
  }

  std::string IRBranch::DumpText() const {
    return "br " + operands[0]->GetOperandName() + ", " + then_target->GetOperandName() + ", " + else_target->GetOperandName();
  }

  // IRReturn
  IRReturn::IRReturn(std::shared_ptr<IRValue> val)
    : IRInstruction(IRInstructionType::IR_INST_RET) {
    if (val) operands = {std::move(val)};
  }

  std::string IRReturn::DumpText() const {
    if (!operands.empty() && operands[0]) {
      return "ret " + operands[0]->GetOperandName();
    }
    return "ret";
  }

  // IRCall
  IRCall::IRCall(std::string name, std::shared_ptr<IRValue> func, std::vector<std::shared_ptr<IRValue>> args, IRType ret_type)
    : IRInstruction(IRInstructionType::IR_INST_CALL), name(std::move(name)), func(std::move(func)), ret_type(ret_type) {
    operands = std::move(args);
    assert(this->func != nullptr);
  }

  std::string IRCall::DumpText() const {
    std::string str;
    if (HasReturnValue()) {
      str += name + " = ";
    }
    str += "call " + func->GetOperandName() + "(";
    for (size_t i = 0; i < operands.size(); i++) {
      str += operands[i]->GetOperandName();
      if (i != operands.size() - 1) {
        str += ", ";
      }
    }
    str += ")";
    return str;
  }

  std::string IRCall::GetOperandName() const {
    if (HasReturnValue()) {
      return name;
    }
    assert(false && "Void call instruction cannot be used as an operand");
    return "";
  }

  // IRBasicBlock

  void IRBasicBlock::AddInstruction(std::shared_ptr<IRInstruction> inst) {
    insts.push_back(std::move(inst));
  }

  void IRBasicBlock::SetNext(std::shared_ptr<IRBasicBlock> next) {
    next_1 = std::move(next);
  }

  void IRBasicBlock::SetNext(std::shared_ptr<IRBasicBlock> next_1, std::shared_ptr<IRBasicBlock> next_2) {
    this->next_1 = std::move(next_1);
    this->next_2 = std::move(next_2);
  }

  std::vector<IRBasicBlock*> IRBasicBlock::GetSuccessors() const {
    if (insts.empty()) return {};
    auto last = insts.back();
    if (auto br = dyn_cast<IRBranch>(last.get())) {
      std::vector<IRBasicBlock*> succs;
      if (auto t1 = dyn_cast<IRBasicBlock>(br->GetThenTarget().get())) succs.push_back(t1);
      if (auto t2 = dyn_cast<IRBasicBlock>(br->GetElseTarget().get())) succs.push_back(t2);
      return succs;
    }
    if (auto jmp = dyn_cast<IRJump>(last.get())) {
      if (auto t = dyn_cast<IRBasicBlock>(jmp->GetTarget().get())) return {t};
    }
    return {};
  }

  std::string IRBasicBlock::DumpText() const {
    std::string text = name + ":\n";
    for (const auto& inst : insts) {
      text += "  " + inst->DumpText() + "\n";
    }
    return text;
  }

  std::string IRBasicBlock::GetOperandName() const {
    return name;
  }

  IRType IRBasicBlock::GetType() const {
    return IRType::IR_VOID;
  }

  // IRFuncDecl
  IRFuncDecl::IRFuncDecl(std::string name, IRType ret_type, std::vector<IRType> param_types)
    : IRValue(IRValueKind::IR_FUNC_DECL), name(std::move(name)), ret_type(ret_type), param_types(std::move(param_types)) {}

  std::string IRFuncDecl::DumpText() const {
    if (name == "getint") return "decl @getint() : i32";
    if (name == "getch") return "decl @getch() : i32";
    if (name == "getarray") return "decl @getarray(*i32) : i32";
    if (name == "putint") return "decl @putint(i32)";
    if (name == "putch") return "decl @putch(i32)";
    if (name == "putarray") return "decl @putarray(i32, *i32)";
    if (name == "_sysy_starttime") return "decl @_sysy_starttime(i32)";
    if (name == "_sysy_stoptime") return "decl @_sysy_stoptime(i32)";
    if (name == "getfloat") return "decl @getfloat() : float";
    if (name == "getfarray") return "decl @getfarray(*float) : i32";
    if (name == "putfloat") return "decl @putfloat(float)";
    if (name == "putfarray") return "decl @putfarray(i32, *float)";

    std::string text = "decl @" + name + "(";
    for (size_t i = 0; i < param_types.size(); i++) {
      if (param_types[i] == IRType::IR_INT32) {
        text += "i32";
      } else if (param_types[i] == IRType::IR_FLOAT) {
        text += "float";
      } else if (param_types[i] == IRType::IR_POINTER) {
        text += "*i32";
      }
      if (i != param_types.size() - 1) {
        text += ", ";
      }
    }
    text += ")";
    if (ret_type != IRType::IR_VOID) {
      text += " : ";
      text += (ret_type == IRType::IR_FLOAT ? "float" : "i32");
    }
    return text;
  }

  std::string IRFuncDecl::GetOperandName() const {
    return "@" + name;
  }


  // IRFunction

  std::string IRFunction::DumpText() const {
    std::string text = "fun @" + name + "(";
    for (size_t i = 0; i < params.size(); i++) {
      text += params[i]->DumpText();
      if (i != params.size() - 1) {
        text += ", ";
      }
    }
    text += ")";
    if (ret_type != IRType::IR_VOID) {
      text += " : ";
      text += (ret_type == IRType::IR_FLOAT ? "float" : "i32");
    }
    text += " {\n";
    for (const auto& block : blocks) {
      text += block->DumpText();
    }
    text += "}\n";
    return text;
  }

  std::string IRFunction::GetOperandName() const {
    return "@" + name;
  }

  // IRProgram
  std::string IRProgram::DumpText() const {
    std::string text;
    for (const auto& decl : decls) {
      text += decl->DumpText() + "\n";
    }
    if (!decls.empty()) {
      text += "\n";
    }
    for (const auto& global : globals) {
      text += global->DumpText() + "\n";
    }
    if (!globals.empty()) {
      text += "\n";
    }
    for (const auto& func : funcs) {
      text += func->DumpText() + "\n";
    }
    return text;
  }

}
