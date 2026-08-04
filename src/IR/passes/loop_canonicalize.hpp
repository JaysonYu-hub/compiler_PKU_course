#pragma once

#include "loop_utils.hpp"

#include <cstddef>

namespace IR {
namespace Pass {

struct LoopCanonicalizeStats {
  std::size_t loops_seen = 0;
  std::size_t preheaders_created = 0;
};

// 第一阶段循环规范化只建立唯一 dedicated preheader。它是 LICM、归纳
// 变量转换和后续循环展开共同依赖的最小规范形态。
class LoopCanonicalize {
 public:
  bool Run(IRProgram* program) {
    stats_ = {};
    if (!program) return false;
    bool changed = false;
    for (const auto& function : program->funcs) {
      if (!function || function->blocks.empty()) continue;
      changed |= RunFunction(*function);
    }
    return changed;
  }

  const LoopCanonicalizeStats& GetStats() const { return stats_; }

 private:
  bool RunFunction(IRFunction& function) {
    DominatorTree dominator_tree;
    dominator_tree.Analyze(function);
    LoopAnalysis analysis;
    auto loops = analysis.FindLoops(function, dominator_tree);
    LoopValueNameGenerator names(function);
    bool changed = false;
    for (auto& loop : loops) {
      ++stats_.loops_seen;
      std::vector<IRBasicBlock*> external;
      for (IRBasicBlock* predecessor :
           function.GetPredecessors(loop.header)) {
        if (loop.blocks.count(predecessor) == 0)
          external.push_back(predecessor);
      }
      const bool already_canonical =
          external.size() == 1 &&
          IsDedicatedPreheader(external.front(), loop.header);
      if (!EnsureDedicatedPreheader(function, loop, names)) continue;
      if (!already_canonical) {
        ++stats_.preheaders_created;
        changed = true;
      }
    }
    return changed;
  }

  LoopCanonicalizeStats stats_;
};

}  // namespace Pass
}  // namespace IR
