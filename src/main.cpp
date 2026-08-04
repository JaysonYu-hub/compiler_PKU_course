#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <fstream>
#include <string>
#include "ast.hpp"
#include "IR/custom_ir.hpp"
#include "backend/riscv_target.hpp"
#include "backend/codegen/riscv_generator.hpp"
#include "IR/passes/cfg_simplify.hpp"
#include "IR/passes/dce.hpp"
#include "IR/passes/gvn.hpp"
#include "IR/passes/instcombine.hpp"
#include "IR/passes/ir_verifier.hpp"
#include "IR/passes/licm.hpp"
#include "IR/passes/loop_strength_reduce.hpp"
#include "IR/passes/local_memory_opt.hpp"
#include "IR/passes/mem2reg.hpp"
#include "IR/passes/sccp.hpp"
#include "IR/passes/tiny_leaf_inline.hpp"

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

  for (int i = 1; i < argc; i++) {
    string arg = argv[i];
    if (arg == "-S" || arg == "-riscv" || arg == "-perf") {
      mode = "-riscv";
      if (arg == "-riscv" || arg == "-perf") {
        target_config.Set64Bit(false);
      }
    } else if (arg == "-koopa") {
      mode = "-koopa";
    } else if (arg == "-o") {
      assert(i + 1 < argc);
      output = argv[++i];
    } else if (arg == "-O0") {
      target_config.optimization_level = Back::OptimizationLevel::O0;
    } else if (arg == "-O1") {
      target_config.optimization_level = Back::OptimizationLevel::O1;
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
  if (ret != 0) return ret;
   
  GenerateIR(*ast, context);

  const bool pass_timing = std::getenv("COMPILER_PASS_TIMING") != nullptr;
  auto run_timed = [&](const char* name, auto&& action) {
    const auto begin = std::chrono::steady_clock::now();
    action();
    if (pass_timing) {
      const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - begin);
      std::cerr << "[pass-time] " << name << '=' << elapsed.count()
                << "ms\n";
    }
  };

  if (target_config.optimization_level == Back::OptimizationLevel::O1) {
    // 第一阶段 O1 流水线：先把局部变量提升为 SSA，再依次进行稀疏常量
    // 传播、局部化简、CFG 清理、公共子表达式消除和真正的死代码删除。
    // 后半段 LICM 可能暴露新的重复式与死值，因此再做一轮轻量清理。
    IR::Pass::MemToReg mem2reg;
    run_timed("mem2reg", [&] { mem2reg.Run(context.ir_program.get()); });

    // 只内联无递归、无内存副作用、单基本块的小型叶子函数。
    // 内联会把调用点暴露为普通 SSA 表达式，紧随其后的 SCCP、InstCombine
    // 和 DCE 可以继续传播实参常量并删除无用的中间结果。
    IR::Pass::TinyLeafInline tiny_leaf_inline(/*instruction_limit=*/12);
    run_timed("tiny-leaf-inline",
              [&] { tiny_leaf_inline.Run(context.ir_program.get()); });

    // 结构变换后立即检查块终结指令、跳转实参数量和悬空 SSA 引用，
    // 防止错误继续传播到后端并表现为难以定位的汇编或运行时问题。
    IR::Pass::IRVerifier verifier;
    std::string verify_error;
    if (!verifier.Verify(*context.ir_program, &verify_error)) {
      std::cerr << "IR verification failed after tiny leaf inline: "
                << verify_error << '\n';
      return 1;
    }

    // SCCP 按基本块顺序传播。先清理并重排为 RPO，能让直线和前向 CFG
    // 在一次扫描中传播，避免超长函数因块顺序不佳而反复全函数迭代。
    IR::Pass::CFGSimplify cfg_simplify;
    run_timed("cfg-simplify-pre",
              [&] { cfg_simplify.Run(context.ir_program.get()); });

    IR::Pass::SCCP sccp;
    run_timed("sccp", [&] { sccp.Run(context.ir_program.get()); });

    IR::Pass::InstCombine inst_combine;
    run_timed("instcombine-1",
              [&] { inst_combine.Run(context.ir_program.get()); });

    run_timed("cfg-simplify-1",
              [&] { cfg_simplify.Run(context.ir_program.get()); });

    IR::Pass::GVN gvn;
    run_timed("gvn-1", [&] { gvn.Run(context.ir_program.get()); });

    IR::Pass::LocalMemoryOptimization local_memory_opt;
    run_timed("local-memory-1",
              [&] { local_memory_opt.Run(context.ir_program.get()); });

    IR::Pass::DCE dce;
    run_timed("dce-1", [&] { dce.Run(context.ir_program.get()); });

    // 将自然循环整理为唯一 preheader，再识别 header 块参数形式的基本
    // 归纳变量。数组地址由 base + iv * stride 改成沿回边递增的指针，
    // 使后端把动态索引移位/乘法降为常量步长地址更新。
    IR::Pass::LoopStrengthReduction loop_strength_reduction;
    run_timed("loop-strength-reduce", [&] {
      loop_strength_reduction.Run(context.ir_program.get());
    });
    if (pass_timing) {
      const auto& stats = loop_strength_reduction.GetStats();
      std::cerr << "[loop-strength-reduce] loops=" << stats.loops_seen
                << " preheaders=" << stats.preheaders_created
                << " ivs=" << stats.induction_variables
                << " addresses=" << stats.addresses_rewritten << '\n';
    }

    verify_error.clear();
    if (!verifier.Verify(*context.ir_program, &verify_error)) {
      std::cerr << "IR verification failed after loop strength reduction: "
                << verify_error << '\n';
      return 1;
    }

    IR::Pass::LICM licm;
    run_timed("licm", [&] { licm.Run(context.ir_program.get()); });

    run_timed("instcombine-2",
              [&] { inst_combine.Run(context.ir_program.get()); });
    run_timed("gvn-2", [&] { gvn.Run(context.ir_program.get()); });
    run_timed("local-memory-2",
              [&] { local_memory_opt.Run(context.ir_program.get()); });
    run_timed("cfg-simplify-2",
              [&] { cfg_simplify.Run(context.ir_program.get()); });
    run_timed("dce-2", [&] { dce.Run(context.ir_program.get()); });
  }

  if (mode == "-koopa") {
    // 输出 Custom IR 文本
    ofstream file(output);
    file << context.ir_program->DumpText() << endl;
  } else if (mode == "-riscv") {
    Back::RiscvGenerator riscv_gen(target_config);
    ofstream file(output);
    streambuf *coutbuf = cout.rdbuf();
    cout.rdbuf(file.rdbuf());
    riscv_gen.Generate(*context.ir_program);
    cout.rdbuf(coutbuf);
  }
  return 0;
}
