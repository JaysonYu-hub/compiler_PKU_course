#pragma once

#include "../custom_ir.hpp"
#include "cfandcp.hpp"
#include "tool.hpp"
#include <cmath>
#include <cstdint>
#include <deque>
#include <limits>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace IR {
namespace Pass {

// Sparse Conditional Constant Propagation。
//
// SCCP 同时维护两类状态：
//   1. SSA 值是 Unknown、Constant 还是 Overdefined；
//   2. CFG 的某条边是否可执行。
// 因此它能比普通的顺序常量传播更准确地处理块参数（Phi）和死分支。
class SCCP {
private:
  enum class LatticeKind { Unknown, Constant, Overdefined };

  struct LatticeValue {
    LatticeKind kind = LatticeKind::Unknown;
    std::shared_ptr<IRConstant> constant;

    static LatticeValue Unknown() { return {}; }
    static LatticeValue Constant(std::shared_ptr<IRConstant> value) {
      return {LatticeKind::Constant, std::move(value)};
    }
    static LatticeValue Overdefined() {
      return {LatticeKind::Overdefined, nullptr};
    }
  };

  // arm: 0 表示 jump，1 表示 branch 的 then 边，2 表示 else 边。
  struct EdgeKey {
    IRBasicBlock* from = nullptr;
    IRBasicBlock* to = nullptr;
    int arm = 0;

    bool operator==(const EdgeKey& other) const {
      return from == other.from && to == other.to && arm == other.arm;
    }
  };

  struct EdgeHash {
    size_t operator()(const EdgeKey& edge) const {
      const size_t h1 = std::hash<IRBasicBlock*>{}(edge.from);
      const size_t h2 = std::hash<IRBasicBlock*>{}(edge.to);
      return h1 ^ (h2 << 1U) ^ (static_cast<size_t>(edge.arm) << 2U);
    }
  };

  using StateMap = std::unordered_map<const IRValue*, LatticeValue>;
  using EdgeSet = std::unordered_set<EdgeKey, EdgeHash>;

  static bool SameConstant(const std::shared_ptr<IRConstant>& lhs,
                           const std::shared_ptr<IRConstant>& rhs) {
    if (lhs.get() == rhs.get()) return true;
    return lhs && rhs && lhs->GetType() == rhs->GetType() &&
           lhs->DumpText() == rhs->DumpText();
  }

  static bool SameLattice(const LatticeValue& lhs,
                          const LatticeValue& rhs) {
    if (lhs.kind != rhs.kind) return false;
    if (lhs.kind != LatticeKind::Constant) return true;
    return SameConstant(lhs.constant, rhs.constant);
  }

  // SCCP 的 meet 运算只能沿 Unknown -> Constant -> Overdefined 方向前进。
  static LatticeValue Meet(const LatticeValue& lhs,
                           const LatticeValue& rhs) {
    if (lhs.kind == LatticeKind::Overdefined ||
        rhs.kind == LatticeKind::Overdefined) {
      return LatticeValue::Overdefined();
    }
    if (lhs.kind == LatticeKind::Unknown) return rhs;
    if (rhs.kind == LatticeKind::Unknown) return lhs;
    if (SameConstant(lhs.constant, rhs.constant)) return lhs;
    return LatticeValue::Overdefined();
  }

  static bool UpdateState(StateMap& states, const IRValue* value,
                          const LatticeValue& incoming) {
    if (!value) return false;
    const LatticeValue old = states[value];
    const LatticeValue merged = Meet(old, incoming);
    if (SameLattice(old, merged)) return false;
    states[value] = merged;
    return true;
  }

  static LatticeValue GetState(const std::shared_ptr<IRValue>& value,
                               const StateMap& states) {
    if (!value) return LatticeValue::Overdefined();
    if (isa<IRConstant>(value)) {
      return LatticeValue::Constant(cast<IRConstant>(value));
    }
    const auto found = states.find(value.get());
    if (found != states.end()) return found->second;

    // 函数参数、全局地址等运行时输入不是常量；尚未访问到的 SSA 定义和
    // 块参数则保留 Unknown，等待数据流迭代继续求解。
    if (isa<IRFuncParam>(value) || isa<IRGlobalAlloc>(value) ||
        isa<IRSymbol>(value) || isa<IRFunction>(value) ||
        isa<IRFuncDecl>(value)) {
      return LatticeValue::Overdefined();
    }
    return LatticeValue::Unknown();
  }

