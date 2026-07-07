#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <fstream>
#include <string>
#include "ast/ast.hpp"
#include "ast/codegen.hpp"
#include "koopa.h"
#include "riscv/riscv.hpp"
#include "IR/custom_ir.hpp"

using namespace std;

extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);
int main(int argc, const char *argv[]) {
  Context context = Context();  
  Back::IRVisitor ir_visitor;
  // 解析命令行参数. 测试脚本/评测平台要求你的编译器能接收如下参数:
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

  // 使用 DumpText 将内存中的 Custom IR 写入到输出文件
  {
    ofstream file(output);
    file << context.ir_program->DumpText() << endl;
  }
  
  // clog << "--- Custom C++ IR Dump ---\n" << context.ir_program->DumpText() << "-------------------------\n";
  
  if (mode == "-riscv") {
    koopa_program_t program;
    koopa_error_code_t ret_code = koopa_parse_from_file(output, &program);
    assert(ret_code == KOOPA_EC_SUCCESS);
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    koopa_delete_program(program);

    Back::RiscvGenerator riscv_gen;
    {
      ofstream file(output);
      streambuf *coutbuf = cout.rdbuf(); // 保存原来的缓冲区
      cout.rdbuf(file.rdbuf()); // 将 cout 的缓冲区切换到文件
      riscv_gen.Generate(*context.ir_program);
      cout.rdbuf(coutbuf); // 恢复原来的缓冲区
    }

    clog << "\n--- [原版 RISC-V 生成器输出（对照）] ---\n";
    streambuf *clog_buf = cout.rdbuf();
    cout.rdbuf(clog.rdbuf()); // 将 std::cout 临时重定向到 std::clog 打印到控制台
    
    ir_visitor.HandleRawProgram(raw);
    
    cout.rdbuf(clog_buf); // 恢复原来的 std::cout
    clog << "-------------------------------------------\n\n";

    koopa_delete_raw_program_builder(builder);
  }
  return 0;
}
