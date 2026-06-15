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
  ValueAST *value_ast;
  OpAST *op_ast;
}

%token INT RETURN 
%token CONST
%token <str_val> IDENT
%token <int_val> INT_CONST

%token ADD SUB NOT MUL MOD DIV
%token <str_val> ASSIGN LVal BType
%token LT LE GT GE EQ NE L_AND L_OR

%type <ast_val> FuncDef FuncType Block Stmt 
%type <value_val> Exp UnaryExp PrimaryExp UnaryOpMulExp AddExp ConstExp
%type <value_val> RelExp EqExp LAndExp LOrExp ConstInitVal
%type <op_val> AddOp MulOp
%type <op_val> ComOp EquOp LOrOp LAndOp
%type <int_val> Number
%type <ast_val> Decl ConstDecl BlockItem block_item_list const_def_list

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
  : '{' block_item_list '}' { 
    $$ = $2;
  }
  ;

block_item_list 
  : { 
    auto block_ast = new unique_ptr<BlockAST>(); 
    block_ast->block_items = new vector<unique_ptr<BaseAST>>(); 
    $$ = block_ast; 
  }
  | block_item_list Block_item { 
    auto block_ast = (static_cast<BlockAST*>($1));
    block_ast->block_items.push_back($2); 
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
    unary_op_exp->op = unique_ptr<OpAST>($1);
    unary_op_exp->unary_exp = unique_ptr<ValueAST>($2);
    $$ = unary_op_exp; 
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
    auto paren_expr = new ParenExprAST();
    paren_expr->exp = unique_ptr<ValueAST>($2);
    $$ = paren_expr;
  }
  | Number {
    auto number_exp = new NumberExpAST();
    number_exp->value = $1;
    $$ = number_exp;  
  }
  | LVal {
    auto lval_exp = new LValExpAST();
    lval_exp->lval = unique_ptr<string>($1);
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
  : ConstDecl { auto decl = new DeclAST(); decl->const_decl = unique_ptr<BaseAST>($1); $$ = decl; }
  ;

ConstDecl
  : CONST BType const_def_list ';' {
    auto decl_ast = new ConstDeclAST();
    decl_ast->b_type = unique_ptr<string>($2);
    decl_ast->const_def_list = unique_ptr<BaseAST>($3);
    $$ = decl_ast;
  }
  ;

const_def_list
  : ConstDef {
    auto const_def_list_ast = new ConstDefListAST();
    const_def_list_ast->const_defs = vector<unique_ptr<BaseAST>>($1);
    $$ = const_def_list_ast;
  }
  | const_def_list ',' ConstDef {
    auto const_def_list_ast = (static_cast<ConstDefListAST*>($1));
    const_def_list_ast->const_defs.push_back(unique_ptr<BaseAST>($3));
    $$ = const_def_list_ast;
  }

BType
  : INT { $$ = $1 }
  ;

ConstDef
  : IDENT '=' ConstInitVal { 
    auto const_def = new ConstDefAST();
    const_def->ident = unique_ptr<string>($1);
    const_def->const_init_val = unique_ptr<ValueAST>($3);
    $$ = const_def;
   }
  ;

ConstInitVal
  : ConstExp { 
    auto constinit_val = new ConstInitValAST(); 
    constinit_val->const_exp = unique_ptr<ValueAST>($1); 
    $$ = constinit_val;
  }
  ;

LVal
  : IDENT { $$ = $1; }
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
  cerr << "error: " << s << endl;
}
