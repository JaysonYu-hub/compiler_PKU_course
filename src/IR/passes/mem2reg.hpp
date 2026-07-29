#pragma once
#include "../custom_ir.hpp"
#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace IR {

class MemToReg {
private:
  std::unordered_set<std::string> allow2reg;

  // 支配分析相关成员
  std::vector<IRBasicBlock*> rpo_order;
  std::unordered_map<IRBasicBlock*, int> rpo_index;
  std::vector<int> idom; // idom[rpo_idx] = parent rpo_idx
  std::unordered_map<IRBasicBlock*, IRBasicBlock*> idom_map;
  std::unordered_map<IRBasicBlock*, std::vector<IRBasicBlock*>> dom_children;
  std::unordered_map<IRBasicBlock*, std::unordered_set<IRBasicBlock*>> dom_frontier;

public:
  int intersect(int b1, int b2) {
    int finger1 = b1;
    int finger2 = b2;
    while (finger1 != finger2) {
      while (finger1 > finger2) {
        finger1 = idom[finger1];
      }
      while (finger2 > finger1) {
        finger2 = idom[finger2];
      }
    }
    return finger1;
  }

  // 清理不可达基本块 (Unreachable Code Removal)
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

  // 建立支配树与支配边界
  void createDominTree(IRFunction& function) {
    // 预先清理不可达基本块，防止陷入死循环
    RemoveUnreachableBlocks(function);

    rpo_order.clear();
    rpo_index.clear();
    idom.clear();
    idom_map.clear();
    dom_children.clear();
    dom_frontier.clear();

    if (function.blocks.empty()) return;

    IRBasicBlock* entry = function.blocks[0].get();

    // 1. DFS 计算后序遍历，反转得到 RPO 顺序
    std::vector<IRBasicBlock*> post_order;
    std::unordered_set<IRBasicBlock*> visited;

    std::function<void(IRBasicBlock*)> dfs = [&](IRBasicBlock* b) {
      visited.insert(b);
      for (IRBasicBlock* succ : b->GetSuccessors()) {
        if (succ && visited.find(succ) == visited.end()) {
          dfs(succ);
        }
      }
      post_order.push_back(b);
    };
    dfs(entry);

    for (auto it = post_order.rbegin(); it != post_order.rend(); ++it) {
      rpo_index[*it] = static_cast<int>(rpo_order.size());
      rpo_order.push_back(*it);
    }

    int n = static_cast<int>(rpo_order.size());
    idom.assign(n, -1); // -1 表示 Undefined
    idom[0] = 0;        // Entry 节点的 idom 为自己

    // 前驱节点缓存 (按 RPO 索引)
    std::vector<std::vector<int>> preds(n);
    for (int i = 0; i < n; ++i) {
      IRBasicBlock* b = rpo_order[i];
      for (IRBasicBlock* p : function.GetPredecessors(b)) {
        auto it = rpo_index.find(p);
        if (it != rpo_index.end()) {
          preds[i].push_back(it->second);
        }
      }
    }

    // 2. 数据流迭代计算 idom
    bool changed = true;
    while (changed) {
      changed = false;
      for (int b = 1; b < n; ++b) {
        // 寻找 b 的第一个已计算 idom 的前驱节点
        int new_idom = -1;
        for (int p : preds[b]) {
          if (idom[p] != -1) {
            new_idom = p;
            break;
          }
        }

        if (new_idom != -1) {
          // 与 b 的其他已刷新的前驱求 LCA
          for (int p : preds[b]) {
            if (p != new_idom && idom[p] != -1) {
              new_idom = intersect(p, new_idom);
            }
          }

          if (idom[b] != new_idom) {
            idom[b] = new_idom;
            changed = true;
          }
        }
      }
    }

    // 3. 填充 idom_map 与 dom_children 树关系
    for (int i = 1; i < n; ++i) {
      if (idom[i] != -1) {
        IRBasicBlock* b = rpo_order[i];
        IRBasicBlock* parent = rpo_order[idom[i]];
        idom_map[b] = parent;
        dom_children[parent].push_back(b);
      }
    }

    // 4. 计算支配边界 (Dominance Frontier)
    for (int i = 0; i < n; ++i) {
      IRBasicBlock* b = rpo_order[i];
      if (preds[i].size() >= 2) {
        for (int p : preds[i]) {
          int runner = p;
          while (runner != -1 && runner != idom[i]) {
            dom_frontier[rpo_order[runner]].insert(b);
            runner = idom[runner];
          }
        }
      }
    }
  }

  bool IsPromotable(const IRFunction& function, const IRAlloc* alloc) {
    if (!alloc || alloc->IsArray()) return false;
    for (const auto& block : function.blocks) {
      for (const auto& inst : block->insts) {
        for (const auto& use : inst->GetUses()) {
          if (use.get() != alloc) continue;
          if (auto* load = dyn_cast<IRLoad>(inst.get())) {
            if (load->GetPtr().get() != alloc) return false;
          } else if (auto* store = dyn_cast<IRStore>(inst.get())) {
            if (store->GetPtr().get() != alloc) return false;
          } else {
            return false;
          }
        }
      }
    }
    return true;
  }

