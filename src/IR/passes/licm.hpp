#pragma once

#include "../custom_ir.hpp"
#include "tool.hpp"
#include <algorithm>
#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace IR {
namespace Pass {

struct LoopInfo {
  IRBasicBlock* header = nullptr;               // 循环头基本块
  std::unordered_set<IRBasicBlock*> blocks;     // 包含属于该循环的所有基本块
  IRBasicBlock* pre_header = nullptr;           // 循环前置块(用于放置移出的不变量)
};

class LICM {
public:
  // 查找自然循环
  std::vector<LoopInfo> FindLoops(IRFunction& function, const DominatorTree& dom_tree) {
    std::vector<LoopInfo> loops;

    for (const auto& block_ptr : function.blocks) {
      IRBasicBlock* A = block_ptr.get();
      for (IRBasicBlock* B : A->GetSuccessors()) {
        if (!B) continue;

        // 判断回边: 后继B支配当前节点A
        if (dom_tree.Dominates(B, A)) {
          LoopInfo loop;
          loop.header = B;
          loop.blocks.insert(B);
          loop.blocks.insert(A);

          // 从A开始反向搜寻属于该loop的所有节点
          std::vector<IRBasicBlock*> worklist = {A};
          while (!worklist.empty()) {
            IRBasicBlock* curr = worklist.back();
            worklist.pop_back();

            for (IRBasicBlock* pred : function.GetPredecessors(curr)) {
              if (pred && loop.blocks.find(pred) == loop.blocks.end()) {
                loop.blocks.insert(pred);
                worklist.push_back(pred);
              }
            }
          }

          // 寻找外部前驱作为Preheader
          for (IRBasicBlock* pred : function.GetPredecessors(B)) {
            if (loop.blocks.find(pred) == loop.blocks.end()) {
              loop.pre_header = pred;
              break;
            }
          }

          loops.push_back(std::move(loop));
        }
      }
    }
    return loops;
  }

  // 确保循环拥有 Preheader
  IRBasicBlock* EnsurePreheader(IRFunction& function, LoopInfo& loop) {
    IRBasicBlock* B = loop.header;
    if (!B) return nullptr;

    std::vector<IRBasicBlock*> ext_preds;
    for (IRBasicBlock* pred : function.GetPredecessors(B)) {
      if (loop.blocks.find(pred) == loop.blocks.end()) {
        ext_preds.push_back(pred);
      }
    }

    if (ext_preds.empty()) return nullptr;

    // 如果只有一个外部前驱 P，且 P 的唯一后继是 B
    if (ext_preds.size() == 1) {
      IRBasicBlock* P = ext_preds[0];
      auto succs = P->GetSuccessors();
      if (succs.size() == 1 && succs[0] == B) {
        loop.pre_header = P;
        return P;
      }
    }

    // 存在多个外部前驱或前驱有分支，新建专用的 Preheader
    // 不能只通过支配树来判断
    std::string preheader_name = B->name + ".preheader";
    auto preheader = std::make_shared<IRBasicBlock>(preheader_name);
    IRBasicBlock* pre_ptr = preheader.get();

    // 如果 Header B 带有 BlockParam 参数，将形参同步给 preheader
    std::vector<std::shared_ptr<IRValue>> jmp_args;
    for (const auto& param : B->params) {
      auto p_new = pre_ptr->AddParam(param->GetName() + ".pre", param->GetType());
      jmp_args.push_back(p_new);
    }

    auto jmp_to_b = std::make_shared<IRJump>(B->shared_from_this(), jmp_args);
    pre_ptr->AddInstruction(jmp_to_b);
    // 更改来源块相应的跳转
    for (IRBasicBlock* ext_p : ext_preds) {
      if (ext_p->insts.empty()) continue;
      auto& term = ext_p->insts.back();

      if (auto jmp = dyn_cast<IRJump>(term.get())) {
        if (jmp->GetTarget().get() == B) {
          ext_p->insts.back() = std::make_shared<IRJump>(preheader, jmp->GetArgs());
        }
      } else if (auto br = dyn_cast<IRBranch>(term.get())) {
        if (br->GetThenTarget().get() == B) {
          auto new_br = std::make_shared<IRBranch>(br->GetCond(), preheader, br->GetElseTarget(), br->GetThenArgs(), br->GetElseArgs());
          ext_p->insts.back() = new_br;
        } else if (br->GetElseTarget().get() == B) {
          auto new_br = std::make_shared<IRBranch>(br->GetCond(), br->GetThenTarget(), preheader, br->GetThenArgs(), br->GetElseArgs());
          ext_p->insts.back() = new_br;
        }
      }
    }

    auto it_b = std::find_if(function.blocks.begin(), function.blocks.end(),
                             [B](const auto& item) { return item.get() == B; });
    if (it_b != function.blocks.end()) {
      function.blocks.insert(it_b, preheader);
    } else {
      function.blocks.push_back(preheader);
    }

    loop.pre_header = pre_ptr;
    return pre_ptr;
  }

