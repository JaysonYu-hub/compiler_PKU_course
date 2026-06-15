#include "ast/ast.hpp"
#include <iostream>
#include <assert.h>
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

// Block ::= '{' {BlockItem} '}'
std::string BlockAST::Dump(IRBuilder& builder) const {
  std::cout << "\%entry:\n";
  for (auto& item : block_items) {
    item->Dump(builder);
  }
  return "";
}

// BlockItem ::= Decl | Stmt
std::string DeclBloAST::Dump(IRBuilder& builder) const {
  if (decl) {
    decl->Dump(builder);
  }
  return "";
}

std::string StmtBloAST::Dump(IRBuilder& builder) const {
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

// Exp ::= LorExp
std::string ExpAST::Dump(IRBuilder& builder) const {
  if (lor_exp) {
    return lor_exp->Dump(builder); // 透传子节点的寄存器/常量
  }
  return "";
}
int ExpAST::getValue() const {
  return lor_exp->getValue();
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
int UnaryOPExpAST::getValue() const {
  if (op->op == "+") {
    return unary_exp->getValue();
  } else if (op->op == "-") {
    return -unary_exp->getValue();
  } else if (op->op == "!") {
    return unary_exp->getValue() == 0 ? 1 : 0;
  }
}

// PrimaryExp ::= '(' Exp ')' | LVal | Number
std::string ThreePriAST::Dump(IRBuilder& builder) const {
  if (exp) {
    return exp->Dump(builder);
  }
  return "";
}

int ThreePriAST::getValue() const {
  return exp->getValue();
}

std::string NumberPriAST::Dump(IRBuilder& builder) const {
  return std::to_string(value);
}
int NumberPriAST::getValue() const {
  return value;
}

//TODO
std::string LValPriAST::Dump(IRBuilder& builder) const {

  return *ident;
}

// '+' | '-' | '!' | '*' | '/' | '%'
std::string OpAST::Dump(IRBuilder& builder) const {
  return op;
}

// AddExp ::= MulExp | AddExp ('+' | '-') MulExp
string MulAddExpAST::Dump(IRBuilder& builder) const {
  if (mul_exp) {
    return mul_exp->Dump(builder);
  }
  return "";
}
int MulAddExpAST::getValue() const {
  return mul_exp->getValue();
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
int ThreeAddExpAST::getValue() const {
  if (op->op == "+") {
    return add_exp->getValue() + mul_exp->getValue();
  } else if (op->op == "-") {
    return add_exp->getValue() - mul_exp->getValue();
  }
}

// MulExp ::= UnaryExp | MulExp ('*' | '/' | '%') UnaryExp
string UnaryMulExpAST::Dump(IRBuilder& builder) const {
  if (unary_exp) {
    return unary_exp->Dump(builder);
  }
  return "";
}

int UnaryMulExpAST::getValue() const {
  return unary_exp->getValue();
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
    std::cout << "  " << dest_reg << " = mod " << lhs_reg << ", " << rhs_reg << "\n";
  }
  return dest_reg;
}

int ThreeMulExpAST::getValue() const {
  if (op->op == "*") {
    return mul_exp->getValue() * unary_exp->getValue();
  } else if (op->op == "/") {
    return mul_exp->getValue() / unary_exp->getValue();
  } else if (op->op == "%") {
    return mul_exp->getValue() % unary_exp->getValue();
  }
}

// RelExp ::= AddExp | RelExp ('<' | '>' | '<=' | '>=') AddExp
string AddRelExpAST::Dump(IRBuilder& builder) const {
  if (add_exp) {
    return add_exp->Dump(builder);
  }
  return "";
}
int AddRelExpAST::getValue() const {
  return add_exp->getValue();
}
string ThreeRelExpAST::Dump(IRBuilder& builder) const {
  string lhs_reg = rel_exp->Dump(builder);
  string rhs_reg = add_exp->Dump(builder);
  string dest_reg = builder.NewReg();
  if (op->op == "<") {
    std::cout << "  " << dest_reg << " = lt " << lhs_reg << ", " << rhs_reg << "\n";
  } else if (op->op == ">") {
    std::cout << "  " << dest_reg << " = gt " << lhs_reg << ", " << rhs_reg << "\n";
  } else if (op->op == "<=") {
    std::cout << "  " << dest_reg << " = le " << lhs_reg << ", " << rhs_reg << "\n";
  } else if (op->op == ">=") {
    std::cout << "  " << dest_reg << " = ge " << lhs_reg << ", " << rhs_reg << "\n";
  }
  return dest_reg;
}
int ThreeRelExpAST::getValue() const {
  if (op->op == "<") {
    return rel_exp->getValue() < add_exp->getValue();
  } else if (op->op == ">") {
    return rel_exp->getValue() > add_exp->getValue();
  } else if (op->op == "<=") {
    return rel_exp->getValue() <= add_exp->getValue();
  } else if (op->op == ">=") {
    return rel_exp->getValue() >= add_exp->getValue();
  }
}

// EqExp ::= RelExp | EqExp ('==' | '!=') RelExp
string RelEqExpAST::Dump(IRBuilder& builder) const {
  if (rel_exp) {
    return rel_exp->Dump(builder);
  }
  return "";
}
int RelEqExpAST::getValue() const {
  return rel_exp->getValue();
}

string ThreeEqExpAST::Dump(IRBuilder& builder) const {
  string lhs_reg = eq_exp->Dump(builder);
  string rhs_reg = rel_exp->Dump(builder);
  string dest_reg = builder.NewReg();
  if (op->op == "==") {
    std::cout << "  " << dest_reg << " = eq " << lhs_reg << ", " << rhs_reg << "\n";
  } else if (op->op == "!=") {
    std::cout << "  " << dest_reg << " = ne " << lhs_reg << ", " << rhs_reg << "\n";
  }
  return dest_reg;
}
int ThreeEqExpAST::getValue() const {
  if (op->op == "==") {
    return eq_exp->getValue() == rel_exp->getValue();
  } else if (op->op == "!=") {
    return eq_exp->getValue() != rel_exp->getValue();
  }
}
// LAndExp ::= EqExp | LAndExp '&&' EqExp
string EqLAndExpAST::Dump(IRBuilder& builder) const {
  if (eq_exp) {
    return eq_exp->Dump(builder);
  }
  return "";
}
int EqLAndExpAST::getValue() const {
  return eq_exp->getValue();
}

string ThreeLAndExpAST::Dump(IRBuilder& builder) const {
  string lhs_reg = land_exp->Dump(builder);
  string rhs_reg = eq_exp->Dump(builder);
  string dest_reg = builder.NewReg();
  // 先对两边进行neq处理，得到0/1值，再进行and运算
  if (op->op == "&&") {
    string lhs_reg_new = builder.NewReg();
    string rhs_reg_new = builder.NewReg();
    std::cout << "  " << lhs_reg_new << " = ne " << lhs_reg << ", 0\n";
    std::cout << "  " << rhs_reg_new << " = ne " << rhs_reg << ", 0\n";
    std::cout << "  " << dest_reg << " = and " << lhs_reg_new << ", " << rhs_reg_new << "\n";
  }
  return dest_reg;
}
int ThreeLAndExpAST::getValue() const {
  if (op->op == "&&") {
    return land_exp->getValue() && eq_exp->getValue();
  }
}

// LOrExp ::= LAndExp | LOrExp '||' LAndExp
string LAndLOrExpAST::Dump(IRBuilder& builder) const {
  if (land_exp) {
    return land_exp->Dump(builder);
  }
  return "";
}
int LAndLOrExpAST::getValue() const {
  return land_exp->getValue();
}
string ThreeLOrExpAST::Dump(IRBuilder& builder) const {
  string lhs_reg = lor_exp->Dump(builder);
  string rhs_reg = land_exp->Dump(builder);
  string dest_reg = builder.NewReg();
  // 先对两边进行neq处理，得到0/1值，再进行or运算
  if (op->op == "||") {
    string lhs_reg_new = builder.NewReg();
    string rhs_reg_new = builder.NewReg();
    std::cout << "  " << lhs_reg_new << " = ne " << lhs_reg << ", 0\n";
    std::cout << "  " << rhs_reg_new << " = ne " << rhs_reg << ", 0\n";
    std::cout << "  " << dest_reg << " = or " << lhs_reg_new << ", " << rhs_reg_new << "\n";
  }
  return dest_reg;
}
int ThreeLOrExpAST::getValue() const {
  if (op->op == "||") {
    return lor_exp->getValue() || land_exp->getValue();
  }
}
// ConstExp ::= Exp
string ConstExpAST::Dump(IRBuilder& builder) const { 
  if (exp) {
    return exp->Dump(builder);
  }
  return "";
};
int ConstExpAST::getValue() const { 
  return exp->getValue();
}
// ConstInitVal ::= ConstExp
string ConstInitValAST::Dump(IRBuilder& builder) const { 
  if (const_exp) {
    return const_exp->Dump(builder);
  }
  return "";
};
int ConstInitValAST::getValue() const { 
  return const_exp->getValue();
}
// ConstDef ::= IDENT '=' ConstInitVal
// TODO
string ConstDefAST::Dump(IRBuilder& builder) const { 
  string constinit_val = const_init_val->Dump(builder);
  int val = std::stoi(constinit_val);
  CompilerState::GetInstance().set_const_var(*ident, val);
  return "";
};

// Decl ::= ConstDecl
string DeclAST::Dump(IRBuilder& builder) const {
  if (const_decl) {
    return const_decl->Dump(builder);
  }
  return "";
}

// ConstDecl ::= 'const' BType ConstDef { ',' ConstDef } ';'
//TODO
string ConstDeclAST::Dump(IRBuilder& builder) const {
  assert(*b_type == "int");
  const_def_list->Dump(builder);
  return "";
}

string ConstDefListAST::Dump(IRBuilder& builder) const { 
  for (auto& const_def : const_defs) {
    const_def->Dump(builder);
  }
  return "";
}