  static LatticeValue EvaluateCast(const IRCast& cast_inst,
                                   const LatticeValue& operand) {
    if (operand.kind == LatticeKind::Overdefined) {
      return LatticeValue::Overdefined();
    }
    if (operand.kind != LatticeKind::Constant || !operand.constant) {
      return LatticeValue::Unknown();
    }

    if (cast_inst.GetOp() == IRCastOp::INT_TO_FLOAT &&
        operand.constant->GetType() == IRType::IR_INT32) {
      return LatticeValue::Constant(std::make_shared<IRConstant>(
          static_cast<float>(
              std::get<int32_t>(operand.constant->GetValue()))));
    }

    if (cast_inst.GetOp() == IRCastOp::FLOAT_TO_INT &&
        operand.constant->GetType() == IRType::IR_FLOAT) {
      const float value = std::get<float>(operand.constant->GetValue());
      const double wide_value = static_cast<double>(value);
      if (!std::isfinite(value) ||
          wide_value < static_cast<double>(std::numeric_limits<int32_t>::min()) ||
          wide_value > static_cast<double>(std::numeric_limits<int32_t>::max())) {
        return LatticeValue::Overdefined();
      }
      return LatticeValue::Constant(
          std::make_shared<IRConstant>(static_cast<int32_t>(value)));
    }
    return LatticeValue::Overdefined();
  }

  static bool EvaluateInstruction(const std::shared_ptr<IRInstruction>& inst,
                                  StateMap& states) {
    if (isa<IRBinary>(inst)) {
      const auto binary = cast<IRBinary>(inst);
      const auto lhs = GetState(binary->GetLhs(), states);
      const auto rhs = GetState(binary->GetRhs(), states);
      if (lhs.kind == LatticeKind::Constant &&
          rhs.kind == LatticeKind::Constant) {
        auto folded = ConstantFoldAndPropagation::EvaluateBinary(
            binary->GetOp(), lhs.constant.get(), rhs.constant.get());
        return UpdateState(states, inst.get(),
                           folded ? LatticeValue::Constant(std::move(folded))
                                  : LatticeValue::Overdefined());
      }
      if (lhs.kind == LatticeKind::Overdefined ||
          rhs.kind == LatticeKind::Overdefined) {
        return UpdateState(states, inst.get(),
                           LatticeValue::Overdefined());
      }
      return false;
    }

    if (isa<IRCast>(inst)) {
      const auto cast_inst = cast<IRCast>(inst);
      return UpdateState(states, inst.get(),
                         EvaluateCast(*cast_inst,
                                      GetState(cast_inst->GetValue(), states)));
    }

    // 地址、内存读取和有返回值的调用在第一阶段均视为运行时值。
    if (isa<IRAlloc>(inst) || isa<IRLoad>(inst) ||
        isa<IRGetElemPtr>(inst) || isa<IRGetPtr>(inst)) {
      return UpdateState(states, inst.get(), LatticeValue::Overdefined());
    }
    if (isa<IRCall>(inst) && cast<IRCall>(inst)->HasReturnValue()) {
      return UpdateState(states, inst.get(), LatticeValue::Overdefined());
    }
    return false;
  }

  static bool MarkEdge(IRBasicBlock* from, IRBasicBlock* to, int arm,
                       EdgeSet& executable_edges,
                       std::unordered_set<IRBasicBlock*>& executable_blocks) {
    if (!from || !to) return false;
    bool changed = executable_edges.insert({from, to, arm}).second;
    changed |= executable_blocks.insert(to).second;
    return changed;
  }