  bool isInvariant(const std::unordered_set<IRBasicBlock*>& blocks, IRInstruction* inst, 
                   const std::unordered_map<const IRValue*, IRBasicBlock*>& def_map, 
                   const std::unordered_set<const IRValue*>& invariant_set) {
    bool is_invariant = true;
    for (const auto& op : inst->GetUses()) {
      if (!op) continue;

      // 常量、全局变量、函数参数均为循环外常数
      if (isa<IRConstant>(op) || isa<IRGlobalAlloc>(op) || isa<IRFuncParam>(op)) {
        continue;
      }

      auto it_def = def_map.find(op.get());
      if (it_def != def_map.end()) {
        // 若操作数的定义在循环外部，或者是已知不变量
        if (blocks.find(it_def->second) == blocks.end() ||
            invariant_set.count(op.get())) {
          continue;
        }
      }

      is_invariant = false;
      break;
    }
    return is_invariant;
  }

  // 对单条 Loop 进行 LICM 优化
  bool OptimizeLoop(IRFunction& function, LoopInfo& loop,
                    const std::unordered_map<const IRValue*, IRBasicBlock*>& def_map) {
    std::vector<const IRValue*> store_insts;
    bool has_call_in_loop = false;

    for (IRBasicBlock* b : loop.blocks) { 
      for (const auto& inst : b->insts) {
        if (inst->inst_type == IRInstructionType::IR_INST_STORE) {
          if (auto store = dyn_cast<IRStore>(inst.get())) {
            store_insts.push_back(store->GetPtr().get());
          }
        } else if (inst->inst_type == IRInstructionType::IR_INST_CALL) {
          has_call_in_loop = true;
        }
      }
    }

    std::unordered_set<const IRValue*> invariant_set;
    // 不动点迭代分析循环不变量
    bool changed = true;
    while (changed) {
      changed = false;

      for (IRBasicBlock* b : loop.blocks) {
        for (const auto& inst : b->insts) {
          if (inst->HasSideEffect()) continue;
          if (invariant_set.count(inst.get())) continue;
          // 对load进行特殊处理
          if (inst->inst_type == IRInstructionType::IR_INST_LOAD) {
            auto load_inst = dyn_cast<IRLoad>(inst.get());
            bool is_invariant = isInvariant(loop.blocks, load_inst, def_map, invariant_set);
            if (!is_invariant) continue;
            auto load_ptr = load_inst->GetPtr();
            bool has_store_alias = false;
            for (auto store_ptr : store_insts) {
              if (AliasAnalysis::Alias(load_ptr.get(), store_ptr) != AliasResult::NoAlias) {
                has_store_alias = true;
                break;
              }
            }
            if (has_store_alias) continue;
            if (has_call_in_loop) {
              const IRValue* base = AliasAnalysis::GetBaseValue(load_ptr.get());
              if (isa<IRGlobalAlloc>(base) || isa<IRFuncParam>(base)) {
                continue;
              }
            }
            invariant_set.insert(inst.get());
            changed = true;
            continue;
          }

          bool is_invariant = isInvariant(loop.blocks, inst.get(), def_map, invariant_set);
          if (is_invariant) {
            invariant_set.insert(inst.get());
            changed = true;
          }
        }
      }
    }

    if (invariant_set.empty()) return false;
    EnsurePreheader(function, loop);

    if (!loop.pre_header) return false;

    // 搜集待外提的不变量指令, 注意保持内部依赖顺序
    std::vector<std::shared_ptr<IRInstruction>> moved_insts;
    moved_insts.push_back(std::make_shared<IRComment>("----- 循环不变量外提 ( "));
    for (IRBasicBlock* b : loop.blocks) {
      std::vector<std::shared_ptr<IRInstruction>> new_insts;
      for (auto& inst : b->insts) {
        if (invariant_set.count(inst.get())) {
          moved_insts.push_back(inst);
        } else {
          new_insts.push_back(inst);
        }
      }
      b->insts = std::move(new_insts);
    }
    moved_insts.push_back(std::make_shared<IRComment>(" ) -----"));

    if (moved_insts.empty()) return false;

    // 将外提指令插入到 pre_header 的终结跳转指令之前
    IRBasicBlock* pre = loop.pre_header;
    if (!pre->insts.empty()) {
      auto term = pre->insts.back();
      pre->insts.pop_back();
      for (auto& mi : moved_insts) {
        pre->insts.push_back(mi);
      }
      pre->insts.push_back(term);
    } else {
      for (auto& mi : moved_insts) {
        pre->insts.push_back(mi);
      }
    }

    return true;
  }

  void Run(IRProgram* program) {
    if (!program) return;
    for (auto& func : program->funcs) {
      if (!func || func->blocks.empty()) continue;

      DominatorTree dom_tree;
      dom_tree.Analyze(*func);

      // 建立Value的定义BasicBlock查找映射表
      std::unordered_map<const IRValue*, IRBasicBlock*> def_map;
      for (const auto& block : func->blocks) {
        for (const auto& param : block->params) {
          def_map[param.get()] = block.get();
        }
        for (const auto& inst : block->insts) {
          def_map[inst.get()] = block.get();
        }
      }

      auto loops = FindLoops(*func, dom_tree);
      for (auto& loop : loops) {
        OptimizeLoop(*func, loop, def_map);
      }
    }
  }
};
}
}