%define parse.error verbose
%code requires {
  #include <memory>
  #include <string>
  #include "ast/ast.hpp"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include "ast/ast.hpp"
#include <vector>
#include <assert.h>

int yylex();
extern int yylineno;
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

%parse-param { std::unique_ptr<BaseAST> &ast }

%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
  ValueAST *value_ast;
  OpAST *op_ast;
}

%token INT RETURN VOID
%token CONST WHILE BREAK CONTINUE
%token <str_val> IDENT
%token <int_val> INT_CONST

%token ADD SUB NOT MUL MOD DIV
%token <str_val> ASSIGN
%token LT LE GT GE EQ NE L_AND L_OR IF ELSE

%type <ast_val> comp_unit end_comp_unit FuncDef FuncFParam FuncFParams FuncRParams Block Stmt UnmatchedStmt MatchedStmt
%type <value_ast> Exp UnaryExp PrimaryExp AddExp ConstExp
%type <value_ast> RelExp EqExp LAndExp LOrExp MulExp InitVal LVal
%type <op_ast> ComOp EquOp LOrOp LAndOp UnaryOp AddOp MulOp
%type <int_val> Number
%type <ast_val> Decl ConstDecl VarDecl BlockItem block_item_list const_def_list var_def_list initval_exp_list const_exp_list ConstDef VarDef
/* 这里constinival只所以使用base而不是value是为了兼容array */
%type <ast_val> ConstInitVal
%type <str_val> FuncType Type

%%

CompUnit
  : end_comp_unit {
    auto comp_unit = new CompUnitAST();
    comp_unit->end_comp_unit = unique_ptr<BaseAST>($1);
    ast = unique_ptr<BaseAST>(comp_unit);
  }
  ;

end_comp_unit
  : comp_unit {
    auto start = new CompEndCompUnitAST();
    start->comp_unit = unique_ptr<BaseAST>($1);
    $$ = start;
  }
  | end_comp_unit comp_unit { 
    auto start = new TwoEndCompUnitAST();
    start->end_comp_unit = unique_ptr<BaseAST>($1);
    start->comp_unit = unique_ptr<BaseAST>($2);
    $$ = start;
  }
  ;

comp_unit 
  : FuncDef { 
    auto comp_unit = new FuncDefCompUnitAST();
    comp_unit->func_def = unique_ptr<BaseAST>($1);
    $$ = comp_unit;
  }
  | Decl { 
    auto comp_unit = new DeclCompUnitAST();
    comp_unit->decl = unique_ptr<BaseAST>($1);
    $$ = comp_unit;
  }
  ;

FuncDef
  : Type IDENT '(' FuncFParams ')' Block {
    auto func_def = new FuncDefAST();
    func_def->type = unique_ptr<string>($1);
    func_def->ident = unique_ptr<string>($2);
    func_def->func_f_params = unique_ptr<FuncFParamsAST>(static_cast<FuncFParamsAST*>($4));
    func_def->block = unique_ptr<BaseAST>($6);
    $$ = func_def;
  }
  | Type IDENT '(' ')' Block { 
    auto func_def = new FuncDefAST();
    func_def->type = unique_ptr<string>($1);
    func_def->ident = unique_ptr<string>($2);
    func_def->func_f_params = nullptr;
    func_def->block = unique_ptr<BaseAST>($5);
    $$ = func_def;
  }
  ;

FuncFParams
  : FuncFParam { 
    auto func_f_params = new FuncFParamsAST();
    func_f_params->func_f_params.push_back(unique_ptr<FuncFParamAST>(static_cast<FuncFParamAST*>($1)));
    $$ = func_f_params;
  }
  | FuncFParams ',' FuncFParam { 
    auto func_f_params = (static_cast<FuncFParamsAST*>($1));
    func_f_params->func_f_params.push_back(unique_ptr<FuncFParamAST>(static_cast<FuncFParamAST*>($3)));
    $$ = func_f_params;
  }
  ;

