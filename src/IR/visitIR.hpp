#pragma once
#include "custom_ir.hpp"
#include "riscv.hpp"
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <vector>
#include <limits>
#include <array>
#include <algorithm>

class LiveVarAnalysis {
public:
  std::unordered_map<std::string, std::unordered_set<std::string>> live_out;
  std::unordered_map<std::string, std::unordered_set<std::string>> live_in;
  std::unordered_map<std::shared_ptr<IR::IRInstruction>, std::unordered_set<std::string>> live_out_inst;
  std::unordered_map<std::shared_ptr<IR::IRInstruction>, std::unordered_set<std::string>> live_in_inst;
  long long int max_inf = std::numeric_limits<long long int>::max();
  long long int min_inf = std::numeric_limits<long long int>::min();
  const std::vector<std::string> caller_saved_registers = {
    "ra", "t0", "t1", "t2", "t3", "t4", "t5", "t6",
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
    "ft0", "ft1", "ft2", "ft3", "ft4", "ft5", "ft6", "ft7", "ft8", "ft9", "ft10", "ft11",
    "fa0", "fa1", "fa2", "fa3", "fa4", "fa5", "fa6", "fa7"
  };

  void Analyze(IR::IRProgram *program) {
    bool changed = true;
    while (changed) {
      changed = false;
      for (auto func = program->funcs.rbegin(); func != program->funcs.rend(); func++) {
        for (auto block = (*func)->blocks.rbegin(); block != (*func)->blocks.rend(); block++) {
          auto old_live_in = live_in[(*block)->name];
          auto var_kill = std::unordered_set<std::string>();
          auto var_gen = std::unordered_set<std::string>();
          
          hard_code_block(*block, var_gen, var_kill);
          
          live_out[(*block)->name].clear();
          // for S in successors(B): LIVEOUT[B] = LIVEOUT[B].union(LIVEIN[S])
          if ((*block)->next_1) {
            live_out[(*block)->name].insert(live_in[(*block)->next_1->name].begin(), live_in[(*block)->next_1->name].end());
            if ((*block)->next_2) {
              live_out[(*block)->name].insert(live_in[(*block)->next_2->name].begin(), live_in[(*block)->next_2->name].end());
            }
          }
          
          // LIVEOUT[B] - KILL[B]
          for (const auto& var : var_kill) {
            live_out[(*block)->name].erase(var);
          }
          // LIVEOUT[B] U GEN[B]
          live_out[(*block)->name].insert(var_gen.begin(), var_gen.end());
          
          live_in[(*block)->name] = live_out[(*block)->name];
          if (old_live_in != live_in[(*block)->name]) {
            changed = true;
          }
        }
      }
    }

    // 遍历每一个基本块，产出指令级别的活跃表
    for (auto func = program->funcs.begin(); func != program->funcs.end(); ++func) {
      for (auto block = (*func)->blocks.begin(); block != (*func)->blocks.end(); ++block) {
        auto current_live = live_out[(*block)->name];
        
        auto try_add_gen = [&current_live](std::shared_ptr<IR::IRValue> value) { 
          if (!value) return;
          if (IR::isa<IR::IRNameInstruction>(value)) {
            current_live.insert(IR::cast<IR::IRNameInstruction>(value)->GetOperandName());
          } else if (IR::isa<IR::IRCall>(value)) {
            auto call = IR::cast<IR::IRCall>(value);
            if (call->HasReturnValue()) {
              current_live.insert(call->GetOperandName());
            }
          }
        };

        for (auto inst_it = (*block)->insts.rbegin(); inst_it != (*block)->insts.rend(); ++inst_it) {
          auto inst = *inst_it;
          if (!inst) continue;

          live_out_inst[inst] = current_live;

          // 处理当前指令的Def(Kill)
          if (inst->inst_type != IR::IRInstructionType::IR_INST_CALL) {
            if (IR::isa<IR::IRNameInstruction>(inst)) {
              std::string def_var = IR::cast<IR::IRNameInstruction>(inst)->GetOperandName();
              current_live.erase(def_var);
            }
          } else {
            auto call = IR::cast<IR::IRCall>(inst);
            if (call->HasReturnValue()) {
              std::string def_var = call->GetOperandName();
              current_live.erase(def_var);
            }
            for (const auto& reg : caller_saved_registers) {
              current_live.erase(reg);
            }
          }

          // 处理当前指令的Uses(Gen)
          if (inst->inst_type != IR::IRInstructionType::IR_INST_CALL) {
            switch (inst->inst_type) {
              case IR::IRInstructionType::IR_INST_BINARY: {
                auto bin = IR::cast<IR::IRBinary>(inst);
                try_add_gen(bin->GetLhs());
                try_add_gen(bin->GetRhs());
                break;
              }
              case IR::IRInstructionType::IR_INST_LOAD: {
                auto load = IR::cast<IR::IRLoad>(inst);
                try_add_gen(load->GetPtr());
                break;
              }
              case IR::IRInstructionType::IR_INST_STORE: {
                auto store = IR::cast<IR::IRStore>(inst);
                try_add_gen(store->GetVal());
                try_add_gen(store->GetPtr());
                break;
              }
              case IR::IRInstructionType::IR_INST_GETELEMPTR: {
                auto gep = IR::cast<IR::IRGetElemPtr>(inst);
                try_add_gen(gep->GetArr());
                try_add_gen(gep->GetIndex());
                break;
              }
              case IR::IRInstructionType::IR_INST_GETPTR: {
                auto gp = IR::cast<IR::IRGetPtr>(inst);
                try_add_gen(gp->GetPtr());
                try_add_gen(gp->GetIndex());
                break;
              }
              case IR::IRInstructionType::IR_INST_CAST: {
                auto cast_inst = IR::cast<IR::IRCast>(inst);
                try_add_gen(cast_inst->GetValue());
                break;
              }
              case IR::IRInstructionType::IR_INST_BR: {
                auto br = IR::cast<IR::IRBranch>(inst);
                try_add_gen(br->GetCond());
                break;
              }
              case IR::IRInstructionType::IR_INST_RET: {
                auto ret = IR::cast<IR::IRReturn>(inst);
                try_add_gen(ret->GetValue());
                break;
              }
              default:
                break;
            }
          } else {
            auto call = IR::cast<IR::IRCall>(inst);
            for (const auto& arg : call->GetArgs()) {
              try_add_gen(arg);
            }
          }
          live_in_inst[inst] = current_live;
        }
      }
    }

  for (auto func = program->funcs.begin(); func != program->funcs.end(); ++func) {
    std::unordered_map<std::string, std::array<long long int, 2>> intervals;
    auto get_or_init_interval = [&](const std::string& var) -> std::array<long long int, 2>& {
      return intervals.insert({var, {max_inf, min_inf}}).first->second;
    };
    long long int i = 0;
    for (const auto& param : (*func)->params) {
      if (param) {
        get_or_init_interval(param->GetOperandName())[0] = 0;
      }
    }
    for (auto block = (*func)->blocks.begin(); block != (*func)->blocks.end(); ++block) {
      for (auto inst_it = (*block)->insts.begin(); inst_it != (*block)->insts.end(); ++inst_it, ++i) { 
        auto in_point = 2 * i;
        auto out_point = 2 * i + 1;
        for (auto var : live_in_inst[*inst_it]) {
          auto& interval = get_or_init_interval(var);
          interval[0] = std::min(in_point, interval[0]);
          interval[1] = std::max(in_point, interval[1]);
        }
        for (auto var : live_out_inst[*inst_it]) {
          auto& interval = get_or_init_interval(var);
          interval[0] = std::min(out_point, interval[0]);
          interval[1] = std::max(out_point, interval[1]);
        }
      }
    } 
  }
  }

private:
  // 遍历得到Gen和Kill
  void hard_code_block(std::shared_ptr<IR::IRBasicBlock> block, std::unordered_set<std::string> &var_gen, std::unordered_set<std::string> &var_kill) { 
    auto try_add_gen = [&var_gen](std::shared_ptr<IR::IRValue> value) { 
      if (!value) return;
      if (IR::isa<IR::IRNameInstruction>(value)) {
        var_gen.insert(IR::cast<IR::IRNameInstruction>(value)->GetOperandName());
      } else if (IR::isa<IR::IRCall>(value)) {
        auto call = IR::cast<IR::IRCall>(value);
        if (call->HasReturnValue()) {
          var_gen.insert(call->GetOperandName());
        }
      }
    };
    
    for (auto inst_it = block->insts.rbegin(); inst_it != block->insts.rend(); ++inst_it) {
      auto inst = *inst_it;
      if (!inst) continue;

      if (inst->inst_type != IR::IRInstructionType::IR_INST_CALL) {
        // 先用它的 Def 去更新 KILL（因为是倒序，所以后发生的写会覆盖前面的写）
        if (IR::isa<IR::IRNameInstruction>(inst)) {
          std::string def_var = IR::cast<IR::IRNameInstruction>(inst)->GetOperandName();
          var_kill.insert(def_var);
          var_gen.erase(def_var); // 已经在外层被 KILL 的，不再向上暴露
        }
        switch (inst->inst_type) {
          case IR::IRInstructionType::IR_INST_BINARY: {
            auto bin = IR::cast<IR::IRBinary>(inst);
            try_add_gen(bin->GetLhs());
            try_add_gen(bin->GetRhs());
            break;
          }
          case IR::IRInstructionType::IR_INST_LOAD: {
            auto load = IR::cast<IR::IRLoad>(inst);
            try_add_gen(load->GetPtr());
            break;
          }
          case IR::IRInstructionType::IR_INST_STORE: {
            auto store = IR::cast<IR::IRStore>(inst);
            try_add_gen(store->GetVal());
            try_add_gen(store->GetPtr());
            break;
          }
          case IR::IRInstructionType::IR_INST_GETELEMPTR: {
            auto gep = IR::cast<IR::IRGetElemPtr>(inst);
            try_add_gen(gep->GetArr());
            try_add_gen(gep->GetIndex());
            break;
          }
          case IR::IRInstructionType::IR_INST_GETPTR: {
            auto gp = IR::cast<IR::IRGetPtr>(inst);
            try_add_gen(gp->GetPtr());
            try_add_gen(gp->GetIndex());
            break;
          }
          case IR::IRInstructionType::IR_INST_CAST: {
            auto cast_inst = IR::cast<IR::IRCast>(inst);
            try_add_gen(cast_inst->GetValue());
            break;
          }
          case IR::IRInstructionType::IR_INST_BR: {
            auto br = IR::cast<IR::IRBranch>(inst);
            try_add_gen(br->GetCond());
            break;
          }
          case IR::IRInstructionType::IR_INST_RET: {
            auto ret = IR::cast<IR::IRReturn>(inst);
            try_add_gen(ret->GetValue());
            break;
          }
          default:
            break;
        }
      }
      else {
        auto call = IR::cast<IR::IRCall>(inst);
        
        if (call->HasReturnValue()) {
          std::string def_var = call->GetOperandName();
          var_kill.insert(def_var);
          var_gen.erase(def_var);
        }

        for (const auto& reg : caller_saved_registers) {
          var_kill.insert(reg);
          var_gen.erase(reg);
        }

        for (const auto& arg : call->GetArgs()) {
          try_add_gen(arg);
        }
      }
    }
  }
};

