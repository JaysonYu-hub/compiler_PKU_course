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
    if (type == "void") return IRType::IR_VOID;
    if (type == "ptr") return IRType::IR_POINTER;
    if (type == "array") return IRType::IR_ARRAY;
    assert(false && "Invalid IR type");
    return IRType::IR_VOID;
  }

  // IRConstant
  std::string IRConstant::DumpText() const {
    return std::to_string(val);
  }
  std::string IRConstant::GetOperandName() const {
    return std::to_string(val);
  }
  IRType IRConstant::GetType() const {
    return IRType::IR_INT32;
  }

  // IRSymbol
  std::string IRSymbol::DumpText() const {
    return name;
  }
  std::string IRSymbol::GetOperandName() const {
    return name;
  }
  IRType IRSymbol::GetType() const {
    return IRType::IR_POINTER;
  }

  // IRGlobalAlloc
  IRGlobalAlloc::IRGlobalAlloc(std::string name, std::string type, bool is_zeroinit, std::vector<int32_t> init_vals, std::vector<int> array_dims)
    : name(std::move(name)), type(std::move(type)), is_zeroinit(is_zeroinit), init_vals(std::move(init_vals)), array_dims(std::move(array_dims)) {}

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

  std::string IRGlobalAlloc::DumpText() const {
    if (is_zeroinit) {
      return "global " + name + " = alloc " + type + ", zeroinit";
    }
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

  std::string IRGlobalAlloc::GetOperandName() const {
    return name;
  }

  IRType IRGlobalAlloc::GetType() const {
    return IRType::IR_POINTER;
  }

  // IRFuncParam
  IRFuncParam::IRFuncParam(std::string name, IRType type, std::string type_str) 
    : name(std::move(name)), type(type), type_str(std::move(type_str)) {}

  std::string IRFuncParam::DumpText() const {
    return name + ": " + type_str;
  }

  std::string IRFuncParam::GetOperandName() const {
    return name;
  }

  IRType IRFuncParam::GetType() const {
    return type;
  }

  // IRInstruction
  std::string IRInstruction::GetOperandName() const {
    assert(false && "Void instruction cannot be used as an operand");
    return "";
  }

  IRType IRInstruction::GetType() const {
    return IRType::IR_VOID;
  }

  // IRNameInstruction
  std::string IRNameInstruction::GetOperandName() const {
    return name;
  }

  // IRBinary
  IRBinary::IRBinary(std::string name, IRBinaryOp op, std::shared_ptr<IRValue> lhs, std::shared_ptr<IRValue> rhs)
    : IRNameInstruction(std::move(name)), op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {
    assert(this->lhs != nullptr && this->rhs != nullptr);
  }

  std::string IRBinary::DumpText() const {
    return name + " = " + OpToString(op) + " " + lhs->GetOperandName() + ", " + rhs->GetOperandName();
  }

  IRType IRBinary::GetType() const {
    return IRType::IR_INT32;
  }

  // IRAlloc
  IRAlloc::IRAlloc(std::string name, std::string type_str, std::vector<int> array_dims)
    : IRNameInstruction(std::move(name)), type(StringToType(type_str)), array_dims(std::move(array_dims)), alloc_type_str(std::move(type_str)) {}

  IRAlloc::IRAlloc(std::string name, std::string type_str, int array_size)
    : IRAlloc(std::move(name), std::move(type_str), std::vector<int>{array_size}) {}

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

  // IRStore
  IRStore::IRStore(std::shared_ptr<IRValue> val, std::shared_ptr<IRValue> ptr)
    : val(std::move(val)), ptr(std::move(ptr)) {
    assert(this->ptr != nullptr && this->val != nullptr);
  }

  std::string IRStore::DumpText() const {
    return "store " + val->GetOperandName() + ", " + ptr->GetOperandName();
  }

  // IRLoad
  IRLoad::IRLoad(std::string name, std::shared_ptr<IRValue> ptr)
    : IRNameInstruction(std::move(name)), ptr(std::move(ptr)) {
    assert(this->ptr != nullptr);
  }

  std::string IRLoad::DumpText() const {
    return name + " = load " + ptr->GetOperandName();
  }

  IRType IRLoad::GetType() const {
    return IRType::IR_INT32;
  }

  // IRGetElemPtr
  IRGetElemPtr::IRGetElemPtr(std::string name, std::shared_ptr<IRValue> arr, std::shared_ptr<IRValue> index, int elem_size)
    : IRNameInstruction(std::move(name)), arr(std::move(arr)), index(std::move(index)), elem_size(elem_size) {
    assert(this->arr != nullptr && this->index != nullptr);
  }

  std::string IRGetElemPtr::DumpText() const {
    return name + " = getelemptr " + arr->GetOperandName() + ", " + index->GetOperandName();
  }

  IRType IRGetElemPtr::GetType() const {
    return IRType::IR_POINTER;
  }

  // IRGetPtr
  IRGetPtr::IRGetPtr(std::string name, std::shared_ptr<IRValue> ptr, std::shared_ptr<IRValue> index, int elem_size)
    : IRNameInstruction(std::move(name)), ptr(std::move(ptr)), index(std::move(index)), elem_size(elem_size) {
    assert(this->ptr != nullptr && this->index != nullptr);
  }

  std::string IRGetPtr::DumpText() const {
    return name + " = getptr " + ptr->GetOperandName() + ", " + index->GetOperandName();
  }

  IRType IRGetPtr::GetType() const {
    return IRType::IR_POINTER;
  }

  // IRJump
  IRJump::IRJump(std::shared_ptr<IRValue> target) : target(std::move(target)) {
    assert(this->target != nullptr);
  }

  std::string IRJump::DumpText() const {
    return "jump " + target->GetOperandName();
  }

  // IRBranch
  IRBranch::IRBranch(std::shared_ptr<IRValue> cond, std::shared_ptr<IRValue> then_target, std::shared_ptr<IRValue> else_target)
    : cond(std::move(cond)), then_target(std::move(then_target)), else_target(std::move(else_target)) {
    assert(this->cond != nullptr && this->then_target != nullptr && this->else_target != nullptr);
  }

  std::string IRBranch::DumpText() const {
    return "br " + cond->GetOperandName() + ", " + then_target->GetOperandName() + ", " + else_target->GetOperandName();
  }

  // IRReturn
  IRReturn::IRReturn(std::shared_ptr<IRValue> val) : val(std::move(val)) {}

  std::string IRReturn::DumpText() const {
    if (val) {
      return "ret " + val->GetOperandName();
    }
    return "ret";
  }

  // IRCall
  IRCall::IRCall(std::string name, std::shared_ptr<IRValue> func, std::vector<std::shared_ptr<IRValue>> args, IRType ret_type)
    : name(std::move(name)), func(std::move(func)), args(std::move(args)), ret_type(ret_type) {
    assert(this->func != nullptr);
  }

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

  IRType IRCall::GetType() const {
    return ret_type;
  }

  // IRBasicBlock
  IRBasicBlock::IRBasicBlock(std::string name) : name(std::move(name)) {}

  void IRBasicBlock::AddInstruction(std::shared_ptr<IRInstruction> inst) {
    insts.push_back(std::move(inst));
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
    std::string text = "decl @" + name + "(";
    for (size_t i = 0; i < param_types.size(); i++) {
      if (param_types[i] == IRType::IR_INT32) {
        text += "i32";
      } else if (param_types[i] == IRType::IR_POINTER) {
        text += "*i32";
      }
      if (i != param_types.size() - 1) {
        text += ", ";
      }
    }
    text += ")";
    if (ret_type != IRType::IR_VOID) {
      text += " : i32";
    }
    return text;
  }

  std::string IRFuncDecl::GetOperandName() const {
    return "@" + name;
  }

  IRType IRFuncDecl::GetType() const {
    return ret_type;
  }

  // IRFunction
  IRFunction::IRFunction(std::string name, IRType ret_type) : name(std::move(name)), ret_type(ret_type) {}

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
      text += " : i32";
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

  IRType IRFunction::GetType() const {
    return ret_type;
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