FuncFParam
  : Type IDENT { 
    auto func_f_param = new FuncFParamAST();
    func_f_param->type = unique_ptr<string>($1);
    func_f_param->ident = unique_ptr<string>($2);
    $$ = func_f_param;
  }
  ;

Block 
  : '{' block_item_list '}' { 
    $$ = $2;
  }
  ;

block_item_list 
  : { 
    auto block_ast = new BlockAST(); 
    block_ast->block_items.clear(); 
    $$ = block_ast; 
  }
  | block_item_list BlockItem { 
    auto block_ast = (static_cast<BlockAST*>($1));
    block_ast->block_items.push_back(unique_ptr<BaseAST>($2)); 
    $$ = block_ast; 
  }
  ;

BlockItem
  : Stmt {
    auto stmt_blo = new StmtBloAST();
    stmt_blo->stmt = unique_ptr<BaseAST>($1);
    $$ = stmt_blo;
  }
  | Decl { 
    auto decl_blo = new DeclBloAST();
    decl_blo->decl = unique_ptr<BaseAST>($1);
    $$ = decl_blo;
   }
  ;

Stmt
  : MatchedStmt { 
    auto stmt = new MatchedStmtStmtAST();
    stmt->matched_stmt = unique_ptr<BaseAST>($1);
    $$ = stmt;
  }
  | UnmatchedStmt { 
    auto stmt = new UnmatchedStmtStmtAST();
    stmt->unmatched_stmt = unique_ptr<BaseAST>($1);
    $$ = stmt;
  }
  ;


MatchedStmt
  : RETURN Exp ';' {
    auto stmt = new ReturnStmtAST();
    stmt->exp = unique_ptr<ValueAST>($2);
    $$ = stmt;
  }
  | RETURN ';' { 
    auto stmt = new ReturnStmtAST();
    stmt->exp = nullptr;
    $$ = stmt;
  }
  | LVal ASSIGN Exp ';' { 
    auto assign_stmt = new AssignStmtAST();
    assign_stmt->lval = unique_ptr<LValAST>(static_cast<LValAST*>($1));
    assign_stmt->exp = unique_ptr<ValueAST>($3);
    $$ = assign_stmt;
  }
  | Exp ';' { 
    auto exp_stmt = new ExpStmtAST(); 
    exp_stmt->exp = unique_ptr<ValueAST>($1); 
    $$ = exp_stmt;
  }
  | ';' {
    auto empty_stmt = new EmptyStmtAST();
    $$ = empty_stmt; 
  }
  | Block {
    auto block_stmt = new BlockStmtAST(); 
    block_stmt->block = unique_ptr<BaseAST>($1); 
    $$ = block_stmt; 
  }
  | IF '(' Exp ')' MatchedStmt ELSE MatchedStmt { 
     auto if_stmt = new IfMatchedStmtAST();
     if_stmt->if_exp = unique_ptr<ValueAST>($3);
     if_stmt->if_stmt = unique_ptr<BaseAST>($5);
     if_stmt->else_stmt = unique_ptr<BaseAST>($7);
     $$ = if_stmt;
  }
  | WHILE '(' Exp ')' MatchedStmt { 
    auto while_stmt = new WhileStmtAST();
    while_stmt->while_exp = unique_ptr<ValueAST>($3);
    while_stmt->while_stmt = unique_ptr<BaseAST>($5);
    $$ = while_stmt;
  }
  | BREAK ';' { 
    auto break_stmt = new BreakStmtAST();
    $$ = break_stmt;
  }
  | CONTINUE ';' { 
    auto continue_stmt = new ContinueStmtAST();
    $$ = continue_stmt;
  }
  ;

