#pragma once

#include "../analysis/function_summary.hpp"
#include "../custom_ir.hpp"
#include "tool.hpp"
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace IR {
namespace Pass {

// 基本块内的保守内存优化：
//   1. store p, v; load p  -> v
//   2. store p, a; store p, b -> 删除第一条 store（中间没有内存观察）
// 这里只使用精确指针身份和 AliasAnalysis 的保守结论，不跨基本块传播。
class LocalMemoryOptimization {
private:
  static std::shared_ptr<IRValue> Resolve(
      std::shared_ptr<IRValue> value,
      const std::unordered_map<const IRValue*, std::shared_ptr<IRValue>>&
          replacements) {
    std::unordered_set<const IRValue*> visited;
    while (value && replacements.count(value.get()) != 0 &&
           visited.insert(value.get()).second)
      value = replacements.at(value.get());
    return value;
  }

  static void InvalidateAliasingValues(
      const IRValue* pointer, std::size_t access_size,
      std::unordered_map<const IRValue*, std::shared_ptr<IRValue>>& available) {
    for (auto iterator = available.begin(); iterator != available.end();) {
      const std::size_t known_size =
          iterator->second && iterator->second->GetType() == IRType::IR_POINTER
              ? 8
              : 4;
      if (AliasAnalysis::AliasMemoryAccess(pointer, access_size,
                                           iterator->first, known_size) !=
          AliasResult::NoAlias)
        iterator = available.erase(iterator);
      else
        ++iterator;
    }
  }

public:
  bool OptimizeFunction(
      IRFunction& function,
      const Analysis::FunctionSummaryAnalysis& function_summaries) {
    bool changed = false;
    for (const auto& block : function.blocks) {
      std::unordered_map<const IRValue*, std::shared_ptr<IRValue>> available;
      std::unordered_map<const IRValue*, const IRStore*> last_unobserved_store;
      std::unordered_map<const IRValue*, std::shared_ptr<IRValue>> replacements;
      std::unordered_set<const IRInstruction*> removed;

      for (const auto& instruction : block->insts) {
        // 当前块中被转发的 load 先规范化到代表值，后续地址和值分析才能
        // 看到真实操作数。
        for (const auto& use : instruction->GetUses()) {
          if (!use) continue;
          auto canonical = Resolve(use, replacements);
          if (canonical.get() != use.get())
            instruction->ReplaceUse(use.get(), std::move(canonical));
        }

        if (const auto* store = dyn_cast<IRStore>(instruction.get())) {
          const IRValue* pointer = store->GetPtr().get();

          // 新 store 覆盖相同精确地址；若旧 store 之后没有 load/call，旧值
          // 从未被观察，可以删除。
          const auto previous = last_unobserved_store.find(pointer);
          if (previous != last_unobserved_store.end()) {
            removed.insert(previous->second);
            changed = true;
          }

          const std::size_t store_size =
              store->GetVal()->GetType() == IRType::IR_POINTER ? 8 : 4;
          InvalidateAliasingValues(pointer, store_size, available);
          available[pointer] = store->GetVal();
          last_unobserved_store[pointer] = store;
          continue;
        }

        if (const auto* load = dyn_cast<IRLoad>(instruction.get())) {
          const IRValue* pointer = load->GetPtr().get();
          const auto known = available.find(pointer);
          if (known != available.end() && known->second &&
              known->second->GetType() == load->GetType()) {
            replacements[instruction.get()] =
                Resolve(known->second, replacements);
            removed.insert(instruction.get());
            changed = true;
          } else {
            available[pointer] = instruction;
          }

          // 一个未知别名的 load 可能观察任何待删除 store。为保证正确性，
          // 当前第一版在每次 load 后结束所有 dead-store 候选链。
          last_unobserved_store.clear();
          continue;
        }

        if (const auto* call = dyn_cast<IRCall>(instruction.get())) {
          const IRFunction* callee = function_summaries.FindFunction(*call);
          const Analysis::FunctionSummary* summary =
              callee ? function_summaries.Get(callee) : nullptr;
          // 未知调用最保守；已知调用则只根据摘要失效必要事实。只读函数
          // 不会破坏 available value，纯函数也不会终止 dead-store 链。
          if (!summary || summary->writes_memory) available.clear();
          if (!summary || summary->reads_memory)
            last_unobserved_store.clear();
          continue;
        }
        if (isa<IRZeroInit>(instruction)) {
          available.clear();
          last_unobserved_store.clear();
        }
      }

      if (removed.empty()) continue;
      std::vector<std::shared_ptr<IRInstruction>> kept;
      kept.reserve(block->insts.size() - removed.size());
      for (const auto& instruction : block->insts) {
        if (removed.count(instruction.get()) == 0) kept.push_back(instruction);
      }
      block->insts = std::move(kept);
    }
    return changed;
  }

  bool Run(IRProgram* program) {
    if (!program) return false;
    Analysis::FunctionSummaryAnalysis function_summaries;
    function_summaries.Run(*program);
    bool changed = false;
    for (auto& function : program->funcs) {
      if (function)
        changed |= OptimizeFunction(*function, function_summaries);
    }
    return changed;
  }
};

}  // namespace Pass
}  // namespace IR
