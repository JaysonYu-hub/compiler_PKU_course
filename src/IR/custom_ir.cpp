#include "custom_ir.hpp"
#include <cassert>

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

  // IRConstant
  std::string IRConstant::DumpText() const {
    return std::visit([](auto&& arg) -> std::string {
      return std::to_string(arg);
    }, val);
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

  std::string IRGlobalAlloc::FormatNestedInitializer(const std::vector<int32_t>& vals, 
                                             const std::vector<int>& dims, 
                                             size_t dim_idx, 
                                             size_t& val_idx) {
    if (dim_idx == dims.size() - 1) {
      std::string res = "{";
      for (int i = 0; i < dims[dim_idx]; i++) {
        if (val_idx < vals.size()) {
          res += std::to_string(vals[val_idx++]);
        } else {
          res += "0";
        }
        if (i != dims[dim_idx] - 1) {
          res += ", ";
        }
      }
      res += "}";
      return res;
    } else {
      std::string res = "{";
      for (int i = 0; i < dims[dim_idx]; i++) {
        res += FormatNestedInitializer(vals, dims, dim_idx + 1, val_idx);
        if (i != dims[dim_idx] - 1) {
          res += ", ";
        }
      }
      res += "}";
      return res;
    }
  }

  std::string IRGlobalAlloc::FormatNestedFloatInitializer(const std::vector<float>& vals, 
                                                  const std::vector<int>& dims, 
                                                  size_t dim_idx, 
                                                  size_t& val_idx) {
    if (dim_idx == dims.size() - 1) {
      std::string res = "{";
      for (int i = 0; i < dims[dim_idx]; i++) {
        if (val_idx < vals.size()) {
          res += std::to_string(vals[val_idx++]);
        } else {
          res += "0.0";
        }
        if (i != dims[dim_idx] - 1) {
          res += ", ";
        }
      }
      res += "}";
      return res;
    } else {
      std::string res = "{";
      for (int i = 0; i < dims[dim_idx]; i++) {
        res += FormatNestedFloatInitializer(vals, dims, dim_idx + 1, val_idx);
        if (i != dims[dim_idx] - 1) {
          res += ", ";
        }
      }
      res += "}";
      return res;
    }
  }

  std::string IRGlobalAlloc::DumpText() const {
    if (is_zeroinit) {
      return "global " + name + " = alloc " + type + ", zeroinit";
    }
    if (!init_float_vals.empty()) {
      if (init_float_vals.size() == 1 && array_dims.empty()) {
        return "global " + name + " = alloc " + type + ", " + std::to_string(init_float_vals[0]);
      }
      std::string str;
      if (!array_dims.empty()) {
        size_t val_idx = 0;
        str = FormatNestedFloatInitializer(init_float_vals, array_dims, 0, val_idx);
      } else {
        str = "{";
        for (size_t i = 0; i < init_float_vals.size(); i++) {
          str += std::to_string(init_float_vals[i]);
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

  // IRNameInstruction
  std::string IRNameInstruction::GetOperandName() const {
    return name;
  }

  // IRBinary

  std::string IRBinary::DumpText() const {
    return name + " = " + OpToString(op) + " " + lhs->GetOperandName() + ", " + rhs->GetOperandName();
  }

  IRType IRBinary::GetType() const {
    return IRType::IR_INT32;
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
    : IRInstruction(IR_Instruction_Type::IR_INST_STORE), val(std::move(val)), ptr(std::move(ptr)) {
    assert(this->ptr != nullptr && this->val != nullptr);
  }

  std::string IRStore::DumpText() const {
    return "store " + val->GetOperandName() + ", " + ptr->GetOperandName();
  }

  // IRLoad

  std::string IRLoad::DumpText() const {
    return name + " = load " + ptr->GetOperandName();
  }

  IRType IRLoad::GetType() const {
    return loaded_type;
  }

  // IRGetElemPtr
  std::string IRGetElemPtr::DumpText() const {
    return name + " = getelemptr " + arr->GetOperandName() + ", " + index->GetOperandName();
  }

  IRType IRGetElemPtr::GetType() const {
    return IRType::IR_POINTER;
  }

  // IRGetPtr
  std::string IRGetPtr::DumpText() const {
    return name + " = getptr " + ptr->GetOperandName() + ", " + index->GetOperandName();
  }

  IRType IRGetPtr::GetType() const {
    return IRType::IR_POINTER;
  }

  // IRJump

  std::string IRJump::DumpText() const {
    return "jump " + target->GetOperandName();
  }

  // IRBranch

  std::string IRBranch::DumpText() const {
    return "br " + cond->GetOperandName() + ", " + then_target->GetOperandName() + ", " + else_target->GetOperandName();
  }

  // IRReturn

  std::string IRReturn::DumpText() const {
    if (val) {
      return "ret " + val->GetOperandName();
    }
    return "ret";
  }

  // IRCall

  std::string IRCall::DumpText() const {
    std::string str;
    if (HasReturnValue()) {
      str += name + " = ";
    }
    str += "call " + func->GetOperandName() + "(";
    for (size_t i = 0; i < args.size(); i++) {
      str += args[i]->GetOperandName();
      if (i != args.size() - 1) {
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
    : name(std::move(name)), ret_type(ret_type), param_types(std::move(param_types)) {}

  std::string IRFuncDecl::DumpText() const {
    if (name == "getint") return "decl @getint() : i32";
    if (name == "getch") return "decl @getch() : i32";
    if (name == "getarray") return "decl @getarray(*i32) : i32";
    if (name == "putint") return "decl @putint(i32)";
    if (name == "putch") return "decl @putch(i32)";
    if (name == "putarray") return "decl @putarray(i32, *i32)";
    if (name == "starttime") return "decl @starttime()";
    if (name == "stoptime") return "decl @stoptime()";
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