UnmatchedStmt
  : IF '(' Exp ')' Stmt { 
    auto if_stmt = new IfUnmatchedStmtAST();
    if_stmt->if_exp = unique_ptr<ValueAST>($3);
    if_stmt->if_stmt = unique_ptr<BaseAST>($5);
    $$ = if_stmt;
  }
  | IF '(' Exp ')' MatchedStmt ELSE UnmatchedStmt { 
     auto if_else_stmt = new IfElseUnmatchedStmtAST();
     if_else_stmt->if_exp = unique_ptr<ValueAST>($3);
     if_else_stmt->if_stmt = unique_ptr<BaseAST>($5);
     if_else_stmt->else_stmt = unique_ptr<BaseAST>($7);
     $$ = if_else_stmt;
  }
  | WHILE '(' Exp ')' UnmatchedStmt { 
    auto while_stmt = new WhileStmtAST();
    while_stmt->while_exp = unique_ptr<ValueAST>($3);
    while_stmt->while_stmt = unique_ptr<BaseAST>($5);
    $$ = while_stmt;
  }
  ;

Number
  : INT_CONST   { $$ = $1; }
  ;

UnaryExp
  : PrimaryExp { 
    auto primary_unary_exp = new PrimaryUnaryExpAST();
    primary_unary_exp->primary_exp = unique_ptr<ValueAST>($1);
    $$ = primary_unary_exp;
  }
  | UnaryOp UnaryExp { 
    auto unary_op_exp = new UnaryOPExpAST();
    unary_op_exp->op = unique_ptr<OpAST>($1);
    unary_op_exp->unary_exp = unique_ptr<ValueAST>($2);
    $$ = unary_op_exp; 
  }
  | IDENT '(' ')' { 
    auto func_call_exp = new FuncCallExpAST();
    func_call_exp->ident = unique_ptr<string>($1);
    func_call_exp->func_r_params = nullptr;
    $$ = func_call_exp;
  }
  | IDENT '(' FuncRParams ')' { 
    auto func_call_exp = new FuncCallExpAST();
    func_call_exp->ident = unique_ptr<string>($1);
    func_call_exp->func_r_params = unique_ptr<BaseAST>($3);
    $$ = func_call_exp;
  }
  ;

FuncRParams
  : Exp { 
    auto func_r_params = new FuncRParamsAST();
    func_r_params->func_r_params.push_back(unique_ptr<ValueAST>($1));
    $$ = func_r_params;
  }
  | FuncRParams ',' Exp { 
    auto func_r_params = (static_cast<FuncRParamsAST*>($1));
    func_r_params->func_r_params.push_back(unique_ptr<ValueAST>($3));
    $$ = func_r_params;
  }
  ;

Exp 
  : LOrExp {
    auto exp = new ExpAST();
    exp->lor_exp = unique_ptr<ValueAST>($1);
    $$ = exp;
  }
  ;

PrimaryExp 
  : '(' Exp ')' {
    auto paren_expr = new ThreePriAST();
    paren_expr->exp = unique_ptr<ValueAST>($2);
    $$ = paren_expr;
  }
  | Number {
    auto number_exp = new NumberPriAST();
    number_exp->value = $1;
    $$ = number_exp;  
  }
  | LVal {
    auto lval_exp = new LValPriAST();
    lval_exp->lval = unique_ptr<ValueAST>($1);
    $$ = lval_exp;
  }
  ;

MulExp
  : UnaryExp {
    auto unary_mul = new UnaryMulExpAST();
    unary_mul->unary_exp = unique_ptr<ValueAST>($1);
    $$ = unary_mul;
  }
  | MulExp MulOp UnaryExp {
    auto three_mul_exp = new ThreeMulExpAST();
    three_mul_exp->mul_exp = unique_ptr<ValueAST>($1);
    three_mul_exp->op = unique_ptr<OpAST>($2);
    three_mul_exp->unary_exp = unique_ptr<ValueAST>($3);
    $$ = three_mul_exp;
  }
  ;

