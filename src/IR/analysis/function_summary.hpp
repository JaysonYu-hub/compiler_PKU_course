#pragma once

#include "../custom_ir.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace IR {
namespace Analysis {

// 函数摘要只记录“事实”，不直接修改 IR。
//
// 内联、LICM 和内存优化可以复用这些信息。分析采用保守策略：
// 只要调用目标未知或是外部函数，就认为它可能读写内存并产生可观察行为。
struct FunctionSummary {
  const IRFunction* function = nullptr;
  std::size_t block_count = 0;
  std::size_t instruction_count = 0;
  std::size_t call_count = 0;

  bool reads_memory = false;
  bool writes_memory = false;
  bool has_external_call = false;
  bool has_observable_effect = false;
  bool recursive = false;

  // “无副作用”比 IRInstruction::HasSideEffect 更严格：它同时排除读取内存，
  // 因为未知地址上的 load 也不能随意复制或移动。
  bool IsEffectFree() const {
    return !reads_memory && !writes_memory && !has_external_call &&
           !has_observable_effect;
  }
};

class FunctionSummaryAnalysis {
 public:
  void Run(const IRProgram& program) {
    summaries_.clear();
    functions_by_name_.clear();
    callees_.clear();

    for (const auto& function : program.funcs) {
      if (!function) continue;
      functions_by_name_[function->name] = function.get();
      FunctionSummary summary;
      summary.function = function.get();
      summary.block_count = function->blocks.size();
      summaries_.emplace(function.get(), summary);
    }

    CollectDirectFacts();
    MarkRecursiveFunctions();
    PropagateCalleeFacts();
  }

  const FunctionSummary* Get(const IRFunction* function) const {
    const auto found = summaries_.find(function);
    return found == summaries_.end() ? nullptr : &found->second;
  }

  const IRFunction* FindFunction(const IRCall& call) const {
    return FindFunctionByOperand(call.GetFunc().get());
  }

  const std::unordered_map<const IRFunction*, FunctionSummary>&
  GetAllSummaries() const {
    return summaries_;
  }

 private:
  static std::string NormalizeFunctionName(std::string name) {
    if (!name.empty() && name.front() == '@') name.erase(name.begin());
    return name;
  }

  const IRFunction* FindFunctionByOperand(const IRValue* value) const {
    if (!value) return nullptr;
    const std::string name = NormalizeFunctionName(value->GetOperandName());
    const auto found = functions_by_name_.find(name);
    return found == functions_by_name_.end() ? nullptr : found->second;
  }

  void CollectDirectFacts() {
    for (auto& item : summaries_) {
      const IRFunction* function = item.first;
      FunctionSummary& summary = item.second;
      auto& edges = callees_[function];

      for (const auto& block : function->blocks) {
        if (!block) continue;
        for (const auto& inst : block->insts) {
          if (!inst || isa<IRComment>(inst)) continue;
          ++summary.instruction_count;

          if (isa<IRLoad>(inst)) {
            summary.reads_memory = true;
          } else if (isa<IRStore>(inst) || isa<IRZeroInit>(inst)) {
            summary.writes_memory = true;
            summary.has_observable_effect = true;
          } else if (const auto* call = dyn_cast<IRCall>(inst.get())) {
            ++summary.call_count;
            const IRFunction* callee = FindFunction(*call);
            if (callee) {
              edges.push_back(callee);
            } else {
              // SysY 运行库调用属于 I/O、计时或数组输入输出。对未知外部
              // 调用同样采用最保守结论，避免后续错误删除或移动调用。
              summary.reads_memory = true;
              summary.writes_memory = true;
              summary.has_external_call = true;
              summary.has_observable_effect = true;
            }
          }
        }
      }
    }
  }

  void MarkRecursiveFunctions() {
    // Tarjan SCC：一个多节点 SCC，或带自环的单节点 SCC，都是递归调用环。
    std::unordered_map<const IRFunction*, int> index;
    std::unordered_map<const IRFunction*, int> lowlink;
    std::unordered_set<const IRFunction*> on_stack;
    std::vector<const IRFunction*> stack;
    int next_index = 0;

    const auto visit = [&](const auto& self, const IRFunction* function) -> void {
      index[function] = next_index;
      lowlink[function] = next_index;
      ++next_index;
      stack.push_back(function);
      on_stack.insert(function);

      const auto edge_it = callees_.find(function);
      if (edge_it != callees_.end()) {
        for (const IRFunction* callee : edge_it->second) {
          if (!index.count(callee)) {
            self(self, callee);
            lowlink[function] =
                std::min(lowlink[function], lowlink[callee]);
          } else if (on_stack.count(callee)) {
            lowlink[function] = std::min(lowlink[function], index[callee]);
          }
        }
      }

      if (lowlink[function] != index[function]) return;

      std::vector<const IRFunction*> component;
      while (!stack.empty()) {
        const IRFunction* member = stack.back();
        stack.pop_back();
        on_stack.erase(member);
        component.push_back(member);
        if (member == function) break;
      }

      bool recursive = component.size() > 1;
      if (!recursive && !component.empty()) {
        const IRFunction* only = component.front();
        const auto only_edges = callees_.find(only);
        if (only_edges != callees_.end()) {
          recursive = std::find(only_edges->second.begin(),
                                only_edges->second.end(), only) !=
                      only_edges->second.end();
        }
      }
      if (recursive) {
        for (const IRFunction* member : component) {
          summaries_.at(member).recursive = true;
        }
      }
    };

    for (const auto& item : summaries_) {
      if (!index.count(item.first)) visit(visit, item.first);
    }
  }

  void PropagateCalleeFacts() {
    // 摘要中的布尔量只会从 false 变为 true，因此有限轮后必然收敛。
    bool changed = true;
    while (changed) {
      changed = false;
      for (auto& item : summaries_) {
        FunctionSummary& caller = item.second;
        const auto edge_it = callees_.find(item.first);
        if (edge_it == callees_.end()) continue;

        for (const IRFunction* callee_function : edge_it->second) {
          const FunctionSummary& callee = summaries_.at(callee_function);
          const bool old_reads = caller.reads_memory;
          const bool old_writes = caller.writes_memory;
          const bool old_external = caller.has_external_call;
          const bool old_effect = caller.has_observable_effect;

          caller.reads_memory |= callee.reads_memory;
          caller.writes_memory |= callee.writes_memory;
          caller.has_external_call |= callee.has_external_call;
          caller.has_observable_effect |= callee.has_observable_effect;

          changed |= old_reads != caller.reads_memory ||
                     old_writes != caller.writes_memory ||
                     old_external != caller.has_external_call ||
                     old_effect != caller.has_observable_effect;
        }
      }
    }
  }

  std::unordered_map<std::string, const IRFunction*> functions_by_name_;
  std::unordered_map<const IRFunction*, FunctionSummary> summaries_;
  std::unordered_map<const IRFunction*, std::vector<const IRFunction*>>
      callees_;
};

}  // namespace Analysis
}  // namespace IR
