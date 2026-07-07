#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <fstream>
#include <string>
#include "ast/ast.hpp"
#include "ast/codegen.hpp"
#include "riscv/riscv.hpp"
#include "IR/custom_ir.hpp"

using namespace std;

extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);
int main(int argc, const char *argv[]) {
  Context context = Context();  
  // compiler 模式 输入文件 -o 输出文件
  assert(argc == 5);
  string mode = argv[1];
  auto input = argv[2];
  auto output = argv[4];

  // 打开输入文件, 并且指定 lexer 在解析的时候读取这个文件
  yyin = fopen(input, "r");
  assert(yyin);

  // 调用 parser 函数, parser 函数会进一步调用 lexer 解析输入文件的
  unique_ptr<BaseAST> ast;
  auto ret = yyparse(ast);
  assert(!ret);
   
  CodeGenerator generator(context);
  generator.Generate(*ast);

  if (mode == "-koopa") {
    // 输出 Custom IR 文本
    ofstream file(output);
    file << context.ir_program->DumpText() << endl;
  } else if (mode == "-riscv") {
    // 使用重构的 RiscvGenerator 直接从内存 IR 生成汇编
    Back::RiscvGenerator riscv_gen;
    ofstream file(output);
    streambuf *coutbuf = cout.rdbuf();
    cout.rdbuf(file.rdbuf());
    riscv_gen.Generate(*context.ir_program);
    cout.rdbuf(coutbuf);
  }
  return 0;
}
