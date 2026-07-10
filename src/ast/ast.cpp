#include "ast.hpp"
#include <iostream>
#include <cassert>

// 用于store命令的右值
std::string LValAST::DumpOnlyAddress(Context& context) const { 
  auto sym = context.scope->lookup(*ident);
  if (sym == nullptr) {
    std::cerr << "Error: LVal " << *ident << " not found\n";
    return "";
  }
  if (sym->kind == SymbolKind::VARIABLE) {
    return std::get<std::string>(sym->value);
  } else if (sym->kind == SymbolKind::CONSTANT) {
    std::cerr << "Error: LVal " << *ident << " is a constant\n";
  } else if (sym->kind == SymbolKind::ARRAY) {
    auto ret_reg = context.NewReg();
    
    std::cout << "  " << ret_reg << " = getelemptr " << 
    std::get<ArrayInformation>(sym->value).address << ", " 
    << std::to_string(index_exp[0]->getValue(context).getInt()) << "\n";
    return ret_reg;
  } else {
    std::cerr << "Error: LVal " << *ident << " is not a variable or array\n";
  }
  return "";
}

ConstantVal ExpAST::getValue(Context& context) const {
  return lor_exp->getValue(context);
}

ConstantVal UnaryOPExpAST::getValue(Context& context) const {
  auto const_val = unary_exp->getValue(context);
  if (op->op == "+") {
    return const_val;
  } else if (op->op == "-") {
    if (const_val.isFloat()) {
      return -const_val.getFloat();
    } else {
      return -const_val.getInt();
    }
  } else if (op->op == "!") {
    if (const_val.isFloat()) {
      return const_val.getFloat() == 0.0f ? 1 : 0;
    } else {
      return const_val.getInt() == 0 ? 1 : 0;
    }
  } else {
    assert(false);
    return ConstantVal(0);
  }
}

ConstantVal PrimaryUnaryExpAST::getValue(Context& context) const {
  if (primary_exp) {
    return primary_exp->getValue(context);
  }
  return ConstantVal(0);
}

ConstantVal FuncCallExpAST::getValue(Context& context) const {
  std::cerr << "func can't getvalue before start" << "\n";
  return ConstantVal(0);
}

ConstantVal ParenthesizedExprAST::getValue(Context& context) const {
  return exp->getValue(context);
}

ConstantVal NumberPrimaryExprAST::getValue(Context& context) const {
  return value;
}

ConstantVal LValuePrimaryExprAST::getValue(Context& context) const {
  return lval->getValue(context);
}

ConstantVal MulAddExpAST::getValue(Context& context) const {
  if (mul_exp) {
    return mul_exp->getValue(context);
  }
  return ConstantVal(0);
}

ConstantVal BinaryAddExprAST::getValue(Context& context) const {
  auto add_val = add_exp->getValue(context);
  auto mul_val = mul_exp->getValue(context);
  if (add_val.isFloat() || mul_val.isFloat()) {
    float lhs = add_val.getFloat();
    float rhs = mul_val.getFloat();
    if (op->op == "+") return lhs + rhs;
    else if (op->op == "-") return lhs - rhs;
  } else {
    int lhs = add_val.getInt();
    int rhs = mul_val.getInt();
    if (op->op == "+") return lhs + rhs;
    else if (op->op == "-") return lhs - rhs;
  }
  assert(false);
  return ConstantVal(0);
}

ConstantVal UnaryMulExpAST::getValue(Context& context) const {
  if (unary_exp) {
    return unary_exp->getValue(context);
  }
  return ConstantVal(0);
}

ConstantVal BinaryMulExprAST::getValue(Context& context) const {
  auto mul_val = mul_exp->getValue(context);
  auto unary_val = unary_exp->getValue(context);
  if (mul_val.isFloat() || unary_val.isFloat()) {
    float lhs = mul_val.getFloat();
    float rhs = unary_val.getFloat();
    if (op->op == "*") return lhs * rhs;
    else if (op->op == "/") return lhs / rhs;
    else if (op->op == "%") return (int)lhs % (int)rhs;
  } else {
    int lhs = mul_val.getInt();
    int rhs = unary_val.getInt();
    if (op->op == "*") return lhs * rhs;
    else if (op->op == "/") return lhs / rhs;
    else if (op->op == "%") return lhs % rhs;
  }
  assert(false);
  return ConstantVal(0);
}

ConstantVal AddRelExpAST::getValue(Context& context) const {
  return add_exp->getValue(context);
}

ConstantVal BinaryRelExprAST::getValue(Context& context) const {
  auto rel_val = rel_exp->getValue(context);
  auto add_val = add_exp->getValue(context);
  if (rel_val.isFloat() || add_val.isFloat()) {
    float lhs = rel_val.getFloat();
    float rhs = add_val.getFloat();
    if (op->op == "<") return lhs < rhs ? 1 : 0;
    else if (op->op == ">") return lhs > rhs ? 1 : 0;
    else if (op->op == "<=") return lhs <= rhs ? 1 : 0;
    else if (op->op == ">=") return lhs >= rhs ? 1 : 0;
  } else {
    int lhs = rel_val.getInt();
    int rhs = add_val.getInt();
    if (op->op == "<") return lhs < rhs ? 1 : 0;
    else if (op->op == ">") return lhs > rhs ? 1 : 0;
    else if (op->op == "<=") return lhs <= rhs ? 1 : 0;
    else if (op->op == ">=") return lhs >= rhs ? 1 : 0;
  }
  assert(false);
  return ConstantVal(0);
}

ConstantVal RelEqExpAST::getValue(Context& context) const {
  return rel_exp->getValue(context);
}

