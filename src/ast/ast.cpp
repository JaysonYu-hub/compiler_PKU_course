#include "ast/ast.hpp"
#include <iostream>
#include <assert.h>

// CompUnit ::= FuncDef | CompUnit FuncDef
std::string FuncDefCompUnitAST::Dump(Context& context) const {
  if (func_def) {
    func_def->Dump(context);
  }
  return "";
}

std::string TwoCompUnitAST::Dump(Context& context) const {
  // 注意这里的顺序
  if (comp_unit) {
    comp_unit->Dump(context);
  }
  if (func_def) {
    func_def->Dump(context);
  }
  return "";
}

// FuncDef ::= FuncType IDENT '(' [FuncFParams] ')' Block
std::string FuncDefAST::Dump(Context& context) const {
  context.ResetRegBuilder(); // 解析新函数，寄存器计数器清零
  context.enterNewScope();

  std::cout << "fun @" << *ident << "(";
  if (func_f_params) {
    func_f_params->Dump(context);
  }
  std::cout << ")";

  func_type->Dump(context);
  std::cout << " {\n";
  std::cout << "\%entry:\n";
  context.cur_block_has_terminator = false;

  if (block) {
    block->Dump(context);
  }

  if (!context.cur_block_has_terminator && *ident == "void") {
    std::cout << "  ret\n";
  }

  std::cout << "}\n";
  context.exitScope();
  return "";
}

// FuncFParams ::= | FuncFParam { ',' FuncFParam }
std::string FuncFParamsAST::Dump(Context& context) const {
  if (func_f_params.size() > 0) {
    std::cout << func_f_params[0]->Dump(context);
    for (int i = 1; i < func_f_params.size(); i++) {
      std::cout << ", " << func_f_params[i]->Dump(context);
    }
  }
  return "";
}

// FuncFParam ::= BType Ident ';'
std::string FuncFParamAST::Dump(Context& context) const {
  auto symbol = std::make_shared<Symbol>(Symbol(*ident, SymbolKind::VARIABLE, "@" + *ident));
  context.scope->define(symbol);
  return "@" + *ident + ": " + *btype;
}

// FuncType ::= INT | VOID
std::string FuncTypeAST::Dump(Context& context) const {
  if (type_name == "void") {
    std::cout << "";
  } else {
    std::cout << ": " << type_name;
  }
  return "";
}

// Block ::= '{' {BlockItem} '}'
std::string BlockAST::Dump(Context& context) const {
  context.enterNewScope();
  for (auto& item : block_items) {
    item->Dump(context);
  }
  context.exitScope();
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
  return stmt->Dump(context);
  }
// Stmt ::= MatchedStmt | UnmatchedStmt
std::string MatchedStmtStmtAST::Dump(Context& context) const {
  std::string return_ident = matched_stmt->Dump(context);
  return return_ident;
}

std::string UnmatchedStmtStmtAST::Dump(Context& context) const {
  if (unmatched_stmt) {
    unmatched_stmt->Dump(context);
  }
  return "";
}
// UnmatchedStmt ::= IF '(' Exp ')' MatchedStmt | IF '(' Exp ')' MatchedStmt ELSE UnmatchedStmt
std::string IfUnmatchedStmtAST::Dump(Context& context) const {
  std::string cond_reg = if_exp->Dump(context);
  std::string if_label = LabelBuilder::GetLabel("then", cond_reg);
  std::string else_label = LabelBuilder::GetLabel("else", cond_reg);
  std::string end_label = LabelBuilder::GetLabel("end", cond_reg);
  std::cout << "  br " << cond_reg << ", " << if_label << ", " << end_label << "\n";
  std::cout << if_label << ":\n";
  context.cur_block_has_terminator = false;
  if_stmt->Dump(context);
  if (!context.cur_block_has_terminator) {
    std::cout << "  jump " << end_label << "\n";
  }
  std::cout << end_label << ":\n";
  context.cur_block_has_terminator = false;
  return "";
}

