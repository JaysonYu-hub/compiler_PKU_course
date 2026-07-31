#pragma once

#include "../custom_ir.hpp"
#include <algorithm>
#include <cassert>
#include <memory>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace IR {
namespace Pass {

class DCE {
public:
  // 清理不可达基本块
  void RemoveUnreachableBlocks(IRFunction& function) {
    if (function.blocks.empty()) return;

    IRBasicBlock* entry = function.blocks[0].get();
    std::unordered_set<IRBasicBlock*> reachable;
    std::vector<IRBasicBlock*> worklist = {entry};
    reachable.insert(entry);

    size_t head = 0;
    while (head < worklist.size()) {
      IRBasicBlock* curr = worklist[head++];
      for (IRBasicBlock* succ : curr->GetSuccessors()) {
        if (succ && reachable.insert(succ).second) {
          worklist.push_back(succ);
        }
      }
    }

    if (reachable.size() == function.blocks.size()) return;

    std::vector<std::shared_ptr<IRBasicBlock>> new_blocks;
    new_blocks.reserve(reachable.size());
    for (auto& block : function.blocks) {
      if (reachable.count(block.get())) {
        new_blocks.push_back(block);
      }
    }
    function.blocks = std::move(new_blocks);
  }

  // 消除唯一前驱基本块的参数
  void RemoveSinglePreParams(IRFunction& func) {
    bool changed = true;
    while (changed) {
      changed = false;
      for (auto& block : func.blocks) {
        auto preds = func.GetPredecessors(block.get());
        if (preds.size() != 1) continue;

        IRBasicBlock* P = preds[0];
        if (block->params.empty()) continue;
        if (P->insts.empty()) continue;

        auto jmp = dyn_cast<IRJump>(P->insts.back().get());
        if (!jmp || jmp->GetTarget().get() != block.get()) continue;

        auto args = jmp->GetArgs();
        if (args.size() != block->params.size()) continue;

        // 将形参在整个函数内的 Use 替换为前驱传入的实参
        for (size_t i = 0; i < block->params.size(); ++i) {
          func.ReplaceAllUses(block->params[i].get(), args[i]);
        }

        block->params.clear();
        jmp->SetArgs({});
        changed = true;
      }
    }
  }
  void OptimizeFunction(IRFunction& func) {
    for (auto& block : func.blocks) {
      if (block->insts.empty()) continue;
      auto& end_inst = block->insts.back();
      if (isa<IRBranch>(end_inst)) {
        auto bran = cast<IRBranch>(end_inst);
        if (isa<IRConstant>(bran->GetCond())) {
          auto cond = cast<IRConstant>(bran->GetCond());
          bool is_float = (cond->GetType() == IRType::IR_FLOAT);
          bool is_true = true;

          if (is_float) {
            is_true = (std::get<float>(cond->GetValue()) != 0.0f);
          } else {
            is_true = (std::get<int32_t>(cond->GetValue()) != 0);
          }

          block->insts.pop_back();
          if (is_true) {
            auto jump = std::make_shared<IRJump>(bran->GetThenTarget(), bran->GetThenArgs());
            block->AddInstruction(jump);
          } else {
            auto jump = std::make_shared<IRJump>(bran->GetElseTarget(), bran->GetElseArgs());
            block->AddInstruction(jump);
          }
        }
      }
    }

    RemoveUnreachableBlocks(func);

    RemoveSinglePreParams(func);
  }

  void Run(IRProgram* program) {
    if (!program) return;
    for (auto& func : program->funcs) {
      if (func) {
        OptimizeFunction(*func);
      }
    }
  }
};

}
}