ConstantVal BinaryEqExprAST::getValue(Context& context) const {
  auto eq_val = eq_exp->getValue(context);
  auto rel_val = rel_exp->getValue(context);
  if (eq_val.isFloat() || rel_val.isFloat()) {
    float lhs = eq_val.getFloat();
    float rhs = rel_val.getFloat();
    if (op->op == "==") return lhs == rhs ? 1 : 0;
    else if (op->op == "!=") return lhs != rhs ? 1 : 0;
  } else {
    int lhs = eq_val.getInt();
    int rhs = rel_val.getInt();
    if (op->op == "==") return lhs == rhs ? 1 : 0;
    else if (op->op == "!=") return lhs != rhs ? 1 : 0;
  }
  assert(false);
  return ConstantVal(0);
}

ConstantVal EqLAndExpAST::getValue(Context& context) const {
  return eq_exp->getValue(context);
}

ConstantVal BinaryLogicalAndExprAST::getValue(Context& context) const {
  auto land_val = land_exp->getValue(context);
  auto eq_val = eq_exp->getValue(context);
  bool lhs = land_val.isFloat() ? (land_val.getFloat() != 0.0f) : (land_val.getInt() != 0);
  bool rhs = eq_val.isFloat() ? (eq_val.getFloat() != 0.0f) : (eq_val.getInt() != 0);
  return (lhs && rhs) ? 1 : 0;
}

ConstantVal LAndLOrExpAST::getValue(Context& context) const {
  return land_exp->getValue(context);
}

ConstantVal BinaryLogicalOrExprAST::getValue(Context& context) const {
  auto lor_val = lor_exp->getValue(context);
  auto land_val = land_exp->getValue(context);
  bool lhs = lor_val.isFloat() ? (lor_val.getFloat() != 0.0f) : (lor_val.getInt() != 0);
  bool rhs = land_val.isFloat() ? (land_val.getFloat() != 0.0f) : (land_val.getInt() != 0);
  return (lhs || rhs) ? 1 : 0;
}

ConstantVal LValAST::getValue(Context& context) const { 
  auto sym = context.scope->lookup(*ident);
  if (sym == nullptr) {
    std::cerr << "Error: LVal " << *ident << " not found\n";
    return ConstantVal(0);
  }
  if (sym->kind == SymbolKind::CONSTANT) {
    if (std::holds_alternative<float>(sym->value)) {
      return std::get<float>(sym->value);
    } else {
      return std::get<int>(sym->value);
    }
  } else if (sym->kind == SymbolKind::CONST_ARRAY) {
    auto arr_info = std::get<ArrayInformation>(sym->value);
    std::vector<int> idxs;
    for (auto& expr : index_exp) {
      idxs.push_back(expr->getValue(context).getInt());
    }
    int flat_idx = 0;
    for (size_t i = 0; i < idxs.size(); i++) {
      int suffix_size = 1;
      for (size_t j = i + 1; j < arr_info.dims.size(); j++) {
        suffix_size *= arr_info.dims[j];
      }
      flat_idx += idxs[i] * suffix_size;
    }
    if (flat_idx >= 0 && flat_idx < (int)arr_info.GetTotalSize()) {
      if (!arr_info.const_float_values.empty()) {
        return arr_info.const_float_values[flat_idx];
      } else {
        return arr_info.const_values[flat_idx];
      }
    }
    return ConstantVal(0);
  } else if (sym->kind == SymbolKind::VARIABLE) {
    std::cerr << "Error: LVal is a variable " << *ident << "\n";
    return ConstantVal(0);
  } else {
    std::cerr << "Error: LVal " << *ident << " is not a variable, constant, or const array\n";
    return ConstantVal(0);
  }
}

ConstantVal ConstExpAST::getValue(Context& context) const { 
  return exp->getValue(context);
}

ConstantVal ConstExpConstInitValAST::getValue(Context& context) const { 
  return const_exp->getValue(context);
}

static void FlattenConstInitVal(const BaseAST& node, Context& context, std::vector<ConstantVal>& result) {
  if (const auto* p = dynamic_cast<const ConstExpConstInitValAST*>(&node)) {
    result.push_back(p->const_exp->getValue(context));
  } else if (const auto* p = dynamic_cast<const CompoundConstInitValueAST*>(&node)) {
    if (p->const_init_val_list) {
      for (const auto& child : p->const_init_val_list->const_init_vals) {
        FlattenConstInitVal(*child, context, result);
      }
    }
  }
}

ConstantVal CompoundConstInitValueAST::getValue(Context& context) const { 
  assert(false);
  return ConstantVal(0);
}

std::vector<ConstantVal> CompoundConstInitValueAST::getConstantVals(Context& context) const { 
  std::vector<ConstantVal> init_vals;
  FlattenConstInitVal(*this, context, init_vals);
  return init_vals;
}

ConstantVal InitValAST::getValue(Context& context) const { 
  return exp->getValue(context);
}

static void FlattenInitValValues(const BaseAST& node, Context& context, std::vector<ConstantVal>& result) {
  if (const auto* p = dynamic_cast<const InitValAST*>(&node)) {
    result.push_back(p->exp->getValue(context));
  } else if (const auto* p = dynamic_cast<const InitValListAST*>(&node)) {
    for (const auto& child : p->init_vals) {
      FlattenInitValValues(*child, context, result);
    }
  }
}

std::vector<ConstantVal> InitValListAST::getConstantVals(Context& context) const { 
  std::vector<ConstantVal> ret;
  FlattenInitValValues(*this, context, ret);
  return ret;
}