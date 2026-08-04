#pragma once

#include "../analysis/function_summary.hpp"
#include "../custom_ir.hpp"
#include "tool.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace IR {
namespace Pass {

// 基本内存位置：常量地址使用 base + byte_offset 统一表示；动态地址只有
// 在指针 SSA 值完全相同时才视为同一位置。
struct MemoryLocation {
  const IRValue* base = nullptr;
  const IRValue* pointer_identity = nullptr;
  std::int64_t byte_offset = 0;
  std::size_t byte_size = 0;
  bool offset_known = false;

  bool operator==(const MemoryLocation& other) const {
    if (byte_size != other.byte_size || base != other.base ||
        offset_known != other.offset_known)
      return false;
    if (offset_known) return byte_offset == other.byte_offset;
    return pointer_identity == other.pointer_identity;
  }
};

struct MemoryLocationHash {
  std::size_t operator()(const MemoryLocation& location) const {
    std::size_t hash = std::hash<const IRValue*>{}(location.base);
    hash ^= std::hash<std::size_t>{}(location.byte_size) + 0x9e3779b9U +
            (hash << 6U) + (hash >> 2U);
    if (location.offset_known)
      hash ^= std::hash<std::int64_t>{}(location.byte_offset) + 0x9e3779b9U +
              (hash << 6U) + (hash >> 2U);
    else
      hash ^= std::hash<const IRValue*>{}(location.pointer_identity) +
              0x9e3779b9U + (hash << 6U) + (hash >> 2U);
    return hash;
  }
};

class LocalMemoryOptimization {
private:
  using FactMap = std::unordered_map<MemoryLocation,
                                     std::shared_ptr<IRValue>,
                                     MemoryLocationHash>;

  static std::size_t AccessSize(IRType type) {
    return type == IRType::IR_POINTER ? 8U : 4U;
  }

  static MemoryLocation MakeLocation(const IRValue* pointer,
                                     std::size_t byte_size) {
    const AliasAnalysis::PointerLocation decomposed =
        AliasAnalysis::GetPointerLocation(pointer);
    return {decomposed.base, pointer, decomposed.byte_offset, byte_size,
            decomposed.offset_known};
  }

  static bool SameKnownValue(const std::shared_ptr<IRValue>& lhs,
                             const std::shared_ptr<IRValue>& rhs) {
    if (lhs.get() == rhs.get()) return true;
    if (!lhs || !rhs || !isa<IRConstant>(lhs) || !isa<IRConstant>(rhs) ||
        lhs->GetType() != rhs->GetType())
      return false;
    return lhs->DumpText() == rhs->DumpText();
  }

  static bool SameFacts(const FactMap& lhs, const FactMap& rhs) {
    if (lhs.size() != rhs.size()) return false;
    for (const auto& [location, value] : lhs) {
      const auto found = rhs.find(location);
      if (found == rhs.end() || !SameKnownValue(value, found->second))
        return false;
    }
    return true;
  }

  static FactMap IntersectFacts(const std::vector<IRBasicBlock*>& predecessors,
                                const std::unordered_map<IRBasicBlock*, FactMap>&
                                    out_facts) {
    if (predecessors.empty()) return {};
    const auto first = out_facts.find(predecessors.front());
    if (first == out_facts.end()) return {};
    FactMap result = first->second;
    for (std::size_t index = 1; index < predecessors.size(); ++index) {
      const auto facts = out_facts.find(predecessors[index]);
      if (facts == out_facts.end()) return {};
      for (auto iterator = result.begin(); iterator != result.end();) {
        const auto candidate = facts->second.find(iterator->first);
        if (candidate == facts->second.end() ||
            !SameKnownValue(iterator->second, candidate->second))
          iterator = result.erase(iterator);
        else
          ++iterator;
      }
    }
    return result;
  }

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

  static void InvalidateAliasing(const IRValue* pointer,
                                 std::size_t access_size, FactMap& facts) {
    for (auto iterator = facts.begin(); iterator != facts.end();) {
      const MemoryLocation& known = iterator->first;
      if (AliasAnalysis::AliasMemoryAccess(pointer, access_size,
                                            known.pointer_identity,
                                            known.byte_size) !=
          AliasResult::NoAlias)
        iterator = facts.erase(iterator);
      else
        ++iterator;
    }
  }

  static void ApplyCallEffects(
      const IRCall& call,
      const Analysis::FunctionSummaryAnalysis& function_summaries,
      FactMap& facts) {
    const IRFunction* callee = function_summaries.FindFunction(call);
    const Analysis::FunctionSummary* summary =
        callee ? function_summaries.Get(callee) : nullptr;
    if (!summary || summary->writes_memory) facts.clear();
  }

  static FactMap TransferBlock(
      const IRBasicBlock& block, FactMap facts,
      const Analysis::FunctionSummaryAnalysis& function_summaries) {
    for (const auto& instruction : block.insts) {
      if (const auto* store = dyn_cast<IRStore>(instruction.get())) {
        const std::size_t size = AccessSize(store->GetVal()->GetType());
        InvalidateAliasing(store->GetPtr().get(), size, facts);
        facts[MakeLocation(store->GetPtr().get(), size)] = store->GetVal();
      } else if (const auto* load = dyn_cast<IRLoad>(instruction.get())) {
        const MemoryLocation location =
            MakeLocation(load->GetPtr().get(), AccessSize(load->GetType()));
        if (facts.find(location) == facts.end()) facts[location] = instruction;
      } else if (const auto* call = dyn_cast<IRCall>(instruction.get())) {
        ApplyCallEffects(*call, function_summaries, facts);
      } else if (isa<IRZeroInit>(instruction)) {
        facts.clear();
      }
    }
    return facts;
  }

