#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <fstream>
#include <string>
#include "ast.hpp"
#include "IR/custom_ir.hpp"
#include "backend/target/riscv_target.hpp"
#include "backend/codegen/riscv_generator.hpp"

using namespace std;

extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);
int main(int argc, const char *argv[]) {
  Context context = Context();  

  string mode = "-riscv";
  string input;
  string output;
  Back::TargetConfig target_config;
  target_config.is_64bit = true;
  Back::OptimizationLevel optimization_level = Back::OptimizationLevel::O0;

  for (int i = 1; i < argc; i++) {
    string arg = argv[i];
    if (arg == "-S" || arg == "-riscv" || arg == "-perf") {
      mode = "-riscv";
      if (arg == "-riscv" || arg == "-perf") {
        target_config.is_64bit = false;
      }
    } else if (arg == "-koopa") {
      mode = "-koopa";
    } else if (arg == "-o") {
      assert(i + 1 < argc);
      output = argv[++i];
    } else if (arg == "-O0") {
      optimization_level = Back::OptimizationLevel::O0;
    } else if (arg == "-O1") {
      optimization_level = Back::OptimizationLevel::O1;
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
   
  GenerateIR(*ast, context);

  if (mode == "-koopa") {
    // 输出 Custom IR 文本
    ofstream file(output);
    file << context.ir_program->DumpText() << endl;
  } else if (mode == "-riscv") {
    // 使用重构 of RiscvGenerator 直接从内存 IR 生成汇编
    Back::RiscvGenerator riscv_gen(target_config, optimization_level);
    ofstream file(output);
    streambuf *coutbuf = cout.rdbuf();
    cout.rdbuf(file.rdbuf());
    riscv_gen.Generate(*context.ir_program);
    cout.rdbuf(coutbuf);
  }
  return 0;
}
