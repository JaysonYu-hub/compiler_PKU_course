#pragma once

#include "../custom_ir.hpp"
#include "tool.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace IR {
namespace Pass {

// 自然循环的公共描述。header 是循环头，latches 是所有回边源，blocks
// 包含循环头和循环体。后续规范化、归纳变量分析和 LICM 都可以复用它。
struct CanonicalLoopInfo {
  IRBasicBlock* header = nullptr;
  std::unordered_set<IRBasicBlock*> blocks;
  std::unordered_set<IRBasicBlock*> latches;
  IRBasicBlock* preheader = nullptr;
};

class LoopAnalysis {
 public:
  std::vector<CanonicalLoopInfo> FindLoops(
      IRFunction& function, const DominatorTree& dominator_tree) const {
    std::vector<CanonicalLoopInfo> loops;
    std::unordered_map<IRBasicBlock*, std::size_t> by_header;

    for (const auto& block_ptr : function.blocks) {
      IRBasicBlock* latch = block_ptr.get();
      for (IRBasicBlock* header : latch->GetSuccessors()) {
        if (!header || !dominator_tree.Dominates(header, latch)) continue;

        std::unordered_set<IRBasicBlock*> natural_loop = {header, latch};
        std::vector<IRBasicBlock*> worklist;
        if (latch != header) worklist.push_back(latch);
        while (!worklist.empty()) {
          IRBasicBlock* current = worklist.back();
          worklist.pop_back();
          for (IRBasicBlock* predecessor :
               function.GetPredecessors(current)) {
            if (!predecessor || predecessor == header) continue;
            if (natural_loop.insert(predecessor).second)
              worklist.push_back(predecessor);
          }
        }

        auto [iterator, inserted] =
            by_header.emplace(header, loops.size());
        if (inserted) {
          CanonicalLoopInfo loop;
          loop.header = header;
          loops.push_back(std::move(loop));
        }
        auto& loop = loops[iterator->second];
        loop.blocks.insert(natural_loop.begin(), natural_loop.end());
        loop.latches.insert(latch);
      }
    }
    return loops;
  }
};

// IR 名字在后端被当作值的唯一键。结构变换创建的新块参数和指令必须使用
// 全函数唯一名字，不能只保证在当前基本块内唯一。
class LoopValueNameGenerator {
 public:
  explicit LoopValueNameGenerator(const IRFunction& function) {
    for (const auto& parameter : function.params)
      if (parameter) used_.insert(parameter->GetOperandName());
    for (const auto& block : function.blocks) {
      if (!block) continue;
      for (const auto& parameter : block->params)
        if (parameter) used_.insert(parameter->GetOperandName());
      for (const auto& instruction : block->insts) {
        if (!instruction) continue;
        if (isa<IRNameInstruction>(instruction) ||
            (isa<IRCall>(instruction) &&
             cast<IRCall>(instruction)->HasReturnValue()))
          used_.insert(instruction->GetOperandName());
      }
    }
  }

  std::string Next(const std::string& stem = "%ivaddr") {
    while (true) {
      std::string candidate = stem + "_" + std::to_string(next_++);
      if (used_.insert(candidate).second) return candidate;
    }
  }

