#include "liveness.hpp"
#include "tool.hpp"

#include <algorithm>
#include <vector>

namespace {

// 只把真正的 SSA 虚拟值交给活跃性分析；常量、全局符号和基本块标签
// 不需要占用通用寄存器。
bool GetVirtualName(const IR::IRValue* value, std::string& name) {
  if (!value) return false;
  if (IR::isa<IR::IRNameInstruction>(value) ||
      IR::isa<IR::IRFuncParam>(value) ||
      IR::isa<IR::IRBlockParam>(value)) {
    name = value->GetOperandName();
    return !name.empty();
  }
  if (const auto* call = IR::dyn_cast<IR::IRCall>(value)) {
    if (!call->HasReturnValue()) return false;
    name = call->GetOperandName();
    return !name.empty();
  }
  return false;
}

bool GetDefinition(const IR::IRInstruction* inst, std::string& name) {
  if (!inst) return false;
  if (IR::isa<IR::IRNameInstruction>(inst)) {
    name = inst->GetOperandName();
    return !name.empty();
  }
  if (const auto* call = IR::dyn_cast<IR::IRCall>(inst)) {
    if (!call->HasReturnValue()) return false;
    name = call->GetOperandName();
    return !name.empty();
  }
  return false;
}

std::uint64_t WeightForLoopDepth(int depth) {
  // 每深入一层循环提高 8 倍，最多计算三层，防止异常 CFG 导致权重溢出。
  return std::uint64_t{1} << (3 * std::min(depth, 3));
}

} // namespace

