#include "ast/ast.hpp"
#include <iostream>
#include <assert.h>
using namespace std;

// CompUnit ::= FuncDef
std::string CompUnitAST::Dump(Context& context) const {
  if (func_def) {
    func_def->Dump(context);
  }
  return "";
}

// FuncDef ::= FuncType IDENT '(' ')' Block
std::string FuncDefAST::Dump(Context& context) const {
  context.reg_builder.Reset(); // 解析新函数，寄存器计数器清零
  context.enterNewScope();

  std::cout << "fun @" << *ident << "(): ";
  func_type->Dump(context);
  std::cout << " {\n";

  if (block) {
    block->Dump(context);
  }

  std::cout << "}\n";
  //context.exitScope();
  return "";
}

// FuncType ::= 'int'
std::string FuncTypeAST::Dump(Context& context) const {
  std::cout << type_name;
  return "";
}

// Block ::= '{' {BlockItem} '}'
std::string BlockAST::Dump(Context& context) const {
  std::cout << "\%entry:\n";
  for (auto& item : block_items) {
    item->Dump(context);
  }
  return "";
}

// BlockItem ::= Decl | Stmt
std::string DeclBloAST::Dump(Context& context) const {
  if (decl) {
    decl->Dump(context);
  }
  return "";
}

std::string StmtBloAST::Dump(Context& context) const {
  if (stmt) {
    stmt->Dump(context);
  }
  return "";
}

// Stmt ::= 'return' Exp ';' | LVal '=' Exp ';'
std::string ReturnStmtAST::Dump(Context& context) const {
  if (exp) {
    std::string ret_val = exp->Dump(context);
    std::cout << "  ret " << ret_val << "\n";
  }
  return "";
}

string AssignStmtAST::Dump(Context& context) const {
  if (lval) {
    // 这里先右后左
    std::string rhs_reg = exp->Dump(context);
    std::string lhs_reg = lval->DumpOnlyAddress(context);
    std::cout << "  store " << rhs_reg << ", " << lhs_reg << "\n";
  }
  return "";
}

