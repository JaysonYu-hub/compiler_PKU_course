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

int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

%parse-param { std::unique_ptr<BaseAST> &ast }

%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
}

%token INT RETURN
%token <str_val> IDENT
%token <int_val> INT_CONST

%token ADD SUB NOT MUL MOD DIV
%token LT LE GT GE EQ NE L_AND L_OR

%type <ast_val> FuncDef FuncType Block Stmt Exp UnaryExp PrimaryExp UnaryOp
%type <ast_val> MulExp AddExp
%type <ast_val> RelExp EqExp LAndExp LOrExp
%type <ast_val> AddOp MulOp
%type <ast_val> ComOp EquOp LOrOp LAndOp
%type <int_val> Number

%%

CompUnit
  : FuncDef {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->func_def = unique_ptr<BaseAST>($1);
    ast = std::move(comp_unit);
  }
  ;

FuncDef
  : FuncType IDENT '(' ')' Block {
    auto func_def = new FuncDefAST();
    func_def->func_type = unique_ptr<BaseAST>($1);
    func_def->ident = unique_ptr<string>($2);
    func_def->block = unique_ptr<BaseAST>($5);
    $$ = func_def;
  }
  ;

FuncType
  : INT {
    auto func_type = new FuncTypeAST();
    auto type = "i32";
    func_type->type_name = type;
    $$ = func_type;
  }
  ;

Block
  : '{' Stmt '}' {
    auto block = new BlockAST();
    block->stmt = unique_ptr<BaseAST>($2);
    $$ = block;
  }
  ;

Stmt
  : RETURN Exp ';' {
    auto stmt = new StmtAST();
    stmt->exp = unique_ptr<BaseAST>($2);
    $$ = stmt;
  }
  ;

Number
  : INT_CONST   { $$ = $1; }
  ;

UnaryExp
  : PrimaryExp { 
    $$ = $1; 
  }
  | UnaryOp UnaryExp { 
    auto unary_op_exp = new UnaryOPExpAST();
    // 使用 static_cast 确保指针转换到对应的子类安全级别
    unary_op_exp->op = unique_ptr<OpAST>(static_cast<OpAST*>($1));
    unary_op_exp->unary_exp = unique_ptr<UnaryExpAST>(static_cast<UnaryExpAST*>($2));
    $$ = unary_op_exp; 
  }
  ;

Exp 
  : LOrExp {
    auto exp = new ExpAST();
    exp->lor_exp = unique_ptr<BaseAST>($1);
    $$ = exp;
  }
  ;

PrimaryExp 
  : '(' Exp ')' {
    auto paren_expr = new ParenExprAST();
    paren_expr->exp = unique_ptr<BaseAST>($2);
    $$ = paren_expr;
  }
  | Number {
    auto number_exp = new NumberExpAST();
    number_exp->value = $1;
    $$ = number_exp;  
  }
  ;


MulExp
  : UnaryExp {
    auto unary_mul = new UnaryMulExpAST();
    unary_mul->unary_exp = unique_ptr<BaseAST>($1);
    $$ = unary_mul;
  }
  | MulExp MulOp UnaryExp {
    auto three_mul_exp = new ThreeMulExpAST();
    three_mul_exp->mul_exp = unique_ptr<MulExpAST>(static_cast<MulExpAST*>($1));
    three_mul_exp->op = unique_ptr<OpAST>(static_cast<OpAST*>($2));
    three_mul_exp->unary_exp = unique_ptr<UnaryExpAST>(static_cast<UnaryExpAST*>($3));
    $$ = three_mul_exp;
  }
  ;

AddExp
  : MulExp {
    auto mul_add_exp = new MulAddExpAST();
    mul_add_exp->mul_exp = unique_ptr<BaseAST>($1);
    $$ = mul_add_exp;
  }
  | AddExp AddOp MulExp {
    auto three_add_exp = new ThreeAddExpAST();
    three_add_exp->add_exp = unique_ptr<AddExpAST>(static_cast<AddExpAST*>($1));
    three_add_exp->op = unique_ptr<OpAST>(static_cast<OpAST*>($2));
    three_add_exp->mul_exp = unique_ptr<MulExpAST>(static_cast<MulExpAST*>($3));
    $$ = three_add_exp;
  }
  ;

RelExp
  : AddExp {
    auto add_rel_exp = new AddRelExpAST();
    add_rel_exp->add_exp = unique_ptr<BaseAST>($1);
    $$ = add_rel_exp;
  }
  | RelExp ComOp AddExp {
    auto three_rel_exp = new ThreeRelExpAST();
    three_rel_exp->rel_exp = unique_ptr<RelExpAST>(static_cast<RelExpAST*>($1));
    three_rel_exp->op = unique_ptr<OpAST>(static_cast<OpAST*>($2));
    three_rel_exp->add_exp = unique_ptr<AddExpAST>(static_cast<AddExpAST*>($3));
    $$ = three_rel_exp;
  }
  ;

EqExp
  : RelExp {
    auto rel_eq_exp = new RelEqExpAST();
    rel_eq_exp->rel_exp = unique_ptr<BaseAST>($1);
    $$ = rel_eq_exp;
  }
  | EqExp EquOp RelExp {
    auto three_eq_exp = new ThreeEqExpAST();
    three_eq_exp->eq_exp = unique_ptr<EqExpAST>(static_cast<EqExpAST*>($1));
    three_eq_exp->op = unique_ptr<OpAST>(static_cast<OpAST*>($2));
    three_eq_exp->rel_exp = unique_ptr<RelExpAST>(static_cast<RelExpAST*>($3));
    $$ = three_eq_exp;
  }
  ;

LAndExp
  : EqExp {
    auto eq_land_exp = new EqLAndExpAST();
    eq_land_exp->eq_exp = unique_ptr<BaseAST>($1);
    $$ = eq_land_exp;
  }
  | LAndExp LAndOp EqExp {
    auto three_land_exp = new ThreeLAndExpAST();
    three_land_exp->land_exp = unique_ptr<LAndExpAST>(static_cast<LAndExpAST*>($1));
    three_land_exp->op = unique_ptr<OpAST>(static_cast<OpAST*>($2));
    three_land_exp->eq_exp = unique_ptr<EqExpAST>(static_cast<EqExpAST*>($3));
    $$ = three_land_exp;
  }
  ;

LOrExp
  : LAndExp {
    auto land_lor_exp = new LAndLOrExpAST();
    land_lor_exp->land_exp = unique_ptr<BaseAST>($1);
    $$ = land_lor_exp;
  }
  | LOrExp LOrOp LAndExp {
    auto three_lor_exp = new ThreeLOrExpAST();
    three_lor_exp->lor_exp = unique_ptr<LOrExpAST>(static_cast<LOrExpAST*>($1));
    three_lor_exp->op = unique_ptr<OpAST>(static_cast<OpAST*>($2));
    three_lor_exp->land_exp = unique_ptr<LAndExpAST>(static_cast<LAndExpAST*>($3));
    $$ = three_lor_exp;
  }
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
  cerr << "error: " << s << endl;
}