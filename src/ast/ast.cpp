#include "ast/ast.hpp"
#include <iostream>
using namespace std;

// CompUnit ::= FuncDef
std::string CompUnitAST::Dump(IRBuilder& builder) const {
  if (func_def) {
    func_def->Dump(builder);
  }
  return "";
}

// FuncDef ::= FuncType IDENT '(' ')' Block
std::string FuncDefAST::Dump(IRBuilder& builder) const {
  builder.Reset(); // 解析新函数，寄存器计数器清零

  std::cout << "fun @" << *ident << "(): ";
  func_type->Dump(builder);
  std::cout << " {\n";

  if (block) {
    block->Dump(builder);
}

  std::cout << "}\n";
  return "";
}

// FuncType ::= 'int'
std::string FuncTypeAST::Dump(IRBuilder& builder) const {
  std::cout << type_name;
  return "";
}

// Block ::= '{' Stmt '}'
std::string BlockAST::Dump(IRBuilder& builder) const {
  std::cout << "\%entry:\n";
  if (stmt) {
    stmt->Dump(builder);
  }
  return "";
}

// Stmt ::= 'return' Exp ';'
std::string StmtAST::Dump(IRBuilder& builder) const {
  if (exp) {
    std::string ret_val = exp->Dump(builder);
    std::cout << "  ret " << ret_val << "\n";
  }
  return "";
}

// Exp ::= AddExp
std::string ExpAST::Dump(IRBuilder& builder) const {
  if (add_exp) {
    return add_exp->Dump(builder); // 透传子节点的寄存器/常量
  }
  return "";
}

// UnaryExp ::= UnaryOP UnaryExp
std::string UnaryOPExpAST::Dump(IRBuilder& builder) const {
  std::string src_reg = unary_exp->Dump(builder);
  if (op->op == "+") {
    return src_reg; 
  } 
  else if (op->op == "-") {
    std::string dest_reg = builder.NewReg();
    std::cout << "  " << dest_reg << " = sub 0, " << src_reg << "\n";
    return dest_reg; // 向上层返回新寄存器名
  } 
  else if (op->op == "!") {
    std::string dest_reg = builder.NewReg();
    std::cout << "  " << dest_reg << " = eq 0, " << src_reg << "\n";
    return dest_reg; // 向上层返回新寄存器名
  }
  return "";
}

// PrimaryExp ::= '(' Exp ')'
std::string ParenExprAST::Dump(IRBuilder& builder) const {
  if (exp) {
    return exp->Dump(builder);
  }
  return "";
}

// PrimaryExp ::= Number
std::string NumberExpAST::Dump(IRBuilder& builder) const {
  return std::to_string(value);
}

// '+' | '-' | '!' | '*' | '/' | '%'
std::string OpAST::Dump(IRBuilder& builder) const {
  return op;
}

string MulAddExpAST::Dump(IRBuilder& builder) const {
  if (mul_exp) {
    return mul_exp->Dump(builder);
  }
  return "";
}

string ThreeAddExpAST::Dump(IRBuilder& builder) const {
  string lhs_reg = add_exp->Dump(builder);
  string rhs_reg = mul_exp->Dump(builder);
  string dest_reg = builder.NewReg();
  if (op->op == "+") {
    std::cout << "  " << dest_reg << " = add " << lhs_reg << ", " << rhs_reg << "\n";
  } else if (op->op == "-") {
    std::cout << "  " << dest_reg << " = sub " << lhs_reg << ", " << rhs_reg << "\n";
  }
  return dest_reg;
}

string UnaryMulExpAST::Dump(IRBuilder& builder) const {
  if (unary_exp) {
    return unary_exp->Dump(builder);
  }
  return "";
}

string ThreeMulExpAST::Dump(IRBuilder& builder) const {
  string lhs_reg = mul_exp->Dump(builder);
  string rhs_reg = unary_exp->Dump(builder);
  string dest_reg = builder.NewReg();
  if (op->op == "*") {
    std::cout << "  " << dest_reg << " = mul " << lhs_reg << ", " << rhs_reg << "\n";
  } else if (op->op == "/") {
    std::cout << "  " << dest_reg << " = div " << lhs_reg << ", " << rhs_reg << "\n";
  } else if (op->op == "%") {
    std::cout << "  " << dest_reg << " = rem " << lhs_reg << ", " << rhs_reg << "\n";
  }
  return dest_reg;
}