AddExp
  : MulExp {
    auto mul_add_exp = new MulAddExpAST();
    mul_add_exp->mul_exp = unique_ptr<ValueAST>($1);
    $$ = mul_add_exp;
  }
  | AddExp AddOp MulExp {
    auto three_add_exp = new ThreeAddExpAST();
    three_add_exp->add_exp = unique_ptr<ValueAST>($1);
    three_add_exp->op = unique_ptr<OpAST>($2);
    three_add_exp->mul_exp = unique_ptr<ValueAST>($3);
    $$ = three_add_exp;
  }
  ;

RelExp
  : AddExp {
    auto add_rel_exp = new AddRelExpAST();
    add_rel_exp->add_exp = unique_ptr<ValueAST>($1);
    $$ = add_rel_exp;
  }
  | RelExp ComOp AddExp {
    auto three_rel_exp = new ThreeRelExpAST();
    three_rel_exp->rel_exp = unique_ptr<ValueAST>($1);
    three_rel_exp->op = unique_ptr<OpAST>($2);
    three_rel_exp->add_exp = unique_ptr<ValueAST>($3);
    $$ = three_rel_exp;
  }
  ;

EqExp
  : RelExp {
    auto rel_eq_exp = new RelEqExpAST();
    rel_eq_exp->rel_exp = unique_ptr<ValueAST>($1);
    $$ = rel_eq_exp;
  }
  | EqExp EquOp RelExp {
    auto three_eq_exp = new ThreeEqExpAST();
    three_eq_exp->eq_exp = unique_ptr<ValueAST>($1);
    three_eq_exp->op = unique_ptr<OpAST>($2);
    three_eq_exp->rel_exp = unique_ptr<ValueAST>($3);
    $$ = three_eq_exp;
  }
  ;

LAndExp
  : EqExp {
    auto eq_land_exp = new EqLAndExpAST();
    eq_land_exp->eq_exp = unique_ptr<ValueAST>($1);
    $$ = eq_land_exp;
  }
  | LAndExp LAndOp EqExp {
    auto three_land_exp = new ThreeLAndExpAST();
    three_land_exp->land_exp = unique_ptr<ValueAST>($1);
    three_land_exp->op = unique_ptr<OpAST>($2);
    three_land_exp->eq_exp = unique_ptr<ValueAST>($3);
    $$ = three_land_exp;
  }
  ;

LOrExp
  : LAndExp {
    auto land_lor_exp = new LAndLOrExpAST();
    land_lor_exp->land_exp = unique_ptr<ValueAST>($1);
    $$ = land_lor_exp;
  }
  | LOrExp LOrOp LAndExp {
    auto three_lor_exp = new ThreeLOrExpAST();
    three_lor_exp->lor_exp = unique_ptr<ValueAST>($1);
    three_lor_exp->op = unique_ptr<OpAST>($2);
    three_lor_exp->land_exp = unique_ptr<ValueAST>($3);
    $$ = three_lor_exp;
  }
  ;

Decl
  : ConstDecl { auto decl = new ConstDeclDecAST(); decl->const_decl = unique_ptr<BaseAST>($1); $$ = decl; }
  | VarDecl   { auto decl = new VarDeclDecAST(); decl->var_decl = unique_ptr<BaseAST>($1); $$ = decl; }
  ;

ConstDecl
  : CONST Type const_def_list ';' {
    assert(*$2 != "void");
    auto decl_ast = new ConstDeclAST();
    decl_ast->type = unique_ptr<string>($2);
    decl_ast->const_def_list = unique_ptr<BaseAST>($3);
    $$ = decl_ast;
  }
  ;

const_def_list
  : ConstDef {
    auto const_def_list_ast = new ConstDefListAST();
    const_def_list_ast->const_defs = vector<unique_ptr<BaseAST>>();
    const_def_list_ast->const_defs.push_back(unique_ptr<BaseAST>($1));
    $$ = const_def_list_ast;
  }
  | const_def_list ',' ConstDef {
    auto const_def_list_ast = (static_cast<ConstDefListAST*>($1));
    const_def_list_ast->const_defs.push_back(unique_ptr<BaseAST>($3));
    $$ = const_def_list_ast;
  }
  ;

