#pragma once

#include "../custom_ir.hpp"
#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace IR {
namespace Pass {

class DominatorTree {
public:
  std::vector<IRBasicBlock*> rpo_order;
  std::unordered_map<IRBasicBlock*, int> rpo_index;
  std::vector<int> idom; // idom[rpo_idx] = parent rpo_idx
  std::unordered_map<IRBasicBlock*, IRBasicBlock*> idom_map;
  std::unordered_map<IRBasicBlock*, std::vector<IRBasicBlock*>> dom_children;
  std::unordered_map<IRBasicBlock*, std::unordered_set<IRBasicBlock*>> dom_frontier;

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

  // 清理不可达基本块
  static void RemoveUnreachableBlocks(IRFunction& function) {
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
  void Analyze(IRFunction& function, bool remove_unreachable = true) {
    if (remove_unreachable) RemoveUnreachableBlocks(function);

    rpo_order.clear();
    rpo_index.clear();
    idom.clear();
    idom_map.clear();
    dom_children.clear();
    dom_frontier.clear();

    if (function.blocks.empty()) return;

    IRBasicBlock* entry = function.blocks[0].get();

    // DFS 计算后序遍历，反转得到 RPO 顺序
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

    // 前驱节点缓存(按RPO索引)
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

    // 数据流迭代计算 idom
    bool changed = true;
    while (changed) {
      changed = false;
      for (int b = 1; b < n; ++b) {
        int new_idom = -1;
        for (int p : preds[b]) {
          if (idom[p] != -1) {
            new_idom = p;
            break;
          }
        }

        if (new_idom != -1) {
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

    // 填充 idom_map 与 dom_children 树关系
    for (int i = 1; i < n; ++i) {
      if (idom[i] != -1) {
        IRBasicBlock* b = rpo_order[i];
        IRBasicBlock* parent = rpo_order[idom[i]];
        idom_map[b] = parent;
        dom_children[parent].push_back(b);
      }
    }

    // 计算支配边界
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

  // 判断基本块 a 是否支配 b
  bool Dominates(IRBasicBlock* a, IRBasicBlock* b) const {
    if (a == b) return true;
    IRBasicBlock* curr = b;
    while (curr) {
      auto it = idom_map.find(curr);
      if (it == idom_map.end() || it->second == curr) break;
      curr = it->second;
      if (curr == a) return true;
    }
    return false;
  }
};

enum class AliasResult {
  NoAlias,
  MayAlias,
  MustAlias
};

class AliasAnalysis {
public: 
  static const IRValue* GetBaseValue(const IRValue* ptr) {
    while (ptr) {
      if (auto gep = dyn_cast<IRGetElemPtr>(ptr)) {
        ptr = gep->GetArr().get();
      } else if (auto gp = dyn_cast<IRGetPtr>(ptr)) {
        ptr = gp->GetPtr().get();
      } else {
        break;
      }
    }
    return ptr;
  }

  static AliasResult Alias(const IRValue* ptr_a, const IRValue* ptr_b) { 
    if (ptr_a == ptr_b) return AliasResult::MustAlias;
    const IRValue* baseA = GetBaseValue(ptr_a);
    const IRValue* baseB = GetBaseValue(ptr_b);
    if (baseA == baseB) return AliasResult::MayAlias;
    // 两个不同的局部变量/全局变量，绝对不重叠
    if ((isa<IRAlloc>(baseA) || isa<IRGlobalAlloc>(baseA)) &&
        (isa<IRAlloc>(baseB) || isa<IRGlobalAlloc>(baseB))) {
      return AliasResult::NoAlias;
    }
    return AliasResult::MayAlias;
  }
};
}
}
