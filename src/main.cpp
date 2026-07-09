#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <fstream>
#include <string>
#include "ast.hpp"
#include "codegen.hpp"
#include "riscv.hpp"
#include "custom_ir.hpp"

using namespace std;

extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);
int main(int argc, const char *argv[]) {
  Context context = Context();  

  string mode = "-riscv";
  string input;
  string output;
  bool is_64bit = true;

  for (int i = 1; i < argc; i++) {
    string arg = argv[i];
    if (arg == "-S" || arg == "-riscv") {
      mode = "-riscv";
      if (arg == "-riscv") {
        is_64bit = false;
      }
    } else if (arg == "-koopa") {
      mode = "-koopa";
    } else if (arg == "-o") {
      assert(i + 1 < argc);
      output = argv[++i];
    } else if (arg == "-O0") {
    } else if (arg == "-O1") {
      return 0;
    } else {
      input = arg;
    }
  }

  assert(!input.empty());
  assert(!output.empty());

  // 打开输入文件, 并且指定 lexer 在解析的时候读取这个文件
  yyin = fopen(input.c_str(), "r");
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
    Back::RiscvGenerator riscv_gen(is_64bit);
    ofstream file(output);
    streambuf *coutbuf = cout.rdbuf();
    cout.rdbuf(file.rdbuf());
    riscv_gen.Generate(*context.ir_program);
    cout.rdbuf(coutbuf);
  }
  return 0;
}
