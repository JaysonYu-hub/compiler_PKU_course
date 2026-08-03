#pragma once

#include "../custom_ir.hpp"
#include "tool.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace IR {
namespace Pass {

// Dead Code Elimination（死代码删除）。
//
// 这里不只删除不可达基本块，还会删除“结果无人使用且没有副作用”的
// SSA 指令。删除一条指令后，它的操作数可能随之变成死值，因此需要
// 反复建立 use-count，直到达到不动点。
class DCE {
private:
  static std::vector<std::shared_ptr<IRValue>> FilterArgs(
      const std::vector<std::shared_ptr<IRValue>>& args,
      const std::vector<bool>& remove) {
    std::vector<std::shared_ptr<IRValue>> result;
    result.reserve(args.size());
    for (size_t i = 0; i < args.size(); ++i) {
      if (i >= remove.size() || !remove[i]) result.push_back(args[i]);
    }
    return result;
  }

  // 块参数相当于 Phi。若参数没有任何使用，需要同时删除所有入边上
  // 对应位置的实参，否则块参数个数与跳转实参个数会不一致。
  static bool RemoveUnusedBlockParams(IRFunction& function) {
    std::unordered_map<const IRValue*, size_t> use_count;
    for (const auto& block : function.blocks) {
      for (const auto& inst : block->insts) {
        for (const auto& use : inst->GetUses()) {
          if (use) ++use_count[use.get()];
        }
      }
    }

    bool changed = false;
    for (const auto& target : function.blocks) {
      if (target->params.empty()) continue;

      std::vector<bool> remove(target->params.size(), false);
      bool has_dead_param = false;
      for (size_t i = 0; i < target->params.size(); ++i) {
        if (use_count[target->params[i].get()] == 0) {
          remove[i] = true;
          has_dead_param = true;
        }
      }
      if (!has_dead_param) continue;

      // 先验证每条入边的实参数量。遇到不完整 IR 时保持保守，不进行修改。
      bool all_edges_valid = true;
      for (const auto& pred : function.blocks) {
        if (pred->insts.empty()) continue;
        const auto& term = pred->insts.back();
        if (auto jump = dyn_cast<IRJump>(term.get())) {
          if (jump->GetTarget().get() == target.get() &&
              jump->GetArgs().size() != target->params.size()) {
            all_edges_valid = false;
          }
        } else if (auto branch = dyn_cast<IRBranch>(term.get())) {
          if (branch->GetThenTarget().get() == target.get() &&
              branch->GetThenArgs().size() != target->params.size()) {
            all_edges_valid = false;
          }
          if (branch->GetElseTarget().get() == target.get() &&
              branch->GetElseArgs().size() != target->params.size()) {
            all_edges_valid = false;
          }
        }
      }
      if (!all_edges_valid) continue;

      for (const auto& pred : function.blocks) {
        if (pred->insts.empty()) continue;
        const auto& term = pred->insts.back();
        if (auto jump = dyn_cast<IRJump>(term.get())) {
          if (jump->GetTarget().get() == target.get()) {
            jump->SetArgs(FilterArgs(jump->GetArgs(), remove));
          }
        } else if (auto branch = dyn_cast<IRBranch>(term.get())) {
          if (branch->GetThenTarget().get() == target.get()) {
            branch->SetThenArgs(FilterArgs(branch->GetThenArgs(), remove));
          }
          if (branch->GetElseTarget().get() == target.get()) {
            branch->SetElseArgs(FilterArgs(branch->GetElseArgs(), remove));
          }
        }
      }

      std::vector<std::shared_ptr<IRBlockParam>> kept;
      kept.reserve(target->params.size());
      for (size_t i = 0; i < target->params.size(); ++i) {
        if (!remove[i]) kept.push_back(target->params[i]);
      }
      target->params = std::move(kept);
      changed = true;
    }
    return changed;
  }

  static bool RemoveDeadInstructions(IRFunction& function) {
    std::unordered_map<const IRValue*, size_t> use_count;
    std::unordered_map<const IRValue*, std::shared_ptr<IRInstruction>>
        instruction_values;
    for (const auto& block : function.blocks) {
      for (const auto& inst : block->insts) {
        instruction_values.emplace(inst.get(), inst);
        for (const auto& use : inst->GetUses()) {
          if (use) ++use_count[use.get()];
        }
      }
    }

    // 删除一条死指令时同步递减其操作数的使用次数，一次工作队列遍历即可
    // 删除整条死链，避免反复全函数扫描产生 O(N^2) 复杂度。
    std::vector<std::shared_ptr<IRInstruction>> worklist;
    for (const auto& [value, instruction] : instruction_values) {
      if (!instruction->HasSideEffect() && use_count[value] == 0)
        worklist.push_back(instruction);
    }

    std::unordered_set<const IRInstruction*> dead;
    while (!worklist.empty()) {
      auto instruction = worklist.back();
      worklist.pop_back();
      if (!instruction || dead.count(instruction.get()) != 0 ||
          instruction->HasSideEffect() || use_count[instruction.get()] != 0)
        continue;

      dead.insert(instruction.get());
      for (const auto& operand : instruction->GetUses()) {
        if (!operand) continue;
        auto count = use_count.find(operand.get());
        if (count == use_count.end() || count->second == 0) continue;
        --count->second;
        if (count->second != 0) continue;
        const auto definition = instruction_values.find(operand.get());
        if (definition != instruction_values.end() &&
            !definition->second->HasSideEffect())
          worklist.push_back(definition->second);
      }
    }

    if (dead.empty()) return false;
    for (const auto& block : function.blocks) {
      std::vector<std::shared_ptr<IRInstruction>> kept;
      kept.reserve(block->insts.size());
      for (const auto& inst : block->insts) {
        if (dead.count(inst.get()) == 0) kept.push_back(inst);
      }
      block->insts = std::move(kept);
    }
    return true;
  }

public:
  bool OptimizeFunction(IRFunction& function) {
    const size_t old_block_count = function.blocks.size();
    DominatorTree::RemoveUnreachableBlocks(function);
    bool changed = function.blocks.size() != old_block_count;

    bool local_changed = true;
    while (local_changed) {
      local_changed = false;
      local_changed |= RemoveUnusedBlockParams(function);
      local_changed |= RemoveDeadInstructions(function);
      changed |= local_changed;
    }
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