std::string IfElseUnmatchedStmtAST::Dump(Context& context) const {
  std::string cond_reg = if_exp->Dump(context);
  std::string if_label = LabelBuilder::GetLabel("then", cond_reg);
  std::string else_label = LabelBuilder::GetLabel("else", cond_reg);
  std::string end_label = LabelBuilder::GetLabel("end", cond_reg);
  std::cout << "  br " << cond_reg << ", " << if_label << ", " << else_label << "\n";
  std::cout << if_label << ":\n"; 
  // 语句块有返回时，不生成跳转指令
  context.cur_block_has_terminator = false;
  if_stmt->Dump(context);
  bool then_has_terminator = context.cur_block_has_terminator;
  if (!then_has_terminator) {
    std::cout << "  jump " << end_label << "\n";
  }
  std::cout << else_label << ":\n";
  context.cur_block_has_terminator = false;
  else_stmt->Dump(context);
  bool else_has_terminator = context.cur_block_has_terminator;
  if (!else_has_terminator) {
    std::cout << "  jump " << end_label << "\n";
  }
  if (!then_has_terminator || !else_has_terminator) {
    std::cout << end_label << ":\n";
  } else {
    context.cur_block_has_terminator = true;
  }
  return "";
}

/**  MatchedStmt ::= RETURN [Exp] ';' 
 * | LVal '=' Exp ';' 
 * | Exp ';' 
 * | ';' 
 * | Block 
 * | IF '(' Exp ')' MatchedStmt ELSE MatchedStmt
 * | WHILE '(' Exp ')' Stmt
 *  */
std::string ReturnStmtAST::Dump(Context& context) const {
  // 死代码拦截
  if (context.cur_block_has_terminator) return "";
  if (exp) {
    std::string ret_val = exp->Dump(context);
    std::cout << "  ret " << ret_val << "\n";
  } else {
    std::cout << "  ret\n";
  }
  context.cur_block_has_terminator = true;
  return "";
}

std::string AssignStmtAST::Dump(Context& context) const {
  if (context.cur_block_has_terminator) return "";
  if (lval) {
    // 这里先右后左
    std::string rhs_reg = exp->Dump(context);
    std::string lhs_reg = lval->DumpOnlyAddress(context);
    std::cout << "  store " << rhs_reg << ", " << lhs_reg << "\n";
  }
  return "";
}

std::string ExpStmtAST::Dump(Context& context) const {
  if (context.cur_block_has_terminator) return "";
  if (exp) {
    std::string exp_reg = exp->Dump(context);
  }
  return "";
}

std::string EmptyStmtAST::Dump(Context& context) const {
  return "";
}

std::string BlockStmtAST::Dump(Context& context) const {
  if (block) {
    block->Dump(context);
  }
  return "";
}

std::string IfMatchedStmtAST::Dump(Context& context) const {
  if (context.cur_block_has_terminator) return "";
  std::string cond_reg = if_exp->Dump(context);
  std::string if_label = LabelBuilder::GetLabel("then", cond_reg);
  std::string else_label = LabelBuilder::GetLabel("else", cond_reg);
  std::string end_label = LabelBuilder::GetLabel("end", cond_reg);
  std::cout << "  br " << cond_reg << ", " << if_label << ", " << else_label << "\n";
  std::cout << if_label << ":\n";
  // 语句块有返回时，不生成跳转指令
  context.cur_block_has_terminator = false;
  if_stmt->Dump(context);
  bool then_has_terminator = context.cur_block_has_terminator;
  if (!then_has_terminator) { 
    std::cout << "  jump " << end_label << "\n";
  }
  std::cout << else_label << ":\n";
  context.cur_block_has_terminator = false;
  else_stmt->Dump(context);
  bool else_has_terminator = context.cur_block_has_terminator;
  if (!else_has_terminator) {    
    std::cout << "  jump " << end_label << "\n";
  }
  if (!then_has_terminator || !else_has_terminator) {
    std::cout << end_label << ":\n";
  } else {
    context.cur_block_has_terminator = true;
  }
  return "";
}