// Exp ::= LorExp
std::string ExpAST::Dump(Context& context) const {
  if (lor_exp) {
    return lor_exp->Dump(context); // 透传子节点的寄存器/常量
  }
  return "";
}
int ExpAST::getValue(Context& context) const {
  return lor_exp->getValue(context);
}
// UnaryExp ::= UnaryOP UnaryExp | PrimaryExp
std::string UnaryOPExpAST::Dump(Context& context) const {
  std::string src_reg = unary_exp->Dump(context);
  if (op->op == "+") {
    return src_reg; 
  } 
  else if (op->op == "-") {
    std::string dest_reg = context.reg_builder.NewReg();
    std::cout << "  " << dest_reg << " = sub 0, " << src_reg << "\n";
    return dest_reg; // 向上层返回新寄存器名
  } 
  else if (op->op == "!") {
    std::string dest_reg = context.reg_builder.NewReg();
    std::cout << "  " << dest_reg << " = eq 0, " << src_reg << "\n";
    return dest_reg; // 向上层返回新寄存器名
  }
  return "";
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
string PrimaryUnaryExpAST::Dump(Context& context) const {
  if (primary_exp) {
    return primary_exp->Dump(context);
  }
  return "";
}

int PrimaryUnaryExpAST::getValue(Context& context) const {
  if (primary_exp) {
    return primary_exp->getValue(context);
  }
  return 0;
}

// PrimaryExp ::= '(' Exp ')' | LVal | Number
std::string ThreePriAST::Dump(Context& context) const {
  if (exp) {
    return exp->Dump(context);
  }
  return "";
}

int ThreePriAST::getValue(Context& context) const {
  return exp->getValue(context);
}

std::string NumberPriAST::Dump(Context& context) const {
  return std::to_string(value);
}
int NumberPriAST::getValue(Context& context) const {
  return value;
}

std::string LValPriAST::Dump(Context& context) const {
  return lval->Dump(context);
}

int LValPriAST::getValue(Context& context) const {
  return lval->getValue(context);
}

// '+' | '-' | '!' | '*' | '/' | '%'
std::string OpAST::Dump(Context& context) const {
  return op;
}

// AddExp ::= MulExp | AddExp ('+' | '-') MulExp
string MulAddExpAST::Dump(Context& context) const {
  if (mul_exp) {
    return mul_exp->Dump(context);
  }
  return "";
}
int MulAddExpAST::getValue(Context& context) const {
  if (mul_exp) {
    return mul_exp->getValue(context);
  }
  return 0;
}

string ThreeAddExpAST::Dump(Context& context) const {
  string lhs_reg = add_exp->Dump(context);
  string rhs_reg = mul_exp->Dump(context);
  string dest_reg = context.reg_builder.NewReg();
  if (op->op == "+") {
    std::cout << "  " << dest_reg << " = add " << lhs_reg << ", " << rhs_reg << "\n";
  } else if (op->op == "-") {
    std::cout << "  " << dest_reg << " = sub " << lhs_reg << ", " << rhs_reg << "\n";
  }
  return dest_reg;
}
int ThreeAddExpAST::getValue(Context& context) const {
  if (op->op == "+") {
    return add_exp->getValue(context) + mul_exp->getValue(context);
  } else if (op->op == "-") {
    return add_exp->getValue(context) - mul_exp->getValue(context);
  } else {
    return 0;
  }
}

// MulExp ::= UnaryExp | MulExp ('*' | '/' | '%') UnaryExp
string UnaryMulExpAST::Dump(Context& context) const {
  if (unary_exp) {
    return unary_exp->Dump(context);
  }
  return "";
}

int UnaryMulExpAST::getValue(Context& context) const {
  if (unary_exp) {
    return unary_exp->getValue(context);
  }
  return 0;
}

string ThreeMulExpAST::Dump(Context& context) const {
  string lhs_reg = mul_exp->Dump(context);
  string rhs_reg = unary_exp->Dump(context);
  string dest_reg = context.reg_builder.NewReg();
  if (op->op == "*") {
    std::cout << "  " << dest_reg << " = mul " << lhs_reg << ", " << rhs_reg << "\n";
  } else if (op->op == "/") {
    std::cout << "  " << dest_reg << " = div " << lhs_reg << ", " << rhs_reg << "\n";
  } else if (op->op == "%") {
    std::cout << "  " << dest_reg << " = mod " << lhs_reg << ", " << rhs_reg << "\n";
  }
  return dest_reg;
}

int ThreeMulExpAST::getValue(Context& context) const {
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

// RelExp ::= AddExp | RelExp ('<' | '>' | '<=' | '>=') AddExp
string AddRelExpAST::Dump(Context& context) const {
  if (add_exp) {
    return add_exp->Dump(context);
  }
  return "";
}
int AddRelExpAST::getValue(Context& context) const {
  return add_exp->getValue(context);
}
string ThreeRelExpAST::Dump(Context& context) const {
  string lhs_reg = rel_exp->Dump(context);
  string rhs_reg = add_exp->Dump(context);
  string dest_reg = context.reg_builder.NewReg();
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
int ThreeRelExpAST::getValue(Context& context) const {
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

// EqExp ::= RelExp | EqExp ('==' | '!=') RelExp
string RelEqExpAST::Dump(Context& context) const {
  if (rel_exp) {
    return rel_exp->Dump(context);
  }
  return "";
}
int RelEqExpAST::getValue(Context& context) const {
  return rel_exp->getValue(context);
}

string ThreeEqExpAST::Dump(Context& context) const {
  string lhs_reg = eq_exp->Dump(context);
  string rhs_reg = rel_exp->Dump(context);
  string dest_reg = context.reg_builder.NewReg();
  if (op->op == "==") {
    std::cout << "  " << dest_reg << " = eq " << lhs_reg << ", " << rhs_reg << "\n";
  } else if (op->op == "!=") {
    std::cout << "  " << dest_reg << " = ne " << lhs_reg << ", " << rhs_reg << "\n";
  }
  return dest_reg;
}
int ThreeEqExpAST::getValue(Context& context) const {
  if (op->op == "==") {
    return eq_exp->getValue(context) == rel_exp->getValue(context);
  } else if (op->op == "!=") {
    return eq_exp->getValue(context) != rel_exp->getValue(context);
  } else {
    return 0;
  }
}
// LAndExp ::= EqExp | LAndExp '&&' EqExp
string EqLAndExpAST::Dump(Context& context) const {
  if (eq_exp) {
    return eq_exp->Dump(context);
  }
  return "";
}
int EqLAndExpAST::getValue(Context& context) const {
  return eq_exp->getValue(context);
}

string ThreeLAndExpAST::Dump(Context& context) const {
  string lhs_reg = land_exp->Dump(context);
  string rhs_reg = eq_exp->Dump(context);
  string dest_reg = context.reg_builder.NewReg();
  // 先对两边进行neq处理，得到0/1值，再进行and运算
  if (op->op == "&&") {
    string lhs_reg_new = context.reg_builder.NewReg();
    string rhs_reg_new = context.reg_builder.NewReg();
    std::cout << "  " << lhs_reg_new << " = ne " << lhs_reg << ", 0\n";
    std::cout << "  " << rhs_reg_new << " = ne " << rhs_reg << ", 0\n";
    std::cout << "  " << dest_reg << " = and " << lhs_reg_new << ", " << rhs_reg_new << "\n";
  }
  return dest_reg;
}
int ThreeLAndExpAST::getValue(Context& context) const {
  if (op->op == "&&") {
    return land_exp->getValue(context) && eq_exp->getValue(context);
  } else {
    return 0;
  }
}

// LOrExp ::= LAndExp | LOrExp '||' LAndExp
string LAndLOrExpAST::Dump(Context& context) const {
  if (land_exp) {
    return land_exp->Dump(context);
  }
  return "";
}
int LAndLOrExpAST::getValue(Context& context) const {
  return land_exp->getValue(context);
}
string ThreeLOrExpAST::Dump(Context& context) const {
  string lhs_reg = lor_exp->Dump(context);
  string rhs_reg = land_exp->Dump(context);
  string dest_reg = context.reg_builder.NewReg();
  // 先对两边进行neq处理，得到0/1值，再进行or运算
  if (op->op == "||") {
    string lhs_reg_new = context.reg_builder.NewReg();
    string rhs_reg_new = context.reg_builder.NewReg();
    std::cout << "  " << lhs_reg_new << " = ne " << lhs_reg << ", 0\n";
    std::cout << "  " << rhs_reg_new << " = ne " << rhs_reg << ", 0\n";
    std::cout << "  " << dest_reg << " = or " << lhs_reg_new << ", " << rhs_reg_new << "\n";
  }
  return dest_reg;
}
int ThreeLOrExpAST::getValue(Context& context) const {
  if (op->op == "||") {
    return lor_exp->getValue(context) || land_exp->getValue(context);
  } else {
    return 0;
  }
}

// LVal = IDENT
string LValAST::Dump(Context& context) const { 
  auto sym = context.scope->lookupLocal(*ident);
  if (sym == nullptr) {
    cout << "Error: LVal " << *ident << " not found\n";
    return "";
  }
  if (sym->kind == SymbolKind::VARIABLE) {
    string address = get<string>(sym->value);
    string new_reg = context.reg_builder.NewReg();
    std::cout << "  " << new_reg << " = load " << address << "\n";
    return new_reg;
  } else if (sym->kind == SymbolKind::CONSTANT) {
    return to_string(get<int>(sym->value));
  } else {
    cout << "Error: LVal " << *ident << " is not a variable or constant\n";
    return "";
  }
}

int LValAST::getValue(Context& context) const { 
  auto sym = context.scope->lookupLocal(*ident);
  if (sym == nullptr) {
    cerr << "Error: LVal " << *ident << " not found\n";
    return 0;
  }
  if (sym->kind == SymbolKind::CONSTANT) {
    return get<int>(sym->value);
  } else if (sym->kind == SymbolKind::VARIABLE) {
    cerr << "Error: LVal is a variable " << *ident << "\n";
    return 0;
  } else {
    cerr << "Error: LVal " << *ident << " is not a variable or constant\n";
    return 0;
  }
}

string LValAST::DumpOnlyAddress(Context& context) const { 
  auto sym = context.scope->lookupLocal(*ident);
  if (sym == nullptr) {
    cout << "Error: LVal " << *ident << " not found\n";
    return "";
  }
  if (sym->kind == SymbolKind::VARIABLE) {
    return get<string>(sym->value);
  } else if (sym->kind == SymbolKind::CONSTANT) {
    cerr << "Error: LVal " << *ident << " is a constant\n";
  } else {
    cout << "Error: LVal " << *ident << " is not a variable or constant\n";
  }
  return "";
}

// ConstExp ::= Exp
string ConstExpAST::Dump(Context& context) const { 
  if (exp) {
    return exp->Dump(context);
  }
  return "";
};
int ConstExpAST::getValue(Context& context) const { 
  return exp->getValue(context);
}
// ConstInitVal ::= ConstExp
string ConstInitValAST::Dump(Context& context) const { 
  if (const_exp) {
    return const_exp->Dump(context);
  }
  return "";
};
int ConstInitValAST::getValue(Context& context) const { 
  return const_exp->getValue(context);
}
// ConstDef ::= IDENT '=' ConstInitVal
string ConstDefAST::Dump(Context& context) const { 
  int constinit_val = const_init_val->getValue(context);
  auto sym = make_shared<Symbol>(Symbol(*ident, SymbolKind::CONSTANT, constinit_val));
  if (!context.scope->define(sym)) {
    cout << "Error: redefine " << *ident << "\n";
  }
  return "";
}

// Decl ::= ConstDecl | VarDecl
string ConstDeclDecAST::Dump(Context& context) const {
  if (const_decl) {
    return const_decl->Dump(context);
  }
  return "";
}

string VarDeclDecAST::Dump(Context& context) const { 
  if (var_decl) {
    return var_decl->Dump(context);
  }
  return "";
}

// ConstDecl ::= 'const' BType ConstDef { ',' ConstDef } ';'
string ConstDeclAST::Dump(Context& context) const {
  assert(*b_type == "int");
  const_def_list->Dump(context);
  return "";
}

string ConstDefListAST::Dump(Context& context) const { 
  for (auto& const_def : const_defs) {
    const_def->Dump(context);
  }
  return "";
}

// VarDecl ::= BType VarDef { ',' VarDef } ';'
string VarDeclAST::Dump(Context& context) const {
  assert(*b_type == "int");
  var_def_list->Dump(context);
  return "";
}

string VarDefListAST::Dump(Context& context) const { 
  for (auto& var_def : var_defs) {
    if (!var_def) {
      cerr << "Error: VarDef is nullptr\n";
      continue;
    }
    var_def->Dump(context);
  }
  return "";
}

// VarDef ::= IDENT | IDENT '=' InitVal
string IDENTVarAST::Dump(Context& context) const { 
  string address = context.reg_builder.GetAddress(*ident);
  auto sym = make_shared<Symbol>(Symbol(*ident, SymbolKind::VARIABLE, address));

  std::cout << "  " << address << " = alloc i32\n";

  if (!context.scope->define(sym)) {
    cerr << "Error: redefine " << *ident << "\n";
  }
  return "";
}

string ThreeVarAST::Dump(Context& context) const { 
  string address = context.reg_builder.GetAddress(*ident);
  string init_val_reg = init_val->Dump(context);
  auto sym = make_shared<Symbol>(Symbol(*ident, SymbolKind::VARIABLE, address));
  std::cout << "  " << address << " = alloc i32\n";
  std::cout << "  " << "store " << init_val_reg << ", " << address << "\n";
  if (!context.scope->define(sym)) {
    cerr << "Error: redefine " << *ident << "\n";
  }
  return "";
}

// InitVal ::= Exp
string InitValAST::Dump(Context& context) const { 
  return exp->Dump(context);
}

int InitValAST::getValue(Context& context) const { 
  return exp->getValue(context);
}
