#pragma once

#include "../custom_ir.hpp"
#include "tool.hpp"
#include <algorithm>
#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace IR {
namespace Pass {

// 控制流图简化：折叠确定分支、删除不可达块并合并线性基本块。
class CFGSimplify {
private:
  static bool IsTrue(const IRConstant& constant) {
    if (constant.GetType() == IRType::IR_FLOAT) {
      return std::get<float>(constant.GetValue()) != 0.0f;
    }
    return std::get<int32_t>(constant.GetValue()) != 0;
  }

  static bool SameValue(const std::shared_ptr<IRValue>& lhs,
                        const std::shared_ptr<IRValue>& rhs) {
    if (lhs.get() == rhs.get()) return true;
    if (!isa<IRConstant>(lhs) || !isa<IRConstant>(rhs) ||
        lhs->GetType() != rhs->GetType()) {
      return false;
    }
    // 常量可能由不同 shared_ptr 保存，文本和值类型同时相同即可视为相等。
    return lhs->DumpText() == rhs->DumpText();
  }

  static bool SameArgs(const std::vector<std::shared_ptr<IRValue>>& lhs,
                       const std::vector<std::shared_ptr<IRValue>>& rhs) {
    if (lhs.size() != rhs.size()) return false;
    for (size_t i = 0; i < lhs.size(); ++i) {
      if (!SameValue(lhs[i], rhs[i])) return false;
    }
    return true;
  }

  static bool FoldBranches(IRFunction& function) {
    bool changed = false;
    for (const auto& block : function.blocks) {
      if (block->insts.empty()) continue;
      const auto branch = dynamic_cast_ptr<IRBranch>(block->insts.back());
      if (!branch) continue;

      std::shared_ptr<IRValue> target;
      std::vector<std::shared_ptr<IRValue>> args;
      if (isa<IRConstant>(branch->GetCond())) {
        const bool take_then = IsTrue(*cast<IRConstant>(branch->GetCond()));
        target = take_then ? branch->GetThenTarget() : branch->GetElseTarget();
        args = take_then ? branch->GetThenArgs() : branch->GetElseArgs();
      } else if (branch->GetThenTarget().get() ==
                     branch->GetElseTarget().get() &&
                 SameArgs(branch->GetThenArgs(), branch->GetElseArgs())) {
        // 两条边完全相同，条件值不会影响控制流和块参数。
        target = branch->GetThenTarget();
        args = branch->GetThenArgs();
      }

      if (target) {
        block->insts.back() =
            std::make_shared<IRJump>(std::move(target), std::move(args));
        changed = true;
      }
    }
    return changed;
  }

  static bool MergeOneLinearBlock(IRFunction& function) {
    // 入口块不能被合入前驱。从第二个块开始寻找 P -> B 的线性边。
    for (size_t index = 1; index < function.blocks.size(); ++index) {
      const auto block = function.blocks[index];
      const auto predecessors = function.GetPredecessors(block.get());
      if (predecessors.size() != 1) continue;

      IRBasicBlock* pred = predecessors.front();
      if (!pred || pred == block.get() || pred->insts.empty()) continue;
      auto jump = dyn_cast<IRJump>(pred->insts.back().get());
      if (!jump || jump->GetTarget().get() != block.get()) continue;
      if (jump->GetArgs().size() != block->params.size()) continue;

      // B 的块参数在唯一入边上具有唯一实参，可以直接替换。
      for (size_t i = 0; i < block->params.size(); ++i) {
        function.ReplaceAllUses(block->params[i].get(), jump->GetArgs()[i]);
      }

      pred->insts.pop_back();
      pred->insts.insert(pred->insts.end(), block->insts.begin(),
                         block->insts.end());
      function.blocks.erase(function.blocks.begin() +
                            static_cast<std::ptrdiff_t>(index));
      return true;
    }
    return false;
  }