std::string WhileStmtAST::Dump(Context& context) const {
  if (context.cur_block_has_terminator) return "";
  std::cout << "  // while \n";
  std::string loop_label = LabelBuilder::GetLabel("while_entry");
  std::string continue_loop_label = LabelBuilder::GetLabel("while_body");
  std::string end_label = LabelBuilder::GetLabel("end");
  
  context.enterNewWhileScope(loop_label, continue_loop_label, end_label);
  std::cout << "  jump " << loop_label << "\n";
  std::cout << loop_label << ":\n";
  std::string cond_reg = while_exp->Dump(context);
  std::cout << "  br " << cond_reg << ", " << continue_loop_label << ", " << end_label << "\n";
  std::cout << continue_loop_label << ":\n";
  while_stmt->Dump(context);
  if (!context.cur_block_has_terminator) {
    std::cout << "  jump " << loop_label << "\n";
  }
  std::cout << end_label << ":\n";
  context.cur_block_has_terminator = false;
  context.exitWhileScope();
  return "";
}

std::string BreakStmtAST::Dump(Context& context) const {
  if (context.cur_block_has_terminator) return "";
  WhileLabelContext while_label_context = context.GetWhileLabel();
  std::cout << "  jump " << while_label_context.while_label_end << "\n";
  std::string tool_label = while_label_context.while_label_continue + NumBuilder::GetNum();
  std::cout << tool_label << ":\n";
  return "";
}

