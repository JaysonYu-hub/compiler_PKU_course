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
    << std::to_string(index_exp[0]->getValue(context)) << "\n";
    return ret_reg;
  } else {
    std::cerr << "Error: LVal " << *ident << " is not a variable or array\n";
  }
  return "";
}

int ExpAST::getValue(Context& context) const {
  return lor_exp->getValue(context);
}

int UnaryOPExpAST::getValue(Context& context) const {
  if (op->op == "+") {
    return unary_exp->getValue(context);
  } else if (op->op == "-") {
    return -unary_exp->getValue(context);
  } else if (op->op == "!") {
    return unary_exp->getValue(context) == 0 ? 1 : 0;
  } else {
    return 0;
  }
}

int PrimaryUnaryExpAST::getValue(Context& context) const {
  if (primary_exp) {
    return primary_exp->getValue(context);
  }
  return 0;
}

int FuncCallExpAST::getValue(Context& context) const {
  std::cerr << "func can't getvalue before start" << "\n";
  return 0;
}

int ParenthesizedExprAST::getValue(Context& context) const {
  return exp->getValue(context);
}

int NumberPrimaryExprAST::getValue(Context& context) const {
  return value;
}

int LValuePrimaryExprAST::getValue(Context& context) const {
  return lval->getValue(context);
}

int MulAddExpAST::getValue(Context& context) const {
  if (mul_exp) {
    return mul_exp->getValue(context);
  }
  return 0;
}

int BinaryAddExprAST::getValue(Context& context) const {
  if (op->op == "+") {
    return add_exp->getValue(context) + mul_exp->getValue(context);
  } else if (op->op == "-") {
    return add_exp->getValue(context) - mul_exp->getValue(context);
  } else {
    return 0;
  }
}

int UnaryMulExpAST::getValue(Context& context) const {
  if (unary_exp) {
    return unary_exp->getValue(context);
  }
  return 0;
}

int BinaryMulExprAST::getValue(Context& context) const {
  if (op->op == "*") {
    return mul_exp->getValue(context) * unary_exp->getValue(context);
  } else if (op->op == "/") {
    return mul_exp->getValue(context) / unary_exp->getValue(context);
  } else if (op->op == "%") {
    return mul_exp->getValue(context) % unary_exp->getValue(context);
  } else {
    return 0;
  }
}

int AddRelExpAST::getValue(Context& context) const {
  return add_exp->getValue(context);
}

int BinaryRelExprAST::getValue(Context& context) const {
  if (op->op == "<") {
    return rel_exp->getValue(context) < add_exp->getValue(context);
  } else if (op->op == ">") {
    return rel_exp->getValue(context) > add_exp->getValue(context);
  } else if (op->op == "<=") {
    return rel_exp->getValue(context) <= add_exp->getValue(context);
  } else if (op->op == ">=") {
    return rel_exp->getValue(context) >= add_exp->getValue(context);
  } else {
    return 0;
  }
}

int RelEqExpAST::getValue(Context& context) const {
  return rel_exp->getValue(context);
}

int BinaryEqExprAST::getValue(Context& context) const {
  if (op->op == "==") {
    return eq_exp->getValue(context) == rel_exp->getValue(context);
  } else if (op->op == "!=") {
    return eq_exp->getValue(context) != rel_exp->getValue(context);
  } else {
    return 0;
  }
}

int EqLAndExpAST::getValue(Context& context) const {
  return eq_exp->getValue(context);
}

int BinaryLogicalAndExprAST::getValue(Context& context) const {
  if (op->op == "&&") {
    return land_exp->getValue(context) && eq_exp->getValue(context);
  } else {
    return 0;
  }
}

int LAndLOrExpAST::getValue(Context& context) const {
  return land_exp->getValue(context);
}

int BinaryLogicalOrExprAST::getValue(Context& context) const {
  if (op->op == "||") {
    return lor_exp->getValue(context) || land_exp->getValue(context);
  } else {
    return 0;
  }
}

int LValAST::getValue(Context& context) const { 
  auto sym = context.scope->lookup(*ident);
  if (sym == nullptr) {
    std::cerr << "Error: LVal " << *ident << " not found\n";
    return 0;
  }
  if (sym->kind == SymbolKind::CONSTANT) {
    return std::get<int>(sym->value);
  } else if (sym->kind == SymbolKind::CONST_ARRAY) {
    auto arr_info = std::get<ArrayInformation>(sym->value);
    std::vector<int> idxs;
    for (auto& expr : index_exp) {
      idxs.push_back(expr->getValue(context));
    }
    int flat_idx = 0;
    for (size_t i = 0; i < idxs.size(); i++) {
      int suffix_size = 1;
      for (size_t j = i + 1; j < arr_info.dims.size(); j++) {
        suffix_size *= arr_info.dims[j];
      }
      flat_idx += idxs[i] * suffix_size;
    }
    if (flat_idx >= 0 && flat_idx < (int)arr_info.const_values.size()) {
      return arr_info.const_values[flat_idx];
    }
    return 0;
  } else if (sym->kind == SymbolKind::VARIABLE) {
    std::cerr << "Error: LVal is a variable " << *ident << "\n";
    return 0;
  } else {
    std::cerr << "Error: LVal " << *ident << " is not a variable, constant, or const array\n";
    return 0;
  }
}

int ConstExpAST::getValue(Context& context) const { 
  return exp->getValue(context);
}

int ConstExpConstInitValAST::getValue(Context& context) const { 
  return const_exp->getValue(context);
}

static void FlattenConstInitVal(const BaseAST& node, Context& context, std::vector<int>& result) {
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

int CompoundConstInitValueAST::getValue(Context& context) const { 
  assert(false);
  return 0;
}

std::vector<int> CompoundConstInitValueAST::getValues(Context& context) const { 
  std::vector<int> init_vals;
  FlattenConstInitVal(*this, context, init_vals);
  return init_vals;
}

int InitValAST::getValue(Context& context) const { 
  return exp->getValue(context);
}

static void FlattenInitValValues(const BaseAST& node, Context& context, std::vector<int>& result) {
  if (const auto* p = dynamic_cast<const InitValAST*>(&node)) {
    result.push_back(p->exp->getValue(context));
  } else if (const auto* p = dynamic_cast<const InitValListAST*>(&node)) {
    for (const auto& child : p->init_vals) {
      FlattenInitValValues(*child, context, result);
    }
  }
}

std::vector<int> InitValListAST::getValues(Context& context) const { 
  std::vector<int> ret;
  FlattenInitValValues(*this, context, ret);
  return ret;
}