 private:
  std::size_t next_ = 0;
  std::unordered_set<std::string> used_;
};

inline bool IsDedicatedPreheader(const IRBasicBlock* candidate,
                                 const IRBasicBlock* header) {
  if (!candidate || !header) return false;
  const auto successors = candidate->GetSuccessors();
  return successors.size() == 1 && successors.front() == header;
}

// 在一条 predecessor -> target 边后附加块实参。分支两臂恰好都指向同一
// 目标时两条边都必须更新，否则块参数数量会不匹配。
inline bool AppendEdgeArgument(IRBasicBlock& predecessor,
                               IRBasicBlock* target,
                               const std::shared_ptr<IRValue>& argument) {
  if (!target || !argument || predecessor.insts.empty()) return false;
  auto& terminator = predecessor.insts.back();
  if (auto* jump = dyn_cast<IRJump>(terminator.get())) {
    if (jump->GetTarget().get() != target) return false;
    auto arguments = jump->GetArgs();
    arguments.push_back(argument);
    jump->SetArgs(std::move(arguments));
    return true;
  }
  if (auto* branch = dyn_cast<IRBranch>(terminator.get())) {
    bool changed = false;
    if (branch->GetThenTarget().get() == target) {
      auto arguments = branch->GetThenArgs();
      arguments.push_back(argument);
      branch->SetThenArgs(std::move(arguments));
      changed = true;
    }
    if (branch->GetElseTarget().get() == target) {
      auto arguments = branch->GetElseArgs();
      arguments.push_back(argument);
      branch->SetElseArgs(std::move(arguments));
      changed = true;
    }
    return changed;
  }
  return false;
}

inline const std::vector<std::shared_ptr<IRValue>>* GetJumpEdgeArguments(
    const IRBasicBlock& predecessor, const IRBasicBlock* target) {
  if (!target || predecessor.insts.empty()) return nullptr;
  const auto* jump = dyn_cast<IRJump>(predecessor.insts.back().get());
  if (!jump || jump->GetTarget().get() != target) return nullptr;
  return &jump->GetArgs();
}

// 把循环入口规范化为唯一 dedicated preheader。新块只负责转发 header
// 参数，因此不会改变原控制流语义。
inline IRBasicBlock* EnsureDedicatedPreheader(
    IRFunction& function, CanonicalLoopInfo& loop,
    LoopValueNameGenerator& names) {
  IRBasicBlock* header = loop.header;
  if (!header) return nullptr;

  std::vector<IRBasicBlock*> external_predecessors;
  for (IRBasicBlock* predecessor : function.GetPredecessors(header)) {
    if (loop.blocks.count(predecessor) == 0)
      external_predecessors.push_back(predecessor);
  }
  if (external_predecessors.empty()) return nullptr;
  if (external_predecessors.size() == 1 &&
      IsDedicatedPreheader(external_predecessors.front(), header)) {
    loop.preheader = external_predecessors.front();
    return loop.preheader;
  }

  std::string block_name = header->name + ".loop_preheader";
  unsigned suffix = 0;
  auto block_name_exists = [&](const std::string& candidate) {
    return std::any_of(function.blocks.begin(), function.blocks.end(),
                       [&](const auto& block) {
                         return block && block->name == candidate;
                       });
  };
  while (block_name_exists(block_name))
    block_name = header->name + ".loop_preheader." +
                 std::to_string(++suffix);

  auto preheader = std::make_shared<IRBasicBlock>(block_name);
  std::vector<std::shared_ptr<IRValue>> forwarded;
  forwarded.reserve(header->params.size());
  for (const auto& parameter : header->params) {
    auto new_parameter =
        preheader->AddParam(names.Next("%loop_pre"), parameter->GetType());
    forwarded.push_back(new_parameter);
  }
  preheader->AddInstruction(
      std::make_shared<IRJump>(header->shared_from_this(), forwarded));

  for (IRBasicBlock* predecessor : external_predecessors) {
    if (!predecessor || predecessor->insts.empty()) continue;
    const auto old_terminator = predecessor->insts.back();
    if (const auto* jump = dyn_cast<IRJump>(old_terminator.get())) {
      if (jump->GetTarget().get() == header)
        predecessor->insts.back() =
            std::make_shared<IRJump>(preheader, jump->GetArgs());
      continue;
    }
    const auto* branch = dyn_cast<IRBranch>(old_terminator.get());
    if (!branch) continue;
    auto then_target = branch->GetThenTarget();
    auto else_target = branch->GetElseTarget();
    if (then_target.get() == header) then_target = preheader;
    if (else_target.get() == header) else_target = preheader;
    predecessor->insts.back() = std::make_shared<IRBranch>(
        branch->GetCond(), then_target, else_target, branch->GetThenArgs(),
        branch->GetElseArgs());
  }

  const auto header_position =
      std::find_if(function.blocks.begin(), function.blocks.end(),
                   [&](const auto& block) { return block.get() == header; });
  function.blocks.insert(header_position, preheader);
  loop.preheader = preheader.get();
  return loop.preheader;
}

}  // namespace Pass
}  // namespace IR
