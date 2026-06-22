%option noyywrap
%option nounput
%option noinput
%option yylineno

%{

#include <cstdlib>
#include <string>

// 因为 Flex 会用到 Bison 中关于 token 的定义
// 所以需要 include Bison 生成的头文件
#include "sysy.tab.hpp"

using namespace std;

%}

/* 空白符和注释 */
WhiteSpace    [ \t\n\r]+
LineComment   "//".*


/* 标识符 */
Identifier    [a-zA-Z_][a-zA-Z0-9_]*

/* 整数字面量 */
Decimal       [1-9][0-9]*
Octal         0[0-7]*
Hexadecimal   0[xX][0-9a-fA-F]+

/* 多行注释状态 */
%x NOTECOMMENT

%%

"/*"                    {BEGIN(NOTECOMMENT); }
<NOTECOMMENT>"*/"       {BEGIN(INITIAL); /* 遇到多行注释结尾 */}
<NOTECOMMENT>([^*]|\n)+ {/* 吞掉所有非星字符和换行 */}
<NOTECOMMENT>"*"        {/* 吞掉单独的星号 */}

{WhiteSpace}    { /* 忽略, 不做任何操作 */ }
{LineComment}   { /* 忽略, 不做任何操作 */ }

"int"           { return INT; }
"return"        { return RETURN; }
"const"         { return CONST; }
"if"            { return IF; }              
"else"          { return ELSE; }
"while"         { return WHILE; }

{Identifier}    { yylval.str_val = new string(yytext); return IDENT; }

{Decimal}       { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Octal}         { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Hexadecimal}   { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }

"+" { return ADD; }
"-" { return SUB; }
"!" { return NOT; }
"*" { return MUL; }
"/" { return DIV; }
"%" { return MOD; }

"=" { yylval.str_val = new string(yytext); return ASSIGN; }

"<"  { return LT; }
">"  { return GT; }
"<=" { return LE; }
">=" { return GE; }
"==" { return EQ; }
"!=" { return NE; }
"&&" { return L_AND; }
"||" { return L_OR; }


.               { return yytext[0]; }

%%