class LinearScanAllocator {
public:
  struct LiveInterval {
    std::string var;
    long long int start;
    long long int end;
    std::string reg;
    bool spilled = false;
    int stack_offset = 0;
  };

  // Available physical registers for allocation (excluding sp, gp, tp, zero)
  // Ordered by preference: temporaries first, then callee-saved registers (s-series), then argument registers
  const std::vector<std::string> reg_pool = {
    "t0", "t1", "t2", "t3", "t4", "t5", "t6",
    "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"
  };

  LinearScanAllocator(const std::unordered_map<std::string, std::array<long long int, 2>>& intervals) {
    // 1. Filter out physical registers, keep only virtual registers/variables
    for (const auto& [var, range] : intervals) {
      if (is_virtual_register(var)) {
        intervals_list.push_back({var, range[0], range[1], "", false, 0});
      }
    }

    // 2. Sort intervals by start point in ascending order (linear scan requirement)
    std::sort(intervals_list.begin(), intervals_list.end(), [](const LiveInterval& a, const LiveInterval& b) {
      return a.start < b.start;
    });
  }

  void Allocate() {
    int offset = 0;
    std::vector<std::string> free_regs = reg_pool;
    std::vector<LiveInterval*> active_regs = {};
    for (auto& current : intervals_list) {
      auto it = active_regs.begin();
      while (it != active_regs.end()) {
        if ((*it)->end <= current.start) {
          free_regs.push_back((*it)->reg);
          it = active_regs.erase(it);
        } else {
          break;
        }
      }

      if (!free_regs.empty()) {
        current.reg = free_regs.back();
        free_regs.pop_back();
        active_regs.push_back(&current);
        std::sort(active_regs.begin(), active_regs.end(), [](const LiveInterval* a, const LiveInterval* b) {
          return a->end < b->end;
        });
      } else {
        if (!active_regs.empty() && active_regs.back()->end > current.end) {
          LiveInterval* spill_candidate = active_regs.back();
          spill_candidate->spilled = true;
          spill_candidate->stack_offset = offset;
          offset += RegSize();

          active_regs.pop_back();
          active_regs.push_back(&current);
          current.reg = spill_candidate->reg;
        } else { 
          current.spilled = true;
          current.stack_offset = offset;
          offset += RegSize();
        }
      }
    }
  }

