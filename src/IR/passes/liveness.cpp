#include "liveness.hpp"

namespace IR {
namespace Pass {

void LiveVarAnalysis::Analyze(IR::IRProgram *program) {
  bool changed = true;
  while (changed) {
    changed = false;
    for (auto func = program->funcs.rbegin(); func != program->funcs.rend(); func++) {
      std::string func_name = (*func)->name;
      for (auto block = (*func)->blocks.rbegin(); block != (*func)->blocks.rend(); block++) {
        std::string unique_block_name = func_name + ":" + (*block)->name;
        auto old_live_in = live_in[unique_block_name];
        auto var_kill = std::unordered_set<std::string>();
        auto var_gen = std::unordered_set<std::string>();
        
        hard_code_block(*block, var_gen, var_kill);
        
        live_out[unique_block_name].clear();
        // Derive CFG edges from the terminator.  Keeping a second, manually
        // maintained next_1/next_2 graph can silently diverge from jump/br.
        for (const auto* successor : (*block)->GetSuccessors()) {
          std::string successor_name = func_name + ":" + successor->name;
          live_out[unique_block_name].insert(live_in[successor_name].begin(),
                                             live_in[successor_name].end());
        }
        
        // LIVEOUT[B] - KILL[B]
        auto current_live_in = live_out[unique_block_name];
        for (const auto& var : var_kill) {
          current_live_in.erase(var);
        }
        // LIVEOUT[B] U GEN[B]
        current_live_in.insert(var_gen.begin(), var_gen.end());
        
        live_in[unique_block_name] = current_live_in;
        if (old_live_in != live_in[unique_block_name]) {
          changed = true;
        }
      }
    }
  }

  // 遍历每一个基本块，产出指令级别的活跃表
  for (auto func = program->funcs.begin(); func != program->funcs.end(); ++func) {
    std::string func_name = (*func)->name;
    for (auto block = (*func)->blocks.begin(); block != (*func)->blocks.end(); ++block) {
      std::string unique_block_name = func_name + ":" + (*block)->name;
      auto current_live = live_out[unique_block_name];
      
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
    func_intervals[(*func)->name] = intervals;
  }
}

void LiveVarAnalysis::hard_code_block(std::shared_ptr<IR::IRBasicBlock> block, std::unordered_set<std::string> &var_gen, std::unordered_set<std::string> &var_kill) { 
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

}
}
