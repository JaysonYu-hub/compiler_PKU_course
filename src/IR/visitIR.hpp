#pragma once
#include "custom_ir.hpp"
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <vector>

class LiveVarAnalysis {
public:
  std::unordered_map<std::string, std::unordered_set<std::string>> live_out;
  std::unordered_map<std::string, std::unordered_set<std::string>> live_in;
  
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

        const std::vector<std::string> caller_saved_registers = {
          "ra", "t0", "t1", "t2", "t3", "t4", "t5", "t6",
          "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
          "ft0", "ft1", "ft2", "ft3", "ft4", "ft5", "ft6", "ft7", "ft8", "ft9", "ft10", "ft11",
          "fa0", "fa1", "fa2", "fa3", "fa4", "fa5", "fa6", "fa7"
        };
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