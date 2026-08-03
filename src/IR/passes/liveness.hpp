#pragma once

#include "../custom_ir.hpp"
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <vector>
#include <limits>
#include <array>
#include <algorithm>
#include <cstdint>

namespace IR {
namespace Pass {

class LiveVarAnalysis {
public:
  struct NaturalLoopRegion {
    std::string header;
    std::string preheader;
    std::unordered_set<std::string> blocks;
    int depth = 0;
  };

  std::unordered_map<std::string, std::unordered_set<std::string>> live_out;
  std::unordered_map<std::string, std::unordered_set<std::string>> live_in;
  std::unordered_map<std::shared_ptr<IR::IRInstruction>, std::unordered_set<std::string>> live_out_inst;
  std::unordered_map<std::shared_ptr<IR::IRInstruction>, std::unordered_set<std::string>> live_in_inst;
  std::unordered_map<std::string, std::unordered_map<std::string, std::array<long long int, 2>>> func_intervals;

  // 第二阶段寄存器分配提示。和活跃区间由同一次分析产生，避免信息不一致。
  std::unordered_map<std::string, std::unordered_map<std::string, std::uint64_t>> func_use_weights;
  std::unordered_map<std::string, std::unordered_map<std::string, std::uint64_t>> func_use_counts;
  std::unordered_map<std::string, std::unordered_set<std::string>> func_crosses_calls;
  std::unordered_map<std::string,
      std::unordered_map<std::string, std::uint64_t>> func_call_cross_counts;
  std::unordered_map<std::string, std::unordered_set<std::string>> func_float_values;
  std::unordered_map<std::string, std::unordered_map<std::string, int>> func_loop_depth;
  std::unordered_map<std::string, std::vector<NaturalLoopRegion>> func_natural_loops;
  long long int max_inf = std::numeric_limits<long long int>::max();
  long long int min_inf = std::numeric_limits<long long int>::min();
  const std::vector<std::string> caller_saved_registers = {
    "ra", "t0", "t1", "t2", "t3", "t4", "t5", "t6",
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
    "ft0", "ft1", "ft2", "ft3", "ft4", "ft5", "ft6", "ft7", "ft8", "ft9", "ft10", "ft11",
    "fa0", "fa1", "fa2", "fa3", "fa4", "fa5", "fa6", "fa7"
  };

  void Analyze(IR::IRProgram *program);

private:
  void CompuateBlockGenKill(std::shared_ptr<IR::IRBasicBlock> block, std::unordered_set<std::string> &var_gen, std::unordered_set<std::string> &var_kill);
};

}
}