  void PromoteFunction(IRFunction& function) {
    if (function.blocks.empty()) return;

    // 建立支配树
    createDominTree(function);

    // 筛选出所有可提升的标量 alloc 指令
    std::vector<std::shared_ptr<IRAlloc>> allocas;
    for (const auto& block : function.blocks) {
      for (const auto& inst : block->insts) {
        if (isa<IRAlloc>(inst)) {
          auto alloc = cast<IRAlloc>(inst);
          if (IsPromotable(function, alloc.get())) {
            allocas.push_back(alloc);
          }
        }
      }
    }

    if (allocas.empty()) return;

    std::unordered_map<const IRValue*, std::shared_ptr<IRValue>> replacements;
    auto resolve = [&](std::shared_ptr<IRValue> val) -> std::shared_ptr<IRValue> {
      while (val && replacements.count(val.get())) {
        val = replacements[val.get()];
      }
      return val;
    };

    int param_counter = 0;

    for (const auto& alloc : allocas) {
      // IDF 计算 + 插入 BlockParam
      std::unordered_set<IRBasicBlock*> def_blocks;
      for (const auto& block : function.blocks) {
        for (const auto& inst : block->insts) {
          if (auto store = dyn_cast<IRStore>(inst.get())) {
            if (store->GetPtr().get() == alloc.get()) {
              def_blocks.insert(block.get());
              break;
            }
          }
        }
      }

      std::unordered_set<IRBasicBlock*> has_param_blocks;
      std::vector<IRBasicBlock*> worklist(def_blocks.begin(), def_blocks.end());
      std::unordered_set<IRBasicBlock*> worklist_set(def_blocks.begin(), def_blocks.end());

      size_t head = 0;
      while (head < worklist.size()) {
        IRBasicBlock* block = worklist[head++];
        auto it_front = dom_frontier.find(block);
        if (it_front == dom_frontier.end()) continue;

        for (IRBasicBlock* frontier_block : it_front->second) {
          if (has_param_blocks.find(frontier_block) == has_param_blocks.end()) {
            has_param_blocks.insert(frontier_block);
            if (worklist_set.find(frontier_block) == worklist_set.end()) {
              worklist_set.insert(frontier_block);
              worklist.push_back(frontier_block);
            }
          }
        }
      }

      std::unordered_map<IRBasicBlock*, std::shared_ptr<IRBlockParam>> params_map;
      for (IRBasicBlock* block : has_param_blocks) {
        std::string param_name = alloc->GetName() + ".m2r." + std::to_string(param_counter++);
        auto param = block->AddParam(param_name, alloc->GetAllocatedType());
        params_map[block] = param;
      }

      // 支配树 DFS 重命名
      std::vector<std::shared_ptr<IRValue>> values_stack;
      std::shared_ptr<IRValue> default_val;
      if (alloc->GetAllocatedType() == IRType::IR_FLOAT) {
        default_val = std::make_shared<IRConstant>(0.0f);
      } else {
        default_val = std::make_shared<IRConstant>(int32_t(0));
      }
      values_stack.push_back(default_val);

      std::function<void(IRBasicBlock*)> Rename = [&](IRBasicBlock* block) {
        size_t pushed_count = 0;

        auto it_p = params_map.find(block);
        if (it_p != params_map.end()) {
          values_stack.push_back(it_p->second);
          pushed_count++;
        }

        std::vector<std::shared_ptr<IRInstruction>> new_insts;
        new_insts.reserve(block->insts.size());

        for (auto& inst : block->insts) {
          if (inst.get() == alloc.get()) continue;

          if (auto store = dyn_cast<IRStore>(inst.get())) {
            if (store->GetPtr().get() == alloc.get()) {
              auto latest_val = resolve(store->GetVal());
              values_stack.push_back(latest_val);
              pushed_count++;
              continue;
            }
          }

          if (auto load = dyn_cast<IRLoad>(inst.get())) {
            if (load->GetPtr().get() == alloc.get()) {
              replacements[load] = resolve(values_stack.back());
              continue;
            }
          }

          new_insts.push_back(inst);
        }

        block->insts = std::move(new_insts);

        if (!block->insts.empty()) {
          auto terminator = block->insts.back();
          auto top_val = resolve(values_stack.back());

          if (auto jmp = dyn_cast<IRJump>(terminator.get())) {
            if (auto succ_block = dyn_cast<IRBasicBlock>(jmp->GetTarget().get())) {
              if (params_map.count(succ_block)) {
                auto args = jmp->GetArgs();
                args.push_back(top_val);
                jmp->SetArgs(std::move(args));
              }
            }
          } else if (auto br = dyn_cast<IRBranch>(terminator.get())) {
            if (auto then_block = dyn_cast<IRBasicBlock>(br->GetThenTarget().get())) {
              if (params_map.count(then_block)) {
                auto args = br->GetThenArgs();
                args.push_back(top_val);
                br->SetThenArgs(std::move(args));
              }
            }
            if (auto else_block = dyn_cast<IRBasicBlock>(br->GetElseTarget().get())) {
              if (params_map.count(else_block)) {
                auto args = br->GetElseArgs();
                args.push_back(top_val);
                br->SetElseArgs(std::move(args));
              }
            }
          }
        }

        auto it_child = dom_children.find(block);
        if (it_child != dom_children.end()) {
          for (IRBasicBlock* child : it_child->second) {
            Rename(child);
          }
        }

        while (pushed_count > 0) {
          values_stack.pop_back();
          pushed_count--;
        }
      };

      Rename(function.blocks[0].get());
    }

    // 替换全局 Use 点
    for (auto& block : function.blocks) {
      for (auto& inst : block->insts) {
        for (auto& use : inst->GetUses()) {
          if (use) {
            auto resolved_use = resolve(use);
            if (resolved_use != use) {
              inst->ReplaceUse(use.get(), resolved_use);
            }
          }
        }
      }
    }
  }

  void Run(IRProgram *program) {
    if (!program) return;
    for (auto &func : program->funcs) {
      if (func) {
        PromoteFunction(*func);
      }
    }
  }
};

}