  static const std::vector<std::shared_ptr<IRValue>>* GetEdgeArgs(
      const EdgeKey& edge,
      std::vector<std::shared_ptr<IRValue>>& scratch) {
    if (!edge.from || edge.from->insts.empty()) return nullptr;
    const auto& term = edge.from->insts.back();
    if (edge.arm == 0) {
      auto jump = dyn_cast<IRJump>(term.get());
      if (!jump || jump->GetTarget().get() != edge.to) return nullptr;
      return &jump->GetArgs();
    }
    auto branch = dyn_cast<IRBranch>(term.get());
    if (!branch) return nullptr;
    if (edge.arm == 1 && branch->GetThenTarget().get() == edge.to) {
      scratch = branch->GetThenArgs();
      return &scratch;
    }
    if (edge.arm == 2 && branch->GetElseTarget().get() == edge.to) {
      scratch = branch->GetElseArgs();
      return &scratch;
    }
    return nullptr;
  }

  static bool PropagateBlockParams(IRFunction& function,
                                   const EdgeSet& executable_edges,
                                   StateMap& states) {
    bool changed = false;
    if (!function.blocks.empty()) {
      // 若入口块具有参数，它们来自函数外部，必须按运行时值处理。
      for (const auto& param : function.blocks.front()->params) {
        changed |= UpdateState(states, param.get(),
                               LatticeValue::Overdefined());
      }
    }

    for (size_t block_index = 1; block_index < function.blocks.size();
         ++block_index) {
      const auto& block = function.blocks[block_index];
      for (size_t param_index = 0; param_index < block->params.size();
           ++param_index) {
        LatticeValue merged = LatticeValue::Unknown();
        bool has_executable_input = false;

        for (const auto& edge : executable_edges) {
          if (edge.to != block.get()) continue;
          has_executable_input = true;
          std::vector<std::shared_ptr<IRValue>> scratch;
          const auto* args = GetEdgeArgs(edge, scratch);
          if (!args || args->size() != block->params.size()) {
            merged = LatticeValue::Overdefined();
            break;
          }
          merged = Meet(merged, GetState((*args)[param_index], states));
        }

        if (has_executable_input) {
          changed |= UpdateState(states, block->params[param_index].get(),
                                 merged);
        }
      }
    }
    return changed;
  }

  static bool ConstantIsTrue(const IRConstant& constant) {
    if (constant.GetType() == IRType::IR_FLOAT) {
      return std::get<float>(constant.GetValue()) != 0.0f;
    }
    return std::get<int32_t>(constant.GetValue()) != 0;
  }

