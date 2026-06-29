#include "ast/ast.hpp"
#include "ast/codegen.hpp"
#include <iostream>
#include <cassert>

// CompUnit ::= end_comp_unit
std::string CompUnitAST::Dump(Context& context) const { 
  if (end_comp_unit) {
    auto library_decl = R"(decl @getint(): i32
decl @getch(): i32
decl @getarray(*i32): i32
decl @putint(i32)
decl @putch(i32)
decl @putarray(i32, *i32)
decl @starttime()
decl @stoptime()

)";
    std::cout << library_decl;
    auto symbol =  std::make_shared<Symbol>(Symbol("getint", SymbolKind::FUNCTION, FuncInformation("i32")));
    context.scope->define(symbol);
    symbol =  std::make_shared<Symbol>(Symbol("getch", SymbolKind::FUNCTION, FuncInformation("i32")));
    context.scope->define(symbol);
    symbol =  std::make_shared<Symbol>(Symbol("getarray", SymbolKind::FUNCTION, FuncInformation("i32")));
    context.scope->define(symbol);
    symbol =  std::make_shared<Symbol>(Symbol("putint", SymbolKind::FUNCTION, FuncInformation("void")));
    context.scope->define(symbol);
    symbol =  std::make_shared<Symbol>(Symbol("putch", SymbolKind::FUNCTION, FuncInformation("void")));
    context.scope->define(symbol);
    symbol =  std::make_shared<Symbol>(Symbol("putarray", SymbolKind::FUNCTION, FuncInformation("void")));
    context.scope->define(symbol);
    symbol =  std::make_shared<Symbol>(Symbol("starttime", SymbolKind::FUNCTION, FuncInformation("void")));
    context.scope->define(symbol);
    symbol =  std::make_shared<Symbol>(Symbol("stoptime", SymbolKind::FUNCTION, FuncInformation("void")));
    context.scope->define(symbol);
    end_comp_unit->Dump(context);
  }
  return "";
}
// end_comp_unit ::= comp_unit | end_comp_unit comp_unit
std::string CompUnitSequenceAST::Dump(Context& context) const {
  if (comp_unit) {
    comp_unit->Dump(context);
  }
  return "";
}

std::string CompUnitListAST::Dump(Context& context) const {
  // 这里是有先后顺序的
  if (end_comp_unit) {
    end_comp_unit->Dump(context);
  }
  if (comp_unit) {
    comp_unit->Dump(context);
  }
  return "";
}

// comp_unit ::= Decl | FuncDef
std::string FuncDefCompUnitAST::Dump(Context& context) const {
  if (func_def) {
    func_def->Dump(context);
  }
  return "";
}

std::string DeclCompUnitAST::Dump(Context& context) const {
  if (decl) {
    decl->Dump(context);
  }
  return "";
}

