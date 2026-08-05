#pragma once

#include "../custom_ir.hpp"
#include "tool.hpp"
#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace IR {
namespace Pass {

// Global Value Numbering / Common Subexpression Elimination。
//
// 只对无内存副作用的表达式编号：binary、cast、getptr 和
// getelemptr。load/call 不进入值编号，避免在尚无完整 MemorySSA 和
// 别名分析时错误地跨 store 复用旧值。
class GVN {
private:
  using ValueTable =
      std::unordered_map<std::string, std::shared_ptr<IRValue>>;

  static bool IsCommutative(IRBinaryOp op) {
    switch (op) {
      case IRBinaryOp::ADD:
      case IRBinaryOp::MUL:
      case IRBinaryOp::AND:
      case IRBinaryOp::OR:
      case IRBinaryOp::XOR:
      case IRBinaryOp::EQ:
      case IRBinaryOp::NE:
        return true;
      default:
        return false;
    }
  }

  static std::string ValueToken(const std::shared_ptr<IRValue>& value) {
    if (!value) return "null";
    if (isa<IRConstant>(value)) {
      return "c:" + std::to_string(static_cast<int>(value->GetType())) +
             ":" + value->DumpText();
    }
    std::ostringstream stream;
    stream << "v:" << reinterpret_cast<std::uintptr_t>(value.get());
    return stream.str();
  }

  static std::string ExpressionKey(
      const std::shared_ptr<IRInstruction>& inst) {
    if (isa<IRBinary>(inst)) {
      const auto binary = cast<IRBinary>(inst);
      std::string lhs = ValueToken(binary->GetLhs());
      std::string rhs = ValueToken(binary->GetRhs());
      if (IsCommutative(binary->GetOp()) && rhs < lhs) std::swap(lhs, rhs);
      return "binary:" + std::to_string(static_cast<int>(binary->GetOp())) +
             ":" +
             std::to_string(static_cast<int>(binary->GetOperandType())) +
             ":" + std::to_string(static_cast<int>(binary->GetType())) +
             ":" + lhs + ":" + rhs;
    }
    if (isa<IRCast>(inst)) {
      const auto cast_inst = cast<IRCast>(inst);
      return "cast:" +
             std::to_string(static_cast<int>(cast_inst->GetOp())) + ":" +
             std::to_string(static_cast<int>(cast_inst->GetType())) + ":" +
             ValueToken(cast_inst->GetValue());
    }
    if (isa<IRGetElemPtr>(inst)) {
      const auto gep = cast<IRGetElemPtr>(inst);
      return "gep:" + std::to_string(gep->GetElemSize()) + ":" +
             ValueToken(gep->GetArr()) + ":" + ValueToken(gep->GetIndex());
    }
    if (isa<IRGetPtr>(inst)) {
      const auto getptr = cast<IRGetPtr>(inst);
      return "getptr:" + std::to_string(getptr->GetElemSize()) + ":" +
             ValueToken(getptr->GetPtr()) + ":" +
             ValueToken(getptr->GetIndex());
    }
    return {};
  }

  static bool VisitDominatorSubtree(
      IRBasicBlock* block, const DominatorTree& dom_tree, ValueTable& available,
      std::unordered_map<const IRValue*, std::shared_ptr<IRValue>>&
          replacements) {
    bool changed = false;
    std::vector<std::string> locally_inserted;
    std::vector<std::shared_ptr<IRInstruction>> new_insts;
    new_insts.reserve(block->insts.size());

    auto resolve = [&](std::shared_ptr<IRValue> value) {
      while (value && replacements.count(value.get()) != 0)
        value = replacements.at(value.get());
      return value;
    };

    for (const auto& inst : block->insts) {
      // 只规范化当前指令的操作数。这样表达式键始终使用代表值，同时避免
      // 每发现一个公共子表达式就 ReplaceAllUses 扫描整个函数。
      for (const auto& use : inst->GetUses()) {
        if (!use) continue;
        auto canonical = resolve(use);
        if (canonical.get() != use.get())
          inst->ReplaceUse(use.get(), std::move(canonical));
      }
      const std::string key = ExpressionKey(inst);
      if (key.empty()) {
        new_insts.push_back(inst);
        continue;
      }

      const auto found = available.find(key);
      if (found != available.end()) {
        // 支配当前块的等价表达式可以安全替代当前 SSA 定义。
        replacements[inst.get()] = resolve(found->second);
        changed = true;
        continue;
      }

      available.emplace(key, inst);
      locally_inserted.push_back(key);
      new_insts.push_back(inst);
    }
    block->insts = std::move(new_insts);

    const auto child_it = dom_tree.dom_children.find(block);
    if (child_it != dom_tree.dom_children.end()) {
      for (IRBasicBlock* child : child_it->second) {
        changed |= VisitDominatorSubtree(child, dom_tree, available,
                                         replacements);
      }
    }

    // 离开支配树子树时恢复作用域，防止兄弟分支错误共享定义。
    for (auto it = locally_inserted.rbegin();
         it != locally_inserted.rend(); ++it) {
      available.erase(*it);
    }
    return changed;
  }

public:
  bool OptimizeFunction(IRFunction& function) {
    if (function.blocks.empty()) return false;
    DominatorTree dom_tree;
    dom_tree.Analyze(function);
    if (function.blocks.empty()) return false;

    ValueTable available;
    std::unordered_map<const IRValue*, std::shared_ptr<IRValue>> replacements;
    return VisitDominatorSubtree(function.blocks.front().get(), dom_tree,
                                 available, replacements);
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