  static void SolveWithWorklist(
      IRFunction& function, StateMap& states, EdgeSet& executable_edges,
      std::unordered_set<IRBasicBlock*>& executable_blocks) {
    // value -> 使用它的基本块。值状态变化时只重算真正受影响的块。
    std::unordered_map<const IRValue*, std::vector<IRBasicBlock*>> user_blocks;
    for (const auto& block : function.blocks) {
      for (const auto& instruction : block->insts) {
        for (const auto& use : instruction->GetUses()) {
          if (use) user_blocks[use.get()].push_back(block.get());
        }
      }
    }

    std::deque<IRBasicBlock*> worklist;
    std::unordered_set<IRBasicBlock*> queued;
    auto enqueue = [&](IRBasicBlock* block) {
      if (block && queued.insert(block).second) worklist.push_back(block);
    };
    auto enqueue_users = [&](const IRValue* value) {
      const auto found = user_blocks.find(value);
      if (found == user_blocks.end()) return;
      for (IRBasicBlock* block : found->second) enqueue(block);
    };

    IRBasicBlock* entry = function.blocks.front().get();
    executable_blocks.insert(entry);
    for (const auto& parameter : entry->params) {
      if (UpdateState(states, parameter.get(), LatticeValue::Overdefined()))
        enqueue_users(parameter.get());
    }
    enqueue(entry);

    auto propagate_edge = [&](IRBasicBlock* from, IRBasicBlock* to, int arm,
                              const std::vector<std::shared_ptr<IRValue>>& args) {
      if (!to) return false;
      const bool edge_added =
          MarkEdge(from, to, arm, executable_edges, executable_blocks);
      if (edge_added) enqueue(to);
      for (std::size_t index = 0; index < to->params.size(); ++index) {
        const LatticeValue incoming =
            args.size() == to->params.size()
                ? GetState(args[index], states)
                : LatticeValue::Overdefined();
        if (UpdateState(states, to->params[index].get(), incoming))
          enqueue_users(to->params[index].get());
      }
      return edge_added;
    };

    auto process_block = [&](IRBasicBlock* block) {
      if (!block || executable_blocks.count(block) == 0) return;
      for (const auto& instruction : block->insts) {
        if (EvaluateInstruction(instruction, states))
          enqueue_users(instruction.get());
      }
      if (block->insts.empty()) return;

      const auto& terminator = block->insts.back();
      if (const auto* jump = dyn_cast<IRJump>(terminator.get())) {
        propagate_edge(block,
                       dyn_cast<IRBasicBlock>(jump->GetTarget().get()), 0,
                       jump->GetArgs());
        return;
      }
      const auto* branch = dyn_cast<IRBranch>(terminator.get());
      if (!branch) return;
      const LatticeValue condition = GetState(branch->GetCond(), states);
      if (condition.kind == LatticeKind::Constant) {
        const bool take_then = ConstantIsTrue(*condition.constant);
        propagate_edge(
            block,
            dyn_cast<IRBasicBlock>((take_then ? branch->GetThenTarget()
                                              : branch->GetElseTarget()).get()),
            take_then ? 1 : 2,
            take_then ? branch->GetThenArgs() : branch->GetElseArgs());
      } else if (condition.kind == LatticeKind::Overdefined) {
        propagate_edge(block,
                       dyn_cast<IRBasicBlock>(branch->GetThenTarget().get()), 1,
                       branch->GetThenArgs());
        propagate_edge(block,
                       dyn_cast<IRBasicBlock>(branch->GetElseTarget().get()), 2,
                       branch->GetElseArgs());
      }
    };

    while (true) {
      while (!worklist.empty()) {
        IRBasicBlock* block = worklist.front();
        worklist.pop_front();
        queued.erase(block);
        process_block(block);
      }

      // Unknown 条件不能证明任一分支不可达；临时不动点后保守激活两边。
      bool forced_progress = false;
      for (const auto& block : function.blocks) {
        if (executable_blocks.count(block.get()) == 0 || block->insts.empty())
          continue;
        const auto* branch = dyn_cast<IRBranch>(block->insts.back().get());
        if (!branch ||
            GetState(branch->GetCond(), states).kind != LatticeKind::Unknown)
          continue;
        forced_progress |= propagate_edge(
            block.get(),
            dyn_cast<IRBasicBlock>(branch->GetThenTarget().get()), 1,
            branch->GetThenArgs());
        forced_progress |= propagate_edge(
            block.get(),
            dyn_cast<IRBasicBlock>(branch->GetElseTarget().get()), 2,
            branch->GetElseArgs());
      }
      if (!forced_progress && worklist.empty()) break;
    }
  }

public:
  bool OptimizeFunction(IRFunction& function) {
    if (function.blocks.empty()) return false;

    StateMap states;
    EdgeSet executable_edges;
    std::unordered_set<IRBasicBlock*> executable_blocks;
    SolveWithWorklist(function, states, executable_edges, executable_blocks);

    if (false) while (true) {
      bool progress = false;

      for (const auto& block : function.blocks) {
        if (!executable_blocks.count(block.get())) continue;
        for (const auto& inst : block->insts) {
          progress |= EvaluateInstruction(inst, states);
        }

        if (block->insts.empty()) continue;
        const auto& term = block->insts.back();
        if (auto jump = dyn_cast<IRJump>(term.get())) {
          progress |= MarkEdge(block.get(),
                               dyn_cast<IRBasicBlock>(jump->GetTarget().get()),
                               0, executable_edges, executable_blocks);
        } else if (auto branch = dyn_cast<IRBranch>(term.get())) {
          const auto condition = GetState(branch->GetCond(), states);
          if (condition.kind == LatticeKind::Constant) {
            const bool take_then = ConstantIsTrue(*condition.constant);
            const auto target = take_then ? branch->GetThenTarget()
                                          : branch->GetElseTarget();
            progress |= MarkEdge(
                block.get(), dyn_cast<IRBasicBlock>(target.get()),
                take_then ? 1 : 2, executable_edges, executable_blocks);
          } else if (condition.kind == LatticeKind::Overdefined) {
            progress |= MarkEdge(
                block.get(),
                dyn_cast<IRBasicBlock>(branch->GetThenTarget().get()), 1,
                executable_edges, executable_blocks);
            progress |= MarkEdge(
                block.get(),
                dyn_cast<IRBasicBlock>(branch->GetElseTarget().get()), 2,
                executable_edges, executable_blocks);
          }
        }
      }

      progress |= PropagateBlockParams(function, executable_edges, states);
      if (progress) continue;

      // 理论上的未定义值可能让分支永久停留在 Unknown。为保证优化保守，
      // 不动点后把这种分支的两条边都标为可执行，再继续求解。
      bool forced_progress = false;
      for (const auto& block : function.blocks) {
        if (!executable_blocks.count(block.get()) || block->insts.empty()) {
          continue;
        }
        auto branch = dyn_cast<IRBranch>(block->insts.back().get());
        if (!branch ||
            GetState(branch->GetCond(), states).kind != LatticeKind::Unknown) {
          continue;
        }
        forced_progress |= MarkEdge(
            block.get(),
            dyn_cast<IRBasicBlock>(branch->GetThenTarget().get()), 1,
            executable_edges, executable_blocks);
        forced_progress |= MarkEdge(
            block.get(),
            dyn_cast<IRBasicBlock>(branch->GetElseTarget().get()), 2,
            executable_edges, executable_blocks);
      }
      if (!forced_progress) break;
    }

    bool changed = false;

    // 先替换块参数，再替换指令结果；后续 DCE 会删除失去用途的定义和实参。
    // 批量收集后只扫描一次 use。旧实现每发现一个常量定义就扫描整个函数，
    // 在长 SSA 链上会退化为 O(N^2)。
    std::unordered_map<const IRValue*, std::shared_ptr<IRValue>> replacements;
    for (const auto& block : function.blocks) {
      for (const auto& param : block->params) {
        const auto found = states.find(param.get());
        if (found != states.end() &&
            found->second.kind == LatticeKind::Constant)
          replacements.emplace(param.get(), found->second.constant);
      }
      for (const auto& inst : block->insts) {
        const auto found = states.find(inst.get());
        if (found != states.end() &&
            found->second.kind == LatticeKind::Constant)
          replacements.emplace(inst.get(), found->second.constant);
      }
    }
    for (const auto& block : function.blocks) {
      for (const auto& inst : block->insts) {
        for (const auto& use : inst->GetUses()) {
          if (!use) continue;
          const auto replacement = replacements.find(use.get());
          if (replacement != replacements.end())
            changed |= inst->ReplaceUse(use.get(), replacement->second);
        }
      }
    }

    // 常量条件分支立即改成 jump，随后从入口删除不可达块。
    for (const auto& block : function.blocks) {
      if (block->insts.empty()) continue;
      auto branch = dynamic_cast_ptr<IRBranch>(block->insts.back());
      if (!branch || !isa<IRConstant>(branch->GetCond())) continue;
      const bool take_then =
          ConstantIsTrue(*cast<IRConstant>(branch->GetCond()));
      block->insts.back() = std::make_shared<IRJump>(
          take_then ? branch->GetThenTarget() : branch->GetElseTarget(),
          take_then ? branch->GetThenArgs() : branch->GetElseArgs());
      changed = true;
    }

    const size_t old_block_count = function.blocks.size();
    DominatorTree::RemoveUnreachableBlocks(function);
    changed |= old_block_count != function.blocks.size();
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
