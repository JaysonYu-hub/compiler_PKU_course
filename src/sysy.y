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

%token ADD SUB NOT

%type <ast_val> FuncDef FuncType Block Stmt Exp UnaryExp PrimaryExp UnaryOp
%type <int_val> Number

%%

CompUnit
  : FuncDef {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->func_def = unique_ptr<BaseAST>($1);
    ast = std::move(comp_unit);
  }
  ;

// FuncDef ::= FuncType IDENT '(' ')' Block;
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
    unary_op_exp->unary_op = unique_ptr<UnaryOPAST>(static_cast<UnaryOPAST*>($1));
    unary_op_exp->unary_exp = unique_ptr<UnaryExpAST>(static_cast<UnaryExpAST*>($2));
    $$ = unary_op_exp; 
  }
  ;

Exp 
  : UnaryExp {
    auto exp = new ExpAST();
    exp->unary_exp = unique_ptr<BaseAST>($1);
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

UnaryOp
  : ADD { auto op = new UnaryOPAST(); op->unary_op = "+"; $$ = op; }
  | SUB { auto op = new UnaryOPAST(); op->unary_op = "-"; $$ = op; }
  | NOT { auto op = new UnaryOPAST(); op->unary_op = "!"; $$ = op; }
  ;

%%

void yyerror(std::unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}