std::string ContinueStmtAST::Dump(Context& context) const {
  if (context.cur_block_has_terminator) return "";
  WhileLabelContext while_label_context = context.GetWhileLabel();
  std::cout << "  jump " << while_label_context.while_label_begin << "\n";
  std::string tool_label = while_label_context.while_label_continue + NumBuilder::GetNum();
  std::cout << tool_label << ":\n";
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
    std::string dest_reg = context.NewReg();
    std::cout << "  " << dest_reg << " = sub 0, " << src_reg << "\n";
    return dest_reg; // 向上层返回新寄存器名
  } 
  else if (op->op == "!") {
    std::string dest_reg = context.NewReg();
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
std::string PrimaryUnaryExpAST::Dump(Context& context) const {
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
std::string MulAddExpAST::Dump(Context& context) const {
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

std::string ThreeAddExpAST::Dump(Context& context) const {
  std::string lhs_reg = add_exp->Dump(context);
  std::string rhs_reg = mul_exp->Dump(context);
  std::string dest_reg = context.NewReg();
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
std::string UnaryMulExpAST::Dump(Context& context) const {
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

std::string ThreeMulExpAST::Dump(Context& context) const {
  std::string lhs_reg = mul_exp->Dump(context);
  std::string rhs_reg = unary_exp->Dump(context);
  std::string dest_reg = context.NewReg();
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
std::string AddRelExpAST::Dump(Context& context) const {
  if (add_exp) {
    return add_exp->Dump(context);
  }
  return "";
}
int AddRelExpAST::getValue(Context& context) const {
  return add_exp->getValue(context);
}
std::string ThreeRelExpAST::Dump(Context& context) const {
  std::string lhs_reg = rel_exp->Dump(context);
  std::string rhs_reg = add_exp->Dump(context);
  std::string dest_reg = context.NewReg();
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
std::string RelEqExpAST::Dump(Context& context) const {
  if (rel_exp) {
    return rel_exp->Dump(context);
  }
  return "";
}
int RelEqExpAST::getValue(Context& context) const {
  return rel_exp->getValue(context);
}

std::string ThreeEqExpAST::Dump(Context& context) const {
  std::string lhs_reg = eq_exp->Dump(context);
  std::string rhs_reg = rel_exp->Dump(context);
  std::string dest_reg = context.NewReg();
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
std::string EqLAndExpAST::Dump(Context& context) const {
  if (eq_exp) {
    return eq_exp->Dump(context);
  }
  return "";
} 
int EqLAndExpAST::getValue(Context& context) const {
  return eq_exp->getValue(context);
}

std::string ThreeLAndExpAST::Dump(Context& context) const {
  // 使用跳转实现短路求值
  if (op->op == "&&") {
    std::string lhs_reg = land_exp->Dump(context);
    std::string temp_address = context.NewReg();

    std::cout << "  // " << temp_address << " : " << lhs_reg << " && " << "rhs_reg\n";
    std::string then_label = LabelBuilder::GetLabel("and_calc_rhs");
    std::string end_label = LabelBuilder::GetLabel("and_end");
    std::string temp_reg_1 = context.NewReg();
    std::cout << "  " << temp_address << " = alloc i32\n";
    std::cout << "  " << "store 0, " << temp_address << "\n";
    std::cout << "  br " << lhs_reg << " , " << then_label << " , " << end_label << "\n";
    // then块
    std::cout << then_label << ":\n";
    context.cur_block_has_terminator = false;
    std::string rhs_reg = eq_exp->Dump(context);
    std::string temp_reg = context.NewReg();
    std::cout << "  " <<  temp_reg << " = ne " << rhs_reg << ", 0\n";
    std::cout << "  " << "store " << temp_reg << ", " << temp_address << "\n";
    std::cout << "  jump " << end_label << "\n";
    std::cout << end_label << ":\n";
    context.cur_block_has_terminator = false;

    std::string return_reg = context.NewReg();
    std::cout << "  " << return_reg << " = load " << temp_address << "\n";
    return return_reg;
  }
  return "";
}
int ThreeLAndExpAST::getValue(Context& context) const {
  if (op->op == "&&") {
    return land_exp->getValue(context) && eq_exp->getValue(context);
  } else {
    return 0;
  }
}

// LOrExp ::= LAndExp | LOrExp '||' LAndExp
std::string LAndLOrExpAST::Dump(Context& context) const {
  if (land_exp) {
    return land_exp->Dump(context);
  }
  return "";
}
int LAndLOrExpAST::getValue(Context& context) const {
  return land_exp->getValue(context);
}
std::string ThreeLOrExpAST::Dump(Context& context) const {
  // 使用跳转实现短路求值
  if (op->op == "||") {
    std::string lhs_reg = lor_exp->Dump(context);
    std::string temp_address = context.NewReg();

    std::cout << "  // " << temp_address << " : " << lhs_reg << " || " << "rhs_reg" << "\n";
    std::string then_label = LabelBuilder::GetLabel("or_calc_rhs", temp_address);
    std::string end_label = LabelBuilder::GetLabel("or_end", temp_address);
    std::cout << "  " << temp_address << " = alloc i32\n";
    std::cout << "  " << "store 1, " << temp_address << "\n";
    std::cout << "  " << "br " << lhs_reg <<", " << end_label << ", " << then_label << "\n";
    // then块
    std::cout << then_label << ":\n";
    context.cur_block_has_terminator = false;
    std::string rhs_reg = land_exp->Dump(context);
    std::string temp_reg = context.NewReg();
    std::cout << "  " << temp_reg << " = ne 0, " << rhs_reg << "\n";
    std::cout << "  " << "store " << temp_reg << ", " << temp_address << "\n";
    std::cout << "  jump " << end_label << "\n";
    std::cout << end_label << ":\n";
    context.cur_block_has_terminator = false;

    std::string return_reg = context.NewReg();
    std::cout << "  " << return_reg << " = load " << temp_address << "\n";
    return return_reg;
  }
  return "";
}
int ThreeLOrExpAST::getValue(Context& context) const {
  if (op->op == "||") {
    return lor_exp->getValue(context) || land_exp->getValue(context);
  } else {
    return 0;
  }
}

// LVal = IDENT
std::string LValAST::Dump(Context& context) const { 
  auto sym = context.scope->lookup(*ident);
  if (sym == nullptr) {
    std::cout << "Error: LVal " << *ident << " not found\n";
    return "";
  }
  if (sym->kind == SymbolKind::VARIABLE) {
    std::string address = std::get<std::string>(sym->value);
    std::string new_reg = context.NewReg();
    std::cout << "  " << new_reg << " = load " << address << "\n";
    return new_reg;
  } else if (sym->kind == SymbolKind::CONSTANT) {
    return std::to_string(std::get<int>(sym->value));
  } else {
    std::cout << "Error: LVal " << *ident << " is not a variable or constant\n";
    return "";
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

// ConstExp ::= Exp
std::string ConstExpAST::Dump(Context& context) const { 
  if (exp) {
    return exp->Dump(context);
  }
  return "";
};
int ConstExpAST::getValue(Context& context) const { 
  return exp->getValue(context);
}
// ConstInitVal ::= ConstExp
std::string ConstInitValAST::Dump(Context& context) const { 
  if (const_exp) {
    return const_exp->Dump(context);
  }
  return "";
};
int ConstInitValAST::getValue(Context& context) const { 
  return const_exp->getValue(context);
}
// ConstDef ::= IDENT '=' ConstInitVal
std::string ConstDefAST::Dump(Context& context) const { 
  int constinit_val = const_init_val->getValue(context);
  auto sym = std::make_shared<Symbol>(Symbol(*ident, SymbolKind::CONSTANT, constinit_val));
  if (!context.scope->define(sym)) {
    std::cout << "Error: redefine " << *ident << "\n";
  }
  return "";
}

// Decl ::= ConstDecl | VarDecl
std::string ConstDeclDecAST::Dump(Context& context) const {
  if (const_decl) {
    return const_decl->Dump(context);
  }
  return "";
}

std::string VarDeclDecAST::Dump(Context& context) const { 
  if (var_decl) {
    return var_decl->Dump(context);
  }
  return "";
}

// ConstDecl ::= 'const' BType ConstDef { ',' ConstDef } ';'
std::string ConstDeclAST::Dump(Context& context) const {
  assert(*b_type == "i32");
  const_def_list->Dump(context);
  return "";
}

std::string ConstDefListAST::Dump(Context& context) const { 
  for (auto& const_def : const_defs) {
    const_def->Dump(context);
  }
  return "";
}

// VarDecl ::= BType VarDef { ',' VarDef } ';'
std::string VarDeclAST::Dump(Context& context) const {
  assert(*b_type == "i32");
  var_def_list->Dump(context);
  return "";
}

std::string VarDefListAST::Dump(Context& context) const { 
  for (auto& var_def : var_defs) {
    if (!var_def) {
      std::cerr << "Error: VarDef is nullptr\n";
      continue;
    }
    var_def->Dump(context);
  }
  return "";
}

// VarDef ::= IDENT | IDENT '=' InitVal
std::string IDENTVarAST::Dump(Context& context) const { 
  std::string address = context.GetAddress(*ident);
  auto sym = std::make_shared<Symbol>(Symbol(*ident, SymbolKind::VARIABLE, address));

  std::cout << "  " << address << " = alloc i32\n";

  if (!context.scope->define(sym)) {
    std::cerr << "Error: redefine " << *ident << "\n";
  }
  return "";
}

std::string ThreeVarAST::Dump(Context& context) const { 
  std::string address = context.GetAddress(*ident);
  std::string init_val_reg = init_val->Dump(context);
  auto sym = std::make_shared<Symbol>(Symbol(*ident, SymbolKind::VARIABLE, address));
  std::cout << "  " << address << " = alloc i32\n";
  std::cout << "  " << "store " << init_val_reg << ", " << address << "\n";
  if (!context.scope->define(sym)) {
    std::cerr << "Error: redefine " << *ident << "\n";
  }
  return "";
}

// InitVal ::= Exp
std::string InitValAST::Dump(Context& context) const { 
  return exp->Dump(context);
}

int InitValAST::getValue(Context& context) const { 
  return exp->getValue(context);
}