  // Get physical register mapping for successfully allocated variables
  std::unordered_map<std::string, std::string> GetRegMap() const {
    std::unordered_map<std::string, std::string> reg_map;
    for (const auto& interval : intervals_list) {
      if (!interval.spilled && !interval.reg.empty()) {
        reg_map[interval.var] = interval.reg;
      }
    }
    return reg_map;
  }

  // Get list of variables that need to be spilled to stack
  std::vector<LiveInterval> GetSpilledIntervals() const {
    std::vector<LiveInterval> spilled;
    for (const auto& interval : intervals_list) {
      if (interval.spilled) {
        spilled.push_back(interval);
      }
    }
    return spilled;
  }

private:
  std::vector<LiveInterval> intervals_list;

  // Checks if a variable name represents a virtual register/variable to be allocated
  bool is_virtual_register(const std::string& var) const {
    if (var.empty()) return false;
    if (var[0] == '%') return true; // Koopa IR variables start with '%'
    
    static const std::unordered_set<std::string> physical_regs = {
      "zero", "ra", "sp", "gp", "tp", 
      "t0", "t1", "t2", "t3", "t4", "t5", "t6",
      "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",
      "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
      "ft0", "ft1", "ft2", "ft3", "ft4", "ft5", "ft6", "ft7", "ft8", "ft9", "ft10", "ft11",
      "fa0", "fa1", "fa2", "fa3", "fa4", "fa5", "fa6", "fa7"
    };
    return physical_regs.find(var) == physical_regs.end();
  }
};