namespace IR {
namespace Pass {

void LiveVarAnalysis::Analyze(IR::IRProgram *program) {
  // Analyze 可能被测试或工具重复调用，必须先清空上一次结果。
  live_out.clear();
  live_in.clear();
  live_out_inst.clear();
  live_in_inst.clear();
  func_intervals.clear();
  func_use_weights.clear();
  func_use_counts.clear();
  func_crosses_calls.clear();
  func_call_cross_counts.clear();
  func_float_values.clear();
  func_loop_depth.clear();
  func_natural_loops.clear();

  bool changed = true;
  while (changed) {
    changed = false;
    for (auto func = program->funcs.rbegin(); func != program->funcs.rend(); ++func) {
      const std::string& func_name = (*func)->name;
      for (auto block = (*func)->blocks.rbegin(); block != (*func)->blocks.rend(); ++block) {
        const std::string block_key = func_name + ":" + (*block)->name;
        const auto old_live_in = live_in[block_key];
        std::unordered_set<std::string> var_kill;
        std::unordered_set<std::string> var_gen;
        CompuateBlockGenKill(*block, var_gen, var_kill);

        auto& block_live_out = live_out[block_key];
        block_live_out.clear();
        for (const auto* successor : (*block)->GetSuccessors()) {
          const std::string successor_key = func_name + ":" + successor->name;
          block_live_out.insert(live_in[successor_key].begin(),
                                live_in[successor_key].end());
        }

        auto new_live_in = block_live_out;
        for (const auto& var : var_kill) new_live_in.erase(var);
        new_live_in.insert(var_gen.begin(), var_gen.end());
        live_in[block_key] = std::move(new_live_in);
        changed |= old_live_in != live_in[block_key];
      }
    }
  }

  // 从块级结果反向扫描，得到每条指令前后的活跃集合。
  for (const auto& func : program->funcs) {
    const std::string& func_name = func->name;
    for (const auto& block : func->blocks) {
      const std::string block_key = func_name + ":" + block->name;
      auto current_live = live_out[block_key];

      for (auto inst_it = block->insts.rbegin(); inst_it != block->insts.rend(); ++inst_it) {
        const auto& inst = *inst_it;
        if (!inst) continue;
        live_out_inst[inst] = current_live;

        std::string def;
        if (GetDefinition(inst.get(), def)) current_live.erase(def);
        for (const auto& use : inst->GetUses()) {
          std::string name;
          if (GetVirtualName(use.get(), name)) current_live.insert(name);
        }
        live_in_inst[inst] = current_live;
      }
    }
  }

  for (const auto& func : program->funcs) {
    const std::string& func_name = func->name;

    // 使用 Cooper 风格的即时支配树，避免为每个基本块维护一个完整的
    // 支配集合。29_long_line 这类上千基本块用例会因此从平方级内存下降
    // 到近似线性规模。false 表示分析不可达块时不修改原 IR。
    DominatorTree dominator_tree;
    dominator_tree.Analyze(*func, false);
    std::unordered_set<IRBasicBlock*> reachable(
        dominator_tree.rpo_order.begin(), dominator_tree.rpo_order.end());

    struct LocalLoop {
      IRBasicBlock* header = nullptr;
      std::unordered_set<IRBasicBlock*> blocks;
    };
    std::vector<LocalLoop> natural_loops;
    for (IRBasicBlock* latch : dominator_tree.rpo_order) {
      for (IRBasicBlock* header : latch->GetSuccessors()) {
        const auto latch_index = dominator_tree.rpo_index.find(latch);
        const auto header_index = dominator_tree.rpo_index.find(header);
        if (!header || latch_index == dominator_tree.rpo_index.end() ||
            header_index == dominator_tree.rpo_index.end()) continue;
        // 回边目标在 RPO 中不会晚于源块。先做这个 O(1) 判断，可跳过
        // 绝大多数普通控制流边，再进行较贵的支配查询。
        if (header_index->second > latch_index->second ||
            !dominator_tree.Dominates(header, latch)) continue;
        std::unordered_set<IRBasicBlock*> loop_blocks = {header, latch};
        std::vector<IRBasicBlock*> reverse_worklist;
        if (latch != header) reverse_worklist.push_back(latch);
        for (size_t head = 0; head < reverse_worklist.size(); ++head) {
          for (IRBasicBlock* pred : func->GetPredecessors(reverse_worklist[head])) {
            if (reachable.count(pred) && loop_blocks.insert(pred).second && pred != header)
              reverse_worklist.push_back(pred);
          }
        }
        const bool duplicate = std::any_of(
            natural_loops.begin(), natural_loops.end(), [&](const LocalLoop& loop) {
              return loop.header == header && loop.blocks == loop_blocks;
            });
        if (!duplicate)
          natural_loops.push_back({header, std::move(loop_blocks)});
      }
    }

    std::unordered_map<IRBasicBlock*, int> block_depth;
    for (IRBasicBlock* block : reachable) block_depth[block] = 0;
    for (const auto& loop : natural_loops)
      for (IRBasicBlock* block : loop.blocks) ++block_depth[block];
    for (const auto& block : func->blocks)
      func_loop_depth[func_name][block->name] = block_depth[block.get()];

    // 暴露自然循环的块集合和唯一外部前驱。后端第五阶段只在存在唯一
    // preheader 时做循环边界切分；多入口循环保持原分配，避免在错误路径
    // 上使用尚未初始化的寄存器值。
    for (const auto& loop : natural_loops) {
      NaturalLoopRegion region;
      region.header = loop.header ? loop.header->name : "";
      region.depth = loop.header ? block_depth[loop.header] : 0;
      for (IRBasicBlock* block : loop.blocks)
        if (block) region.blocks.insert(block->name);

      std::vector<IRBasicBlock*> outside_predecessors;
      if (loop.header) {
        for (IRBasicBlock* pred : func->GetPredecessors(loop.header)) {
          if (pred && loop.blocks.count(pred) == 0)
            outside_predecessors.push_back(pred);
        }
      }
      if (outside_predecessors.size() == 1)
        region.preheader = outside_predecessors.front()->name;
      func_natural_loops[func_name].push_back(std::move(region));
    }

    auto& weights = func_use_weights[func_name];
    auto& counts = func_use_counts[func_name];
    auto& float_values = func_float_values[func_name];
    auto record_value = [&](const IRValue* value, std::uint64_t weight, bool is_use) {
      std::string name;
      if (!GetVirtualName(value, name)) return;
      if (is_use) {
        ++counts[name];
        weights[name] += weight;
      } else {
        // 即使值只被使用一次，也给定义保留一个基础权重。
        weights[name] += 1;
      }
      if (value->GetType() == IRType::IR_FLOAT) float_values.insert(name);
    };

    for (const auto& param : func->params) record_value(param.get(), 1, false);
    for (const auto& block : func->blocks) {
      for (const auto& param : block->params) record_value(param.get(), 1, false);
      const std::uint64_t use_weight = WeightForLoopDepth(block_depth[block.get()]);
      for (const auto& inst : block->insts) {
        record_value(inst.get(), 1, false);
        for (const auto& use : inst->GetUses()) record_value(use.get(), use_weight, true);

        if (IR::isa<IRCall>(inst.get())) {
          std::string def;
          GetDefinition(inst.get(), def);
          for (const auto& var : live_out_inst[inst]) {
            if (var != def) {
              func_crosses_calls[func_name].insert(var);
              ++func_call_cross_counts[func_name][var];
            }
          }
        }
      }
    }

    std::unordered_map<std::string, std::array<long long int, 2>> intervals;
    auto get_interval = [&](const std::string& var) -> std::array<long long int, 2>& {
      return intervals.insert({var, {max_inf, min_inf}}).first->second;
    };
    for (const auto& param : func->params) get_interval(param->GetOperandName())[0] = 0;

    long long instruction_index = 0;
    for (const auto& block : func->blocks) {
      for (const auto& inst : block->insts) {
        const long long in_point = 2 * instruction_index;
        const long long out_point = in_point + 1;
        for (const auto& var : live_in_inst[inst]) {
          auto& interval = get_interval(var);
          interval[0] = std::min(interval[0], in_point);
          interval[1] = std::max(interval[1], in_point);
        }
        for (const auto& var : live_out_inst[inst]) {
          auto& interval = get_interval(var);
          interval[0] = std::min(interval[0], out_point);
          interval[1] = std::max(interval[1], out_point);
        }
        ++instruction_index;
      }
    }
    func_intervals[func_name] = std::move(intervals);
  }
}

void LiveVarAnalysis::CompuateBlockGenKill(
    std::shared_ptr<IR::IRBasicBlock> block,
    std::unordered_set<std::string> &var_gen,
    std::unordered_set<std::string> &var_kill) {
  for (auto inst_it = block->insts.rbegin(); inst_it != block->insts.rend(); ++inst_it) {
    const auto& inst = *inst_it;
    if (!inst) continue;

    std::string def;
    if (GetDefinition(inst.get(), def)) {
      var_kill.insert(def);
      var_gen.erase(def);
    }
    for (const auto& use : inst->GetUses()) {
      std::string name;
      if (GetVirtualName(use.get(), name)) var_gen.insert(name);
    }
  }

  // 块参数在基本块入口处定义，因此必须在反向扫描完指令后再 Kill。
  for (const auto& param : block->params) {
    var_kill.insert(param->GetOperandName());
    var_gen.erase(param->GetOperandName());
  }
}

} // namespace Pass
} // namespace IR