Type
  : FuncType { $$ = $1; }
  ;

FuncType
  : INT {
    $$ = new string("i32");
  }
  | VOID { 
    $$ = new string("void");
  }
  ;

ConstDef
  : IDENT ASSIGN ConstInitVal { 
    auto const_def = new ConstDefAST();
    const_def->ident = unique_ptr<string>($1);
    const_def->const_init_val = unique_ptr<ValueAST>(static_cast<ValueAST*> ($3));
    $$ = const_def;
  }
  | IDENT '[' ConstExp ']' ASSIGN ConstInitVal { 
    auto const_def = new ArrayConstDefAST();
    const_def->ident = unique_ptr<string>($1);
    const_def->const_exp = unique_ptr<ValueAST>($3);
    const_def->const_init_val = unique_ptr<ThreeConstInitValAST>(static_cast<ThreeConstInitValAST*>($6));
    $$ = const_def;
  }
  ;

ConstInitVal
  : ConstExp { 
    auto constinit_val = new ConstExpConstInitValAST(); 
    constinit_val->const_exp = unique_ptr<ValueAST>($1); 
    $$ = constinit_val;
  }
  | '{' '}' { auto constinit_val = new ThreeConstInitValAST(); $$ = constinit_val; }
  | '{' const_exp_list '}' {
    auto constinit_val = new ThreeConstInitValAST();
    constinit_val->const_init_val_list = unique_ptr<ConstInitValListAST>(static_cast<ConstInitValListAST*>($2));
    $$ = constinit_val;
  }
  ;

const_exp_list
  : const_exp_list ',' ConstExp {
    auto constinit_val = (static_cast<ConstInitValListAST*>($1));
    constinit_val->const_init_vals.push_back(unique_ptr<ValueAST>($3));
    $$ = constinit_val;
  }
  | ConstExp {
    auto constinit_val = new ConstInitValListAST();
     constinit_val->const_init_vals = vector<unique_ptr<ValueAST>>();
     constinit_val->const_init_vals.push_back(unique_ptr<ValueAST>($1));
    $$ = constinit_val;
  }
  ;

VarDecl
  : Type var_def_list ';' { 
    assert(*$1 != "void");
    auto var_decl = new VarDeclAST();
    var_decl->type = unique_ptr<string>($1);
    var_decl->var_def_list = unique_ptr<BaseAST>($2);
    $$ = var_decl;
  }
  ;

var_def_list
  : VarDef {
    auto var_def_list_ast = new VarDefListAST();
    var_def_list_ast->var_defs = vector<unique_ptr<BaseAST>>();
    var_def_list_ast->var_defs.push_back(unique_ptr<BaseAST>($1));
    $$ = var_def_list_ast;
  }
  | var_def_list ',' VarDef {
    auto var_def_list_ast = (static_cast<VarDefListAST*>($1));
    var_def_list_ast->var_defs.push_back(unique_ptr<BaseAST>($3));
    $$ = var_def_list_ast;
  }
  ;

VarDef
  : IDENT { 
    auto var_def = new IDENTVarAST();
    var_def->ident = unique_ptr<string>($1);
    $$ = var_def;
  }
  | IDENT '[' ConstExp ']' { 
    auto var_def = new FourVarDefAST();
    var_def->ident = unique_ptr<string>($1);
    var_def->const_exp = unique_ptr<ValueAST>($3);
    $$ = var_def;
  }
  | IDENT ASSIGN InitVal { 
    auto var_def = new ThreeVarAST();
    var_def->ident = unique_ptr<string>($1);
    var_def->init_val = unique_ptr<ValueAST>($3);
    $$ = var_def;
  }
  | IDENT '[' ConstExp ']' ASSIGN InitVal { 
    auto var_def = new SixVarDefAST();
    var_def->ident = unique_ptr<string>($1);
    var_def->const_exp = unique_ptr<ValueAST>($3);
    var_def->init_val = unique_ptr<ThreeInitValAST>(static_cast<ThreeInitValAST*>($6));
    $$ = var_def;
  }
  ;