// FuncDef ::= FuncType IDENT '(' [FuncFParams] ')' Block
std::string FuncDefAST::Dump(Context& context) const {
  context.ResetRegBuilder(); // 解析新函数，寄存器计数器清零
  // 这里注意函数名表所在的定义域
  auto symbol =  std::make_shared<Symbol>(Symbol(*ident, SymbolKind::FUNCTION, FuncInformation(*type)));
  context.scope->define(symbol);

  context.enterNewScope();
  std::cout << "fun @" << *ident << "(";
  if (func_f_params) {
    func_f_params->Dump(context);
  }
  std::cout << ")";
  if (*type == "void") {;}
  else { std::cout << " : " << *type; }
  // 把函数的返回值信息放到context中
  std::cout << " {\n";
  std::cout << "\%entry:\n";
  context.cur_block_has_terminator = false;
  if (func_f_params) {
    func_f_params->ParamLocalization(context);
  }

  if (block) {
    block->Dump(context);
  }
  
  if (!context.cur_block_has_terminator && *type == "void") {
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

void FuncFParamsAST::ParamLocalization(Context& context) {
  for (auto& param : func_f_params) {
    param->ParamLocalization(context);
  }
}

// FuncFParam ::= BType Ident ';'
std::string FuncFParamAST::Dump(Context& context) const {
  return "@" + *ident + ": " + *type;
}

void FuncFParamAST::ParamLocalization(Context& context) {
  std::string address = context.GetAddress(*ident);
  auto symbol = std::make_shared<Symbol>(Symbol(*ident, SymbolKind::VARIABLE, address));
  context.scope->define(symbol);
  std::cout << "  " << address << " = " << "alloc " << *type << "\n";
  std::cout << "  store " << "@" + *ident << ", " << address << "\n";
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
std::string DeclBlockAST::Dump(Context& context) const {
  if (decl) {
    decl->Dump(context);
  }
  return "";
}

std::string StmtBlockAST::Dump(Context& context) const {
  return stmt->Dump(context);
  }
// Stmt ::= MatchedStmt | UnmatchedStmt
std::string MatchedStmtWrapperAST::Dump(Context& context) const {
  std::string return_ident = matched_stmt->Dump(context);
  return return_ident;
}

std::string UnmatchedStmtWrapperAST::Dump(Context& context) const {
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
// UnaryExp ::= UnaryOP UnaryExp | PrimaryExp | IDENT '(' [FuncRParams] ')'
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

std::string FuncCallExpAST::Dump(Context& context) const {
  std::string func_name = *ident;
  // 这里要先查一下是否有返回值
  auto func_symbol = context.scope->lookup(func_name);
  if (func_symbol == nullptr) {
    std::cerr << "func " << func_name << " not found" << "\n";
  }
  auto func_inf = std::get<FuncInformation>(func_symbol->value);
  std::string param;
  if (func_r_params) {
    param = func_r_params->Dump(context);
  } else {
    param = "";
  }
  if (func_inf.ret_type != "void") {
    std::string func_reg = context.NewReg();
      std::cout << "  " << func_reg << " = call @" << func_name << "(" << param << ")\n";
    return func_reg;
  } else { 
      std::cout << "  call @" << func_name << "(" << param << ")" << "\n";
    return "";
  }
}

int FuncCallExpAST::getValue(Context& context) const {
  std::cerr << "func can't getvalue before start" << "\n";
  return 0;
}
// FuncRParams ::= Exp { ',' Exp }
std::string FuncRParamsAST::Dump(Context& context) const {
  std::string params;
  if (func_r_params.size() > 0) {
    params += func_r_params[0]->Dump(context);
    for (int i = 1; i < func_r_params.size(); i++) {
      params = params + ", " + func_r_params[i]->Dump(context);
    }
  }
  return params;
}

// PrimaryExp ::= '(' Exp ')' | LVal | Number
std::string ParenthesizedExprAST::Dump(Context& context) const {
  if (exp) {
    return exp->Dump(context);
  }
  return "";
}

int ParenthesizedExprAST::getValue(Context& context) const {
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

std::string BinaryAddExprAST::Dump(Context& context) const {
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
int BinaryAddExprAST::getValue(Context& context) const {
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

std::string BinaryMulExprAST::Dump(Context& context) const {
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
std::string BinaryRelExprAST::Dump(Context& context) const {
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

std::string BinaryEqExprAST::Dump(Context& context) const {
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
int BinaryEqExprAST::getValue(Context& context) const {
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

std::string BinaryLogicalAndExprAST::Dump(Context& context) const {
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
int BinaryLogicalAndExprAST::getValue(Context& context) const {
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
std::string BinaryLogicalOrExprAST::Dump(Context& context) const {
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
int BinaryLogicalOrExprAST::getValue(Context& context) const {
  if (op->op == "||") {
    return lor_exp->getValue(context) || land_exp->getValue(context);
  } else {
    return 0;
  }
}

// LVal = IDENT | IDENT '[' Exp ']'
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

// LVal = IDENT | # IDENT '[' Exp ']' 
std::string ArrayLValueAST::Dump(Context& context) const { 
  std::shared_ptr<Symbol> sym = context.scope->lookup(*ident);
  if (!sym) {
    std::cerr << "Error: LVal " << *ident << " not found\n";
    return "";
  }
  assert(sym->kind == SymbolKind::ARRAY);
  auto temp = context.NewReg();
  auto index = exp->Dump(context);
  auto result = context.NewReg();
  std::cout << "  " << temp << " = getelementptr " << std::get<ArrayInformation>(sym->value).address << ", " << index << "\n";
  std::cout << "  " << result << " = load " << temp << "\n";
  return result;
}

int ArrayLValueAST::getValue(Context& context) const { 
  assert(false);
  return 0;
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
// ConstInitVal ::= ConstExp | '{' ConstInitVal { ',' ConstInitVal } '}' | '{' '}'
std::string ConstExpConstInitValAST::Dump(Context& context) const { 
  if (const_exp) {
    return const_exp->Dump(context);
  }
  return "";
};

int ConstExpConstInitValAST::getValue(Context& context) const { 
  return const_exp->getValue(context);
}

std::string CompoundConstInitValueAST::Dump(Context& context) const { 
  return const_init_val_list->Dump(context);
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

std::string ConstInitValListAST::Dump(Context& context) const { 
  return "";
}

// ConstDef ::= IDENT '=' ConstInitVal | IDENT '[' ConstExp ']' '=' ConstInitVal
std::string ConstDefAST::Dump(Context& context) const { 
  int constinit_val = const_init_val->getValue(context);
  auto sym = std::make_shared<Symbol>(Symbol(*ident, SymbolKind::CONSTANT, constinit_val));
  context.scope->define(sym);
  return "";
}

std::string ArrayConstDefAST::Dump(Context& context) const { 
  auto raw_init_vals = const_init_val->getValues(context);
  auto arr_size = const_exp->getValue(context);
  auto address = context.GetAddress(*ident);
  ArrayInformation arr_info(address, arr_size);
  auto sym = std::make_shared<Symbol>(Symbol(address, SymbolKind::CONST_ARRAY));
  sym->value = arr_info;

  auto init_vals = std::vector<std::string>(arr_size);
  for (int i = 0; i < raw_init_vals.size(); i++) {
    init_vals[i] = std::to_string(raw_init_vals[i]);
  }
  for (int i = raw_init_vals.size(); i < arr_size; i++) {
    init_vals[i] = "0";
  }
  if (context.scope->isGlobal()) {
    std::string str;
    for (int i = 0; i < arr_size; i++) {
      str += init_vals[i];
      if (i != arr_size - 1) {
        str += ", ";
      }
    }
    std::cout << "global " << address << " = alloc [i32, " << arr_size << "], {" << str << "}\n";
  } else {
    std::cout << "  " << address  << " = alloc [i32, " << arr_size << "]\n";
    // 对数组进行初始化
    for (int i = 0; i < arr_size; i++) {
      auto temp = context.NewReg();
      std::cout << "  " << temp << " = " << "getelementptr " << address << ", " << i << "\n";
      std::cout << "  store " << init_vals[i] << ", " << temp << "\n";
    }
  }
  context.scope->define(sym);
  return "";
}

// Decl ::= ConstDecl | VarDecl
std::string ConstDeclWrapperAST::Dump(Context& context) const {
  if (const_decl) {
    return const_decl->Dump(context);
  }
  return "";
}

std::string VarDeclWrapperAST::Dump(Context& context) const { 
  if (var_decl) {
    return var_decl->Dump(context);
  }
  return "";
}

// ConstDecl ::= 'const' BType ConstDef { ',' ConstDef } ';'
std::string ConstDeclAST::Dump(Context& context) const {
  assert(*type == "i32");
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
  context.decl_type_context = std::make_shared<std::string>(*type);
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

// VarDef ::= IDENT | IDENT '=' InitVal | IDENT '[' ConstExp ']' | IDENT '[' ConstExp ']' '=' InitVal
std::string SimpleVarDefAST::Dump(Context& context) const { 
  std::string address = context.GetAddress(*ident);
  auto sym = std::make_shared<Symbol>(Symbol(*ident, SymbolKind::VARIABLE, address));
  std::string b_type = *context.decl_type_context;

  if (context.scope->isGlobal()) {
    std::cout << "global " << address << " = alloc " << b_type << ", " << "zeroinit\n";
  } else {
    std::cout << "  " << address << " = alloc " << b_type << "\n";
  }
  context.scope->define(sym);
  return "";
}

std::string InitializedVarDefAST::Dump(Context& context) const { 
  std::string address = context.GetAddress(*ident);
  std::string b_type = *context.decl_type_context;
  auto sym = std::make_shared<Symbol>(Symbol(*ident, SymbolKind::VARIABLE, address));
  if (context.scope->isGlobal()) {
    std::cout << "global " << address << " = alloc " << b_type << ", " << init_val->getValue(context) << "\n";
  } else {
    std::cout << "  " << address << " = alloc " << b_type << "\n";
    std::string init_val_reg = init_val->Dump(context);
    std::cout << "  " << "store " << init_val_reg << ", " << address << "\n";
  }
  context.scope->define(sym);
  return "";
}

std::string ArrayVarDefAST::Dump(Context& context) const { 
  std::string address = context.GetAddress(*ident);
  std::string b_type = *context.decl_type_context;
  auto arr_size = const_exp->getValue(context);
  auto sym = std::make_shared<Symbol>(Symbol(address, SymbolKind::ARRAY));
  sym->value = ArrayInformation(address, arr_size);

  if (context.scope->isGlobal()) {
    std::cout << "global " << address << " = alloc [" << b_type << ", " << arr_size << "]\n";
  } else {
    std::cout << "  " << address << " = alloc [" << b_type << ", " << arr_size << "]\n";
  }
  context.scope->define(sym);
  return "";
}

std::vector<std::string> ArrayInitializedVarDefAST::DumpList(Context& context) const { 
  std::vector<std::string> ret;
  for (auto& init_val : init_val->init_val_list->init_vals) {
     ret.push_back(init_val->Dump(context));
  }
  return ret;
}

std::string ArrayInitializedVarDefAST::Dump(Context& context) const { 
  std::string address = context.GetAddress(*ident);
  std::string b_type = *context.decl_type_context;
  auto arr_size = const_exp->getValue(context);
  auto sym = std::make_shared<Symbol>(Symbol(*ident, SymbolKind::ARRAY));
  sym->value = ArrayInformation(b_type, arr_size);
  if (context.scope->isGlobal()) {
    auto raw_init_vals = init_val->init_val_list->getValue(context);
    auto init_vals = std::vector<std::string>(arr_size);
    for (int i = 0; i < raw_init_vals.size(); i++) {
      init_vals[i] = std::to_string(raw_init_vals[i]);
    }
    for (int i = raw_init_vals.size(); i < arr_size; i++) {
      init_vals[i] = "0";
    }
    std::string str;
    for (int i = 0; i < arr_size; i++) {
      str += init_vals[i];
      if (i != arr_size - 1) {
        str += ", ";
      }
    }
    std::cout << "global " << address << " = alloc [i32, " << arr_size << "], {" << str << "}\n";
  } else {
    auto init_vals = DumpList(context);
    for (int i = init_vals.size(); i < arr_size; i++) {
      init_vals.push_back("0");
    }
    std::cout << "  " << address  << " = alloc [i32, " << arr_size << "]\n";
    // 对数组进行初始化
    for (int i = 0; i < arr_size; i++) {
      auto temp = context.NewReg();
      std::cout << "  " << temp << " = " << "getelementptr " << address << ", " << i << "\n";
      std::cout << "  store " << init_vals[i] << ", " << temp << "\n";
    }
  }
  context.scope->define(sym);
  return "";
}

// InitVal ::= Exp | '{' Exp { ',' Exp } '}'
std::string InitValAST::Dump(Context& context) const { 
  return exp->Dump(context);
}

int InitValAST::getValue(Context& context) const { 
  return exp->getValue(context);
}

std::string CompoundInitValueAST::Dump(Context& context) const { 
  return "";
}

int CompoundInitValueAST::getValue(Context& context) const { 
  assert(false);
  return 0;
}

std::string InitValListAST::Dump(Context& context) const { 
  return "";
}

std::vector<int> InitValListAST::getValue(Context& context) const { 
  std::vector<int> ret;
  for (auto& init_val : init_vals) {
    ret.push_back(init_val->getValue(context));
  }
  return ret;
}