  static std::unordered_map<IRBasicBlock*, FactMap> ComputeIncomingFacts(
      IRFunction& function,
      const Analysis::FunctionSummaryAnalysis& function_summaries) {
    std::unordered_map<IRBasicBlock*, std::vector<IRBasicBlock*>> predecessors;
    for (const auto& block : function.blocks) {
      for (IRBasicBlock* successor : block->GetSuccessors()) {
        if (successor) predecessors[successor].push_back(block.get());
      }
    }

    std::unordered_map<IRBasicBlock*, FactMap> in_facts;
    std::unordered_map<IRBasicBlock*, FactMap> out_facts;
    bool changed = true;
    while (changed) {
      changed = false;
      for (std::size_t index = 0; index < function.blocks.size(); ++index) {
        IRBasicBlock* block = function.blocks[index].get();
        FactMap incoming = index == 0
                               ? FactMap{}
                               : IntersectFacts(predecessors[block], out_facts);
        FactMap outgoing =
            TransferBlock(*block, incoming, function_summaries);
        if (!SameFacts(in_facts[block], incoming)) {
          in_facts[block] = std::move(incoming);
          changed = true;
        }
        if (!SameFacts(out_facts[block], outgoing)) {
          out_facts[block] = std::move(outgoing);
          changed = true;
        }
      }
    }
    return in_facts;
  }

  static bool OptimizeBlock(
      IRBasicBlock& block, FactMap facts,
      const Analysis::FunctionSummaryAnalysis& function_summaries,
      std::unordered_map<const IRValue*, std::shared_ptr<IRValue>>&
          replacements) {
    std::unordered_map<MemoryLocation, const IRStore*, MemoryLocationHash>
        last_unobserved_store;
    std::unordered_set<const IRInstruction*> removed;
    bool changed = false;

    for (const auto& instruction : block.insts) {
      for (const auto& use : instruction->GetUses()) {
        if (!use) continue;
        auto canonical = Resolve(use, replacements);
        if (canonical.get() != use.get())
          instruction->ReplaceUse(use.get(), std::move(canonical));
      }

      if (const auto* store = dyn_cast<IRStore>(instruction.get())) {
        const std::size_t size = AccessSize(store->GetVal()->GetType());
        const MemoryLocation location =
            MakeLocation(store->GetPtr().get(), size);
        const auto previous = last_unobserved_store.find(location);
        if (previous != last_unobserved_store.end()) {
          removed.insert(previous->second);
          changed = true;
        }
        InvalidateAliasing(store->GetPtr().get(), size, facts);
        facts[location] = store->GetVal();
        last_unobserved_store[location] = store;
        continue;
      }

      if (const auto* load = dyn_cast<IRLoad>(instruction.get())) {
        const MemoryLocation location =
            MakeLocation(load->GetPtr().get(), AccessSize(load->GetType()));
        const auto known = facts.find(location);
        if (known != facts.end() && known->second &&
            known->second->GetType() == load->GetType()) {
          replacements[instruction.get()] =
              Resolve(known->second, replacements);
          removed.insert(instruction.get());
          changed = true;
        } else {
          facts[location] = instruction;
        }
        // 当前 DSE 第一版在任意 load 后结束所有候选链，避免未知别名读取。
        last_unobserved_store.clear();
        continue;
      }

      if (const auto* call = dyn_cast<IRCall>(instruction.get())) {
        const IRFunction* callee = function_summaries.FindFunction(*call);
        const Analysis::FunctionSummary* summary =
            callee ? function_summaries.Get(callee) : nullptr;
        if (!summary || summary->writes_memory) facts.clear();
        if (!summary || summary->reads_memory)
          last_unobserved_store.clear();
      } else if (isa<IRZeroInit>(instruction)) {
        facts.clear();
        last_unobserved_store.clear();
      }
    }

    if (removed.empty()) return changed;
    std::vector<std::shared_ptr<IRInstruction>> kept;
    kept.reserve(block.insts.size() - removed.size());
    for (const auto& instruction : block.insts) {
      if (removed.count(instruction.get()) == 0) kept.push_back(instruction);
    }
    block.insts = std::move(kept);
    return true;
  }

public:
  bool OptimizeFunction(
      IRFunction& function,
      const Analysis::FunctionSummaryAnalysis& function_summaries) {
    const auto incoming = ComputeIncomingFacts(function, function_summaries);
    std::unordered_map<const IRValue*, std::shared_ptr<IRValue>> replacements;
    bool changed = false;
    for (const auto& block : function.blocks) {
      const auto found = incoming.find(block.get());
      changed |= OptimizeBlock(*block,
                               found == incoming.end() ? FactMap{}
                                                       : found->second,
                               function_summaries, replacements);
    }

    // 跨块删除 load 后，它的使用可能位于任意被支配的后继块。所有局部
    // 变换完成后统一扫描整个函数，解析可能形成的替换链。
    if (!replacements.empty()) {
      for (const auto& block : function.blocks) {
        for (const auto& instruction : block->insts) {
          for (const auto& use : instruction->GetUses()) {
            if (!use) continue;
            auto canonical = Resolve(use, replacements);
            if (canonical.get() != use.get())
              instruction->ReplaceUse(use.get(), std::move(canonical));
          }
        }
      }
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