InitVal
  : Exp { 
    auto init_val = new InitValAST();
    init_val->exp = unique_ptr<ValueAST>($1);
    $$ = init_val;
  }
  | '{' '}' { 
    auto init_val = new ThreeInitValAST();
    $$ = init_val;
  }
  | '{' initval_exp_list '}' { 
    auto init_val = new ThreeInitValAST();
    init_val->init_val_list = unique_ptr<InitValListAST>(static_cast<InitValListAST*>($2));
    $$ = init_val;
  }
  ;

initval_exp_list
  : Exp { 
    auto initval_exp_list = new InitValListAST();
    initval_exp_list->init_vals.push_back(unique_ptr<ValueAST>($1));
    $$ = initval_exp_list;
  }
  | initval_exp_list ',' Exp { 
    auto initval_exp_list = static_cast<InitValListAST*>($1);
    initval_exp_list->init_vals.push_back(unique_ptr<ValueAST>($3));
    $$ = initval_exp_list;
  }
  ; 

LVal
  : IDENT { 
    auto lval_ast = new LValAST(); 
    lval_ast->ident = unique_ptr<string>($1);
    $$ = lval_ast; 
  }
  | IDENT '[' Exp ']' { 
    auto lval_ast = new FourLValAST(); 
    lval_ast->ident = unique_ptr<string>($1);
    lval_ast->exp = unique_ptr<ValueAST>($3);
    $$ = lval_ast; 
  }
  ;

ConstExp
  : Exp { auto const_exp = new ConstExpAST(); const_exp->exp = unique_ptr<ValueAST>($1); $$ = const_exp; }
  ;

UnaryOp
  : AddOp { $$ = $1; }
  | NOT   { auto op_ast = new OpAST(); op_ast->op = "!"; $$ = op_ast; }
  ;

AddOp
  : ADD { auto op_ast = new OpAST(); op_ast->op = "+"; $$ = op_ast; }
  | SUB { auto op_ast = new OpAST(); op_ast->op = "-"; $$ = op_ast; }
  ;

MulOp
  : MUL { auto op_ast = new OpAST(); op_ast->op = "*"; $$ = op_ast; }
  | DIV { auto op_ast = new OpAST(); op_ast->op = "/"; $$ = op_ast; }
  | MOD { auto op_ast = new OpAST(); op_ast->op = "%"; $$ = op_ast; }
  ;

ComOp
  : LE { auto op_ast = new OpAST(); op_ast->op = "<="; $$ = op_ast; }
  | LT { auto op_ast = new OpAST(); op_ast->op = "<"; $$ = op_ast; }
  | GE { auto op_ast = new OpAST(); op_ast->op = ">="; $$ = op_ast; }
  | GT { auto op_ast = new OpAST(); op_ast->op = ">"; $$ = op_ast; }
  ;

EquOp
  : EQ { auto op_ast = new OpAST(); op_ast->op = "=="; $$ = op_ast; }
  | NE { auto op_ast = new OpAST(); op_ast->op = "!="; $$ = op_ast; }
  ;

LOrOp
  : L_OR  { auto op_ast = new OpAST(); op_ast->op = "||"; $$ = op_ast; }
  ;

LAndOp
  : L_AND { auto op_ast = new OpAST(); op_ast->op = "&&"; $$ = op_ast; }
  ;
%%

void yyerror(std::unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "Parser Error ad line: " << yylineno << ":" << s << endl;
}
