#include "ast/ast.hpp"
#include <iostream>
#include <cassert>

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
  } else {
    std::cerr << "Error: LVal " << *ident << " is not a variable or constant\n";
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
  auto sym = context.scope->lookupLocal(*ident);
  if (sym == nullptr) {
    std::cerr << "Error: LVal " << *ident << " not found\n";
    return 0;
  }
  if (sym->kind == SymbolKind::CONSTANT) {
    return std::get<int>(sym->value);
  } else if (sym->kind == SymbolKind::VARIABLE) {
    std::cerr << "Error: LVal is a variable " << *ident << "\n";
    return 0;
  } else {
    std::cerr << "Error: LVal " << *ident << " is not a variable or constant\n";
    return 0;
  }
}

int ArrayLValueAST::getValue(Context& context) const { 
  assert(false);
  return 0;
}

int ConstExpAST::getValue(Context& context) const { 
  return exp->getValue(context);
}

int ConstExpConstInitValAST::getValue(Context& context) const { 
  return const_exp->getValue(context);
}

int CompoundConstInitValueAST::getValue(Context& context) const { 
  assert(false);
  return 0;
}

std::vector<int> CompoundConstInitValueAST::getValues(Context& context) const { 
  std::vector<int> init_vals;
  for (auto& init_val : const_init_val_list->const_init_vals) {
    init_vals.push_back(init_val->getValue(context));
  }
  return init_vals;
}

int InitValAST::getValue(Context& context) const { 
  return exp->getValue(context);
}

int CompoundInitValueAST::getValue(Context& context) const { 
  assert(false);
  return 0;
}

std::vector<int> InitValListAST::getValue(Context& context) const { 
  std::vector<int> ret;
  for (auto& init_val : init_vals) {
    ret.push_back(init_val->getValue(context));
  }
  return ret;
}