  // 后端活跃性分析按 blocks 的逆序迭代。CFG 合并或 LICM 插入新块后，
  // 原向量顺序可能与控制流完全相反，使数据流方程需要大量轮次才能收敛。
  // 将可达块恢复为 RPO，既不改变语义，也能稳定后端编译时间。
  static bool MergeLinearBlocks(IRFunction& function) {
    if (function.blocks.size() < 2) return false;

    std::unordered_map<IRBasicBlock*, std::unordered_set<IRBasicBlock*>>
        predecessors;
    std::unordered_set<IRBasicBlock*> active;
    for (const auto& block : function.blocks) active.insert(block.get());
    for (const auto& block : function.blocks) {
      for (IRBasicBlock* successor : block->GetSuccessors()) {
        if (successor) predecessors[successor].insert(block.get());
      }
    }

    IRBasicBlock* entry = function.blocks.front().get();
    std::deque<IRBasicBlock*> candidates;
    for (const auto& block : function.blocks) {
      if (block.get() != entry && predecessors[block.get()].size() == 1)
        candidates.push_back(block.get());
    }

    std::unordered_map<const IRValue*, std::shared_ptr<IRValue>> replacements;
    bool changed = false;
    while (!candidates.empty()) {
      IRBasicBlock* block = candidates.front();
      candidates.pop_front();
      if (!block || block == entry || active.count(block) == 0 ||
          predecessors[block].size() != 1)
        continue;

      IRBasicBlock* predecessor = *predecessors[block].begin();
      if (!predecessor || predecessor == block ||
          active.count(predecessor) == 0 || predecessor->insts.empty())
        continue;
      const auto* jump = dyn_cast<IRJump>(predecessor->insts.back().get());
      if (!jump || jump->GetTarget().get() != block ||
          jump->GetArgs().size() != block->params.size())
        continue;

      // 只记录块参数替换，全部合并后统一扫描 use，避免每合并一个块就
      // ReplaceAllUses 整个函数。
      for (std::size_t index = 0; index < block->params.size(); ++index)
        replacements[block->params[index].get()] = jump->GetArgs()[index];

      predecessor->insts.pop_back();
      predecessor->insts.insert(predecessor->insts.end(), block->insts.begin(),
                                block->insts.end());
      active.erase(block);
      changed = true;

      for (IRBasicBlock* successor : block->GetSuccessors()) {
        if (!successor || active.count(successor) == 0) continue;
        auto& preds = predecessors[successor];
        preds.erase(block);
        preds.insert(predecessor);
        if (successor != entry && preds.size() == 1)
          candidates.push_back(successor);
      }
      if (predecessor != entry && predecessors[predecessor].size() == 1)
        candidates.push_back(predecessor);
    }

    if (!changed) return false;

    auto resolve = [&](std::shared_ptr<IRValue> value) {
      std::unordered_set<const IRValue*> visited;
      while (value && replacements.count(value.get()) != 0 &&
             visited.insert(value.get()).second)
        value = replacements.at(value.get());
      return value;
    };
    for (const auto& block : function.blocks) {
      if (active.count(block.get()) == 0) continue;
      for (const auto& instruction : block->insts) {
        for (const auto& use : instruction->GetUses()) {
          if (!use) continue;
          auto replacement = resolve(use);
          if (replacement.get() != use.get())
            instruction->ReplaceUse(use.get(), std::move(replacement));
        }
      }
    }

    std::vector<std::shared_ptr<IRBasicBlock>> kept;
    kept.reserve(active.size());
    for (const auto& block : function.blocks) {
      if (active.count(block.get()) != 0) kept.push_back(block);
    }
    function.blocks = std::move(kept);
    return true;
  }

  static bool ReorderBlocksRPO(IRFunction& function) {
    if (function.blocks.empty()) return false;

    std::vector<IRBasicBlock*> post_order;
    std::unordered_set<IRBasicBlock*> visited;
    std::function<void(IRBasicBlock*)> dfs = [&](IRBasicBlock* block) {
      if (!block || !visited.insert(block).second) return;
      for (IRBasicBlock* successor : block->GetSuccessors()) dfs(successor);
      post_order.push_back(block);
    };
    dfs(function.blocks.front().get());
    std::reverse(post_order.begin(), post_order.end());
    if (post_order.size() != function.blocks.size()) return false;

    bool changed = false;
    for (size_t i = 0; i < post_order.size(); ++i) {
      changed |= function.blocks[i].get() != post_order[i];
    }
    if (!changed) return false;

    std::unordered_map<IRBasicBlock*, std::shared_ptr<IRBasicBlock>> owners;
    for (const auto& block : function.blocks) owners.emplace(block.get(), block);

    std::vector<std::shared_ptr<IRBasicBlock>> reordered;
    reordered.reserve(post_order.size());
    for (IRBasicBlock* block : post_order) reordered.push_back(owners.at(block));
    function.blocks = std::move(reordered);
    return true;
  }

public:
  bool OptimizeFunction(IRFunction& function) {
    bool changed = false;
    bool local_changed = true;
    while (local_changed) {
      local_changed = false;
      local_changed |= FoldBranches(function);

      const size_t old_size = function.blocks.size();
      DominatorTree::RemoveUnreachableBlocks(function);
      local_changed |= old_size != function.blocks.size();

      // 每次只合并一个块，随后重新计算前驱关系，避免使用失效的 CFG 信息。
      local_changed |= MergeLinearBlocks(function);
      changed |= local_changed;
    }
    changed |= ReorderBlocksRPO(function);
    return changed;
  }

  bool Run(IRProgram* program) {
    if (!program) return false;
    bool changed = false;
    for (auto& function : program->funcs) {
      if (function) changed |= OptimizeFunction(*function);
    }
    return changed;
  }
};

}  // namespace Pass
}  // namespace IR
