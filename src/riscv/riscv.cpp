#include "riscv.hpp"
#include "../IR/visitIR.hpp"
#include <algorithm>
#include <iostream>
#include <cassert>
#include <cstring>

bool g_is_64bit = true;

namespace {

bool GetPowerOfTwoShift(uint64_t value, unsigned& shift) {
  if (value == 0 || (value & (value - 1)) != 0) {
    return false;
  }

  shift = 0;
  while (value > 1) {
    value >>= 1;
    ++shift;
  }
  return true;
}

}

void Back::RiscvGenerator::Generate(const IR::IRProgram& program) {
  // Keep O0 as a conservative stack-based correctness baseline. The global
  // linear-scan allocator is enabled only for O1.
  if (optimization_level == OptimizationLevel::O1) {
    LiveVarAnalysis lva;
    lva.Analyze(const_cast<IR::IRProgram*>(&program));
    this->lva_ptr = &lva;
    Visit(program);
    this->lva_ptr = nullptr;
  } else {
    this->lva_ptr = nullptr;
    Visit(program);
  }
}

std::string Back::RiscvGenerator::StripAt(const std::string& name) {
  if (!name.empty() && name[0] == '@') return name.substr(1);
  return name;
}

std::string Back::RiscvGenerator::StripPercent(const std::string& name) {
  if (!name.empty() && name[0] == '%') return name.substr(1);
  return name;
}

void Back::RiscvGenerator::Visit(const IR::IRProgram& program) {
  for (const auto& global : program.globals) {
    Visit(*global);
  }
  std::cout << "  .text\n";
  for (const auto& func : program.funcs) {
    Visit(*func);
  }
}

void Back::RiscvGenerator::Visit(const IR::IRGlobalAlloc& global) {
  std::string name = StripAt(global.GetOperandName());

  if (global.IsZeroInit() ||
      (global.GetInitVals().empty() && global.GetInitFloatVals().empty())) {
    std::cout << "  .data\n";
    std::cout << "  .globl " << name << "\n";
    std::cout << name << ":\n";
    std::cout << "  .zero " << global.GetAllocSize() << "\n";
  } else {
    std::cout << "  .data\n";
    std::cout << "  .globl " << name << "\n";
    std::cout << name << ":\n";
    if (!global.GetInitFloatVals().empty()) {
      for (float value : global.GetInitFloatVals()) {
        int32_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        std::cout << "  .word " << bits << "\n";
      }
    } else {
      for (int32_t value : global.GetInitVals()) {
        std::cout << "  .word " << value << "\n";
      }
    }
  }
  std::cout << "\n";
}

void Back::RiscvGenerator::Visit(const IR::IRFunction& func) {
  val_stack_map.clear();
  val_type_map.clear();
  local_allocs.clear();
  reg_map.clear();
  spill_offsets.clear();
  used_s_regs.clear();
  has_call = false;
  max_stack_arg_count = 0;
  max_call_arg_count = 0;
  argument_buffer_size = 0;

  // 收集属于虚拟寄存器或参数的变量名称，排除局部变量 alloc 
  std::unordered_set<std::string> virtual_regs;
  for (const auto& block : func.blocks) {
    for (const auto& inst : block->insts) {
      bool has_retval = false;
      std::string val_name;
      if (const auto* name_inst = dynamic_cast<const IR::IRNameInstruction*>(inst.get())) {
        if (const auto* alloc = dynamic_cast<const IR::IRAlloc*>(inst.get())) {
          if (!alloc->IsArray()) {
            has_retval = true;
            val_name = name_inst->GetName();
          }
        } else {
          has_retval = true;
          val_name = name_inst->GetName();
        }
      } else if (const auto* call = dynamic_cast<const IR::IRCall*>(inst.get())) {
        if (call->HasReturnValue()) {
          has_retval = true;
          val_name = call->GetOperandName();
        }
      }
      if (has_retval) {
        virtual_regs.insert(val_name);
      }
    }
  }

  // 调用寄存器分配器
  if (lva_ptr) {
    auto it = lva_ptr->func_intervals.find(func.name);
    if (it != lva_ptr->func_intervals.end()) {
      // 仅对真正的临时 SSA 虚拟寄存器和参数进行寄存器分配
      std::unordered_map<std::string, std::array<long long int, 2>> filtered_intervals;
      for (const auto& [var, range] : it->second) {
        if (virtual_regs.find(var) != virtual_regs.end()) {
          filtered_intervals[var] = range;
        }
      }
      LinearScanAllocator allocator(filtered_intervals);
      allocator.Allocate();
      reg_map = allocator.GetRegMap();
      auto spilled = allocator.GetSpilledIntervals();
      for (const auto& s : spilled) {
        spill_offsets[s.var] = s.stack_offset;
      }
    }
  }

  // 溢出变量空间计算
  int spill_size = 0;
  for (const auto& kv : spill_offsets) {
    if (kv.second + GetRegisterSize() > spill_size) {
      spill_size = kv.second + GetRegisterSize();
    }
  }

  // 收集用到的 callee-saved 寄存器
  for (const auto& kv : reg_map) {
    if (!kv.second.empty() && kv.second[0] == 's') {
      if (std::find(used_s_regs.begin(), used_s_regs.end(), kv.second) == used_s_regs.end()) {
        used_s_regs.push_back(kv.second);
      }
    }
  }

  int offset = spill_size;
  // 为被保护的 s 寄存器分配栈偏移量
  offset = AlignTo(offset, GetRegisterSize());
  s_reg_start_offset = offset;
  offset += used_s_regs.size() * GetRegisterSize();

  // 1. 为局部 alloc 数组/变量分配空间
  for (const auto& block : func.blocks) {
    for (const auto& inst : block->insts) {
      bool has_retval = false;
      std::string val_name;
      if (const auto* name_inst = dynamic_cast<const IR::IRNameInstruction*>(inst.get())) {
        has_retval = true;
        val_name = name_inst->GetName();
      } else if (const auto* call = dynamic_cast<const IR::IRCall*>(inst.get())) {
        if (call->HasReturnValue()) {
          has_retval = true;
          val_name = call->GetOperandName();
        }
      }

      if (has_retval) {
        if (const auto* alloc = dynamic_cast<const IR::IRAlloc*>(inst.get())) {
          local_allocs.insert(val_name);
          const int alloc_alignment = alloc->IsArray()
              ? 4
              : GetStackAlignment(alloc->GetAllocatedType());
          offset = AlignTo(offset, alloc_alignment);
          val_stack_map[val_name] = offset;
          val_type_map[val_name] = alloc->GetAllocatedType();
          int alloc_size = 4;
          if (alloc->IsArray()) {
            alloc_size = 4;
            for (int d : alloc->GetArrayDims()) {
              alloc_size *= d;
            }
          } else {
            alloc_size = GetTypeSize(alloc->GetAllocatedType());
          }
          offset += alloc_size;
        }
      }
    }
  }

  // 2. 为函数参数分配空间（使用溢出槽或降级为 fallback 空间）
  for (size_t i = 0; i < func.params.size(); i++) {
    std::string param_name = func.params[i]->GetOperandName();
    val_type_map[param_name] = func.params[i]->GetType();
    if (spill_offsets.count(param_name)) {
      val_stack_map[param_name] = spill_offsets[param_name];
    } else {
      offset = AlignTo(offset, GetStackAlignment(func.params[i]->GetType()));
      val_stack_map[param_name] = offset;
      offset += GetTypeSize(func.params[i]->GetType());
    }
  }

  // 3. 收集所有变量的类型，并将溢出映射写入 val_stack_map
  for (const auto& block : func.blocks) {
    for (const auto& inst : block->insts) {
      bool has_retval = false;
      std::string val_name;
      if (const auto* name_inst = dynamic_cast<const IR::IRNameInstruction*>(inst.get())) {
        has_retval = true;
        val_name = name_inst->GetName();
      } else if (const auto* call = dynamic_cast<const IR::IRCall*>(inst.get())) {
        if (call->HasReturnValue()) {
          has_retval = true;
          val_name = call->GetOperandName();
        }
      }
      if (has_retval) {
        if (const auto* alloc = dynamic_cast<const IR::IRAlloc*>(inst.get())) {
          val_type_map[val_name] = alloc->GetAllocatedType();
        } else {
          val_type_map[val_name] = inst->GetType();
        }
      }
    }
  }

  for (const auto& kv : spill_offsets) {
    if (val_stack_map.find(kv.first) == val_stack_map.end()) {
      val_stack_map[kv.first] = kv.second;
    }
  }

  // 4. 为其他指令分配 fallback 空间，以防后端崩溃
  for (const auto& block : func.blocks) {
    for (const auto& inst : block->insts) {
      if (const auto* call = dynamic_cast<const IR::IRCall*>(inst.get())) {
        has_call = true;
        max_call_arg_count = std::max(
            max_call_arg_count, static_cast<int>(call->GetArgs().size()));
        std::vector<IR::IRType> arg_types;
        for (const auto& arg : call->GetArgs()) {
          arg_types.push_back(arg->GetType());
        }
        int stack_args = 0;
        for (const auto& location : AssignArgumentLocations(arg_types)) {
          if (location.kind == ArgumentLocation::Kind::STACK) ++stack_args;
        }
        if (stack_args > max_stack_arg_count) {
          max_stack_arg_count = stack_args;
        }
      }

      bool has_retval = false;
      std::string val_name;
      if (const auto* name_inst = dynamic_cast<const IR::IRNameInstruction*>(inst.get())) {
        has_retval = true;
        val_name = name_inst->GetName();
      } else if (const auto* call = dynamic_cast<const IR::IRCall*>(inst.get())) {
        if (call->HasReturnValue()) {
          has_retval = true;
          val_name = call->GetOperandName();
        }
      }

      if (has_retval && val_stack_map.find(val_name) == val_stack_map.end()) {
        IR::IRType value_type = inst->GetType();
        if (const auto* alloc = dynamic_cast<const IR::IRAlloc*>(inst.get())) {
          value_type = alloc->GetAllocatedType();
        }
        offset = AlignTo(offset, GetStackAlignment(value_type));
        val_stack_map[val_name] = offset;
        if (const auto* alloc = dynamic_cast<const IR::IRAlloc*>(inst.get())) {
          val_type_map[val_name] = alloc->GetAllocatedType();
          offset += GetTypeSize(alloc->GetAllocatedType());
        } else {
          val_type_map[val_name] = inst->GetType();
          offset += GetTypeSize(inst->GetType());
        }
      }
    }
  }

  int slot_size = GetRegisterSize();
  param_overflow_size = slot_size * max_stack_arg_count;

  // 如果有函数调用，在栈帧最底部（参数溢出区之后）额外分配 128 字节参数暂存区
  // Snapshot register-bound arguments before assigning a0-a7/fa0-fa7. The
  // old fixed 128-byte area overflowed for large mixed-argument calls.
  argument_buffer_size = has_call
      ? AlignTo(max_call_arg_count * slot_size, 16)
      : 0;

  // ra 保存空间
  int ra_size = has_call ? GetRegisterSize() : 0;

  // 计算总栈帧大小
  int total = param_overflow_size + argument_buffer_size + offset + ra_size;
  current_stack_size = AlignTo(total, 16);

  // 调整所有 val_stack_map 偏移量，加上参数溢出区和参数暂存区的偏移
  for (auto& kv : val_stack_map) {
    kv.second += param_overflow_size + argument_buffer_size;
  }
  // 调整 s_reg_start_offset
  s_reg_start_offset += param_overflow_size + argument_buffer_size;

  std::cout << "  .globl " << func.name << "\n";
  std::cout << func.name << ":\n";

  // Prologue：分配栈帧
  if (current_stack_size > 0) {
    if (current_stack_size <= 2048) {
      std::cout << "  addi sp, sp, -" << current_stack_size << "\n";
    } else {
      std::cout << "  li t5, -" << current_stack_size << "\n";
      std::cout << "  add sp, sp, t5\n";
    }
  }

  // 保存 ra
  if (has_call) {
    int ra_offset = current_stack_size - GetRegisterSize();
    if (GetRegisterSize() == 8) {
      EmitSd("ra", ra_offset, "sp");
    } else {
      EmitSw("ra", ra_offset, "sp");
    }
  }

  // 保存被使用的 s 寄存器 (callee-saved)
  for (size_t i = 0; i < used_s_regs.size(); ++i) {
    int s_offset = s_reg_start_offset + i * GetRegisterSize();
    if (GetRegisterSize() == 8) {
      EmitSd(used_s_regs[i], s_offset, "sp");
    } else {
      EmitSw(used_s_regs[i], s_offset, "sp");
    }
  }

  // 将寄存器参数保存到栈上，且初始化其在 reg_map 中的物理寄存器
  std::vector<IR::IRType> param_types;
  for (const auto& param : func.params) param_types.push_back(param->GetType());
  const auto param_locations = AssignArgumentLocations(param_types);
  for (size_t i = 0; i < func.params.size(); ++i) {
    std::string param_name = func.params[i]->GetOperandName();
    auto it = val_stack_map.find(param_name);
    assert(it != val_stack_map.end());
    const auto& location = param_locations[i];
    
    // 检查参数是否被分配了物理寄存器
    auto reg_it = reg_map.find(param_name);
    std::string p_reg = (reg_it != reg_map.end()) ? reg_it->second : "";
    
    if (location.kind == ArgumentLocation::Kind::FLOAT_REGISTER) {
      std::string src_reg = "fa" + std::to_string(location.index);
      if (!p_reg.empty()) {
        if (p_reg != src_reg) {
          if (p_reg[0] == 'f') {
            std::cout << "  fmv.s " << p_reg << ", " << src_reg << "\n";
          } else {
            std::cout << "  fmv.x.w " << p_reg << ", " << src_reg << "\n";
          }
        }
      }
      EmitFsw(src_reg, it->second, "sp");
    } else if (location.kind == ArgumentLocation::Kind::INTEGER_REGISTER) {
      std::string src_reg = "a" + std::to_string(location.index);
      if (!p_reg.empty()) {
        if (p_reg != src_reg) {
          if (p_reg[0] == 'f') {
            std::cout << "  fmv.w.x " << p_reg << ", " << src_reg << "\n";
          } else {
            std::cout << "  mv " << p_reg << ", " << src_reg << "\n";
          }
        }
      }
      if (GetTypeSize(func.params[i]->GetType()) == 8) {
        EmitSd(src_reg, it->second, "sp");
      } else {
        EmitSw(src_reg, it->second, "sp");
      }
    } else {
      const int caller_offset = current_stack_size + slot_size * location.index;
      std::string target_reg = !p_reg.empty() ? p_reg : "t5";
      if (func.params[i]->GetType() == IR::IRType::IR_FLOAT) {
        if (target_reg[0] == 'f') {
          EmitFlw(target_reg, caller_offset, "sp");
        } else {
          EmitFlw("ft5", caller_offset, "sp");
          std::cout << "  fmv.x.w " << target_reg << ", ft5\n";
        }
        EmitFsw(target_reg[0] == 'f' ? target_reg : "ft5", it->second, "sp");
      } else if (GetTypeSize(func.params[i]->GetType()) == 8) {
        if (target_reg[0] != 'f') {
          EmitLd(target_reg, caller_offset, "sp");
        } else {
          EmitLd("t5", caller_offset, "sp");
          std::cout << "  fmv.w.x " << target_reg << ", t5\n";
        }
        EmitSd(target_reg[0] != 'f' ? target_reg : "t5", it->second, "sp");
      } else {
        if (target_reg[0] != 'f') {
          EmitLw(target_reg, caller_offset, "sp");
        } else {
          EmitLw("t5", caller_offset, "sp");
          std::cout << "  fmv.w.x " << target_reg << ", t5\n";
        }
        EmitSw(target_reg[0] != 'f' ? target_reg : "t5", it->second, "sp");
      }
    }
  }
  // 多于8个的参数映射到调用者栈帧中的位置
  // 遍历所有基本块
  for (const auto& block : func.blocks) {
    Visit(*block);
  }
}

void Back::RiscvGenerator::Visit(const IR::IRBasicBlock& block) {
  // 输出label
  std::string label = StripPercent(block.name);
  if (label != "entry") {
    std::cout << label << ":\n";
  }
  for (const auto& inst : block.insts) {
    bool is_call = (inst->inst_type == IR::IRInstructionType::IR_INST_CALL);
    
    std::vector<std::pair<std::string, std::string>> regs_to_save; // (var_name, reg_name)
    if (is_call && lva_ptr) {
      auto live_it = lva_ptr->live_out_inst.find(inst);
      if (live_it != lva_ptr->live_out_inst.end()) {
        std::string def_name;
        auto call_ptr = std::dynamic_pointer_cast<IR::IRCall>(inst);
        if (call_ptr && call_ptr->HasReturnValue()) {
          def_name = call_ptr->GetOperandName();
        }
        for (const auto& var : live_it->second) {
          if (var == def_name) continue;
          auto reg_it = reg_map.find(var);
          if (reg_it != reg_map.end() && !reg_it->second.empty()) {
            if (reg_it->second[0] != 's') {
              regs_to_save.push_back({var, reg_it->second});
            }
          }
        }
      }
    }

    // Save caller-saved registers
    for (const auto& kv : regs_to_save) {
      auto type_it = val_type_map.find(kv.first);
      IR::IRType type = (type_it != val_type_map.end()) ? type_it->second : IR::IRType::IR_INT32;
      StoreRegToValByName(kv.first, kv.second, type);
    }

    Visit(*inst);

    // Restore caller-saved registers
    for (const auto& kv : regs_to_save) {
      auto type_it = val_type_map.find(kv.first);
      IR::IRType type = (type_it != val_type_map.end()) ? type_it->second : IR::IRType::IR_INT32;
      LoadValueToRegByName(kv.first, kv.second, type);
    }
  }
}

void Back::RiscvGenerator::Visit(const IR::IRInstruction& inst) {
  if (const auto* p = dynamic_cast<const IR::IRReturn*>(&inst)) {
    Visit(*p);
  } else if (const auto* p = dynamic_cast<const IR::IRBinary*>(&inst)) {
    Visit(*p);
  } else if (const auto* p = dynamic_cast<const IR::IRCast*>(&inst)) {
    Visit(*p);
  } else if (const auto* p = dynamic_cast<const IR::IRAlloc*>(&inst)) {
    Visit(*p);
  } else if (const auto* p = dynamic_cast<const IR::IRStore*>(&inst)) {
    Visit(*p);
  } else if (const auto* p = dynamic_cast<const IR::IRZeroInit*>(&inst)) {
    Visit(*p);
  } else if (const auto* p = dynamic_cast<const IR::IRLoad*>(&inst)) {
    Visit(*p);
  } else if (const auto* p = dynamic_cast<const IR::IRJump*>(&inst)) {
    Visit(*p);
  } else if (const auto* p = dynamic_cast<const IR::IRBranch*>(&inst)) {
    Visit(*p);
  } else if (const auto* p = dynamic_cast<const IR::IRCall*>(&inst)) {
    Visit(*p);
  } else if (const auto* p = dynamic_cast<const IR::IRGetElemPtr*>(&inst)) {
    Visit(*p);
  } else if (const auto* p = dynamic_cast<const IR::IRGetPtr*>(&inst)) {
    Visit(*p);
  }
}

void Back::RiscvGenerator::Visit(const IR::IRReturn& ret_inst) {
  if (ret_inst.GetValue()) {
    if (ret_inst.GetValue()->GetType() == IR::IRType::IR_FLOAT) {
      LoadValueToFReg(ret_inst.GetValue().get(), "fa0");
    } else {
      LoadValueToReg(ret_inst.GetValue().get(), "a0");
    }
  }
  // Epilogue
  // 恢复被使用的 s 寄存器 (callee-saved)
  for (size_t i = 0; i < used_s_regs.size(); ++i) {
    int s_offset = s_reg_start_offset + i * GetRegisterSize();
    if (GetRegisterSize() == 8) {
      EmitLd(used_s_regs[i], s_offset, "sp");
    } else {
      EmitLw(used_s_regs[i], s_offset, "sp");
    }
  }

  if (has_call) {
    int ra_offset = current_stack_size - GetRegisterSize();
    if (GetRegisterSize() == 8) {
      EmitLd("ra", ra_offset, "sp");
    } else {
      EmitLw("ra", ra_offset, "sp");
    }
  }
  if (current_stack_size > 0) {
    if (current_stack_size <= 2047) {
      std::cout << "  addi sp, sp, " << current_stack_size << "\n";
    } else {
      std::cout << "  li t5, " << current_stack_size << "\n";
      std::cout << "  add sp, sp, t5\n";
    }
  }
  std::cout << "  ret\n";
}

bool Back::RiscvGenerator::TryEmitMulStrengthReduction(const IR::IRBinary& inst) {
  if (optimization_level != OptimizationLevel::O1 ||
      inst.GetOp() != IR::IRBinaryOp::MUL ||
      inst.GetOperandType() != IR::IRType::IR_INT32) {
    return false;
  }

  auto emit_for_constant = [this, &inst](const IR::IRConstant* constant,
                                          const IR::IRValue* other) {
    if (constant == nullptr) {
      return false;
    }

    if (constant->GetType() != IR::IRType::IR_INT32) {
      return false;
    }

    const int64_t signed_value = std::get<int32_t>(constant->GetValue());
    if (signed_value == 0) {
      std::cout << "  mv t5, x0\n";
      StoreRegToVal(&inst, "t5");
      return true;
    }

    const bool negate = signed_value < 0;
    const uint64_t magnitude = negate
      ? static_cast<uint64_t>(-signed_value)
      : static_cast<uint64_t>(signed_value);
    unsigned shift = 0;
    if (!GetPowerOfTwoShift(magnitude, shift)) {
      return false;
    }

    LoadValueToReg(other, "t5");
    if (shift != 0) {
      // SysY int is 32-bit, so RV64 must preserve mulw semantics with slliw.
      const char* shift_op = is_64bit ? "slliw" : "slli";
      std::cout << "  " << shift_op << " t5, t5, " << shift << "\n";
    }
    if (negate) {
      const char* negate_op = is_64bit ? "subw" : "sub";
      std::cout << "  " << negate_op << " t5, x0, t5\n";
    }

    StoreRegToVal(&inst, "t5");
    return true;
  };

  const auto* rhs_constant = dynamic_cast<const IR::IRConstant*>(inst.GetRhs().get());
  if (emit_for_constant(rhs_constant, inst.GetLhs().get())) {
    return true;
  }

  const auto* lhs_constant = dynamic_cast<const IR::IRConstant*>(inst.GetLhs().get());
  return emit_for_constant(lhs_constant, inst.GetRhs().get());
}

void Back::RiscvGenerator::Visit(const IR::IRBinary& inst) {
  if (inst.GetOperandType() == IR::IRType::IR_FLOAT) {
    // Check if destination is allocated a float register
    std::string dest_reg = "ft5";
    std::string name = inst.GetOperandName();
    auto reg_it = reg_map.find(name);
    bool spilled = true;
    if (reg_it != reg_map.end() && !reg_it->second.empty() && reg_it->second[0] == 'f') {
      dest_reg = reg_it->second;
      spilled = false;
    }
    
    // Determine RHS
    std::string rhs_reg = "ft6";
    if (auto rhs_const = dynamic_cast<const IR::IRConstant*>(inst.GetRhs().get())) {
        LoadValueToFReg(rhs_const, "ft6");
    } else if (inst.GetRhs()->GetType() == IR::IRType::IR_INT32) {
        LoadValueToReg(inst.GetRhs().get(), "t5");
        std::cout << "  fcvt.s.w ft6, t5\n";
    } else {
        LoadValueToFReg(inst.GetRhs().get(), "ft6");
    }

    LoadValueToFReg(inst.GetLhs().get(), dest_reg);

    // Comparison operators result in integer register
    std::string int_dest_reg = spilled ? "t5" : "";
    auto int_reg_it = reg_map.find(name);
    if (int_reg_it != reg_map.end() && !int_reg_it->second.empty()) {
        int_dest_reg = int_reg_it->second;
    }
    
    switch (inst.GetOp()) {
        case IR::IRBinaryOp::ADD:
            std::cout << "  fadd.s " << dest_reg << ", " << dest_reg << ", " << rhs_reg << "\n";
            break;
        case IR::IRBinaryOp::SUB:
            std::cout << "  fsub.s " << dest_reg << ", " << dest_reg << ", " << rhs_reg << "\n";
            break;
        case IR::IRBinaryOp::MUL:
            std::cout << "  fmul.s " << dest_reg << ", " << dest_reg << ", " << rhs_reg << "\n";
            break;
        case IR::IRBinaryOp::DIV:
            std::cout << "  fdiv.s " << dest_reg << ", " << dest_reg << ", " << rhs_reg << "\n";
            break;
        case IR::IRBinaryOp::EQ:
            std::cout << "  feq.s " << int_dest_reg << ", " << dest_reg << ", " << rhs_reg << "\n";
            break;
        case IR::IRBinaryOp::NE:
            std::cout << "  feq.s " << int_dest_reg << ", " << dest_reg << ", " << rhs_reg << "\n";
            std::cout << "  xori " << int_dest_reg << ", " << int_dest_reg << ", 1\n";
            break;
        case IR::IRBinaryOp::LT:
            std::cout << "  flt.s " << int_dest_reg << ", " << dest_reg << ", " << rhs_reg << "\n";
            break;
        case IR::IRBinaryOp::LE:
            std::cout << "  fle.s " << int_dest_reg << ", " << dest_reg << ", " << rhs_reg << "\n";
            break;
        case IR::IRBinaryOp::GT:
            std::cout << "  flt.s " << int_dest_reg << ", " << rhs_reg << ", " << dest_reg << "\n";
            break;
        case IR::IRBinaryOp::GE:
            std::cout << "  fle.s " << int_dest_reg << ", " << rhs_reg << ", " << dest_reg << "\n";
            break;
        default:
            assert(false && "Unsupported floating-point binary operator");
    }
    
    if (inst.GetType() == IR::IRType::IR_FLOAT) {
      if (spilled) {
        StoreFRegToVal(&inst, dest_reg);
      }
    } else {
      auto int_reg_it = reg_map.find(name);
      if (int_reg_it == reg_map.end() || int_reg_it->second.empty()) {
        StoreRegToVal(&inst, int_dest_reg);
      }
    }
    return;
  }

  if (TryEmitMulStrengthReduction(inst)) {
    return;
  }

  std::string dest_reg = "t5";
  std::string name = inst.GetOperandName();
  auto reg_it = reg_map.find(name);
  bool spilled = true;
  if (reg_it != reg_map.end() && !reg_it->second.empty() && reg_it->second[0] != 'f') {
    dest_reg = reg_it->second;
    spilled = false;
  }

  std::string rhs_reg = "t6";
  auto rhs_reg_it = reg_map.find(inst.GetRhs()->GetOperandName());
  if (rhs_reg_it != reg_map.end() && !rhs_reg_it->second.empty()) {
     rhs_reg = rhs_reg_it->second;
  }
  if (rhs_reg == dest_reg) {
     rhs_reg = "t6";
  }
  LoadValueToReg(inst.GetRhs().get(), rhs_reg);
  LoadValueToReg(inst.GetLhs().get(), dest_reg);

  std::string op_add = is_64bit ? "addw" : "add";
  std::string op_sub = is_64bit ? "subw" : "sub";
  std::string op_mul = is_64bit ? "mulw" : "mul";
  std::string op_div = is_64bit ? "divw" : "div";
  std::string op_rem = is_64bit ? "remw" : "rem";

  switch (inst.GetOp()) {
    case IR::IRBinaryOp::ADD:
      std::cout << "  " << op_add << " " << dest_reg << ", " << dest_reg << ", " << rhs_reg << "\n";
      break;
    case IR::IRBinaryOp::SUB:
      std::cout << "  " << op_sub << " " << dest_reg << ", " << dest_reg << ", " << rhs_reg << "\n";
      break;
    case IR::IRBinaryOp::MUL:
      std::cout << "  " << op_mul << " " << dest_reg << ", " << dest_reg << ", " << rhs_reg << "\n";
      break;
    case IR::IRBinaryOp::DIV:
      std::cout << "  " << op_div << " " << dest_reg << ", " << dest_reg << ", " << rhs_reg << "\n";
      break;
    case IR::IRBinaryOp::MOD:
      std::cout << "  " << op_rem << " " << dest_reg << ", " << dest_reg << ", " << rhs_reg << "\n";
      break;
    case IR::IRBinaryOp::EQ:
      std::cout << "  sub " << dest_reg << ", " << dest_reg << ", " << rhs_reg << "\n";
      std::cout << "  seqz " << dest_reg << ", " << dest_reg << "\n";
      break;
    case IR::IRBinaryOp::NE:
      std::cout << "  sub " << dest_reg << ", " << dest_reg << ", " << rhs_reg << "\n";
      std::cout << "  snez " << dest_reg << ", " << dest_reg << "\n";
      break;
    case IR::IRBinaryOp::LT:
      std::cout << "  slt " << dest_reg << ", " << dest_reg << ", " << rhs_reg << "\n";
      break;
    case IR::IRBinaryOp::GT:
      std::cout << "  slt " << dest_reg << ", " << rhs_reg << ", " << dest_reg << "\n";
      break;
    case IR::IRBinaryOp::LE:
      std::cout << "  slt " << dest_reg << ", " << rhs_reg << ", " << dest_reg << "\n";
      std::cout << "  xori " << dest_reg << ", " << dest_reg << ", 1\n";
      break;
    case IR::IRBinaryOp::GE:
      std::cout << "  slt " << dest_reg << ", " << dest_reg << ", " << rhs_reg << "\n";
      std::cout << "  xori " << dest_reg << ", " << dest_reg << ", 1\n";
      break;
    case IR::IRBinaryOp::AND:
      std::cout << "  and " << dest_reg << ", " << dest_reg << ", " << rhs_reg << "\n";
      break;
    case IR::IRBinaryOp::OR:
      std::cout << "  or " << dest_reg << ", " << dest_reg << ", t6\n";
      break;
    case IR::IRBinaryOp::XOR:
      std::cout << "  xor " << dest_reg << ", " << dest_reg << ", t6\n";
      break;
    default:
      assert(false && "Unsupported binary operator in RiscvGenerator");
  }

  if (spilled) {
    StoreRegToVal(&inst, dest_reg);
  }
}

void Back::RiscvGenerator::Visit(const IR::IRCast& inst) {
  if (inst.GetOp() == IR::IRCastOp::INT_TO_FLOAT) {
    LoadValueToReg(inst.GetValue().get(), "t5");
    std::cout << "  fcvt.s.w ft5, t5\n";
    StoreFRegToVal(&inst, "ft5");
  } else {
    LoadValueToFReg(inst.GetValue().get(), "ft5");
    std::cout << "  fcvt.w.s t5, ft5, rtz\n";
    StoreRegToVal(&inst, "t5");
  }
}

void Back::RiscvGenerator::Visit(const IR::IRAlloc& inst) {
  return;
}

void Back::RiscvGenerator::Visit(const IR::IRZeroInit& inst) {
  const int word_count = inst.GetByteSize() / 4;
  if (word_count == 0) return;

  const std::string loop_label =
      ".Lzero_init_" + std::to_string(zero_init_counter++);
  LoadAddressToReg(inst.GetPtr().get(), "t5");
  std::cout << "  li t6, " << word_count << "\n";
  std::cout << loop_label << ":\n";
  std::cout << "  sw zero, 0(t5)\n";
  std::cout << "  addi t5, t5, 4\n";
  std::cout << "  addi t6, t6, -1\n";
  std::cout << "  bnez t6, " << loop_label << "\n";
}

void Back::RiscvGenerator::Visit(const IR::IRStore& inst) {
  const auto* ptr_raw = inst.GetPtr().get();
  std::string ptr_name = ptr_raw->GetOperandName();

  if (local_allocs.count(ptr_name)) {
    // 局部标量变量：优先写入分配给它的寄存器
    auto reg_it = reg_map.find(ptr_name);
    if (reg_it != reg_map.end() && !reg_it->second.empty()) {
      std::string target_reg = reg_it->second;
      if (target_reg[0] == 'f') {
        LoadValueToFReg(inst.GetVal().get(), target_reg);
      } else {
        LoadValueToReg(inst.GetVal().get(), target_reg);
      }
      return;
    }
  }

  if (inst.GetVal()->GetType() == IR::IRType::IR_FLOAT) {
    LoadValueToFReg(inst.GetVal().get(), "ft5");
    if (dynamic_cast<const IR::IRGlobalAlloc*>(ptr_raw)) {
      std::cout << "  la t6, " << StripAt(ptr_name) << "\n";
      std::cout << "  fsw ft5, 0(t6)\n";
    } else if (local_allocs.count(ptr_name)) {
      auto it = val_stack_map.find(ptr_name);
      assert(it != val_stack_map.end());
      EmitFsw("ft5", it->second, "sp");
    } else {
      LoadValueToReg(inst.GetPtr().get(), "t6");
      std::cout << "  fsw ft5, 0(t6)\n";
    }
    return;
  }
  if (dynamic_cast<const IR::IRGlobalAlloc*>(ptr_raw)) {
    // 全局变量
    LoadValueToReg(inst.GetVal().get(), "t5");
    std::cout << "  la t6, " << StripAt(ptr_name) << "\n";
    if (GetTypeSize(inst.GetVal()->GetType()) == 8) {
      std::cout << "  sd t5, 0(t6)\n";
    } else {
      std::cout << "  sw t5, 0(t6)\n";
    }
  } else if (local_allocs.count(ptr_name)) {
    // 局部标量变量 (写入栈，当没有分配寄存器时作为 fallback)
    LoadValueToReg(inst.GetVal().get(), "t5");
    auto it = val_stack_map.find(ptr_name);
    assert(it != val_stack_map.end());
    if (GetTypeSize(inst.GetVal()->GetType()) == 8) {
      EmitSd("t5", it->second, "sp");
    } else {
      EmitSw("t5", it->second, "sp");
    }
  } else {
    // 计算出的指针地址
    // Loading a large-offset pointer into t6 uses t5 as address scratch, so
    // materialize the address before placing the stored value in t5.
    LoadValueToReg(inst.GetPtr().get(), "t6");
    LoadValueToReg(inst.GetVal().get(), "t5");
    if (GetTypeSize(inst.GetVal()->GetType()) == 8) {
      std::cout << "  sd t5, 0(t6)\n";
    } else {
      std::cout << "  sw t5, 0(t6)\n";
    }
  }
}

void Back::RiscvGenerator::Visit(const IR::IRLoad& inst) {
  const auto* ptr_raw = inst.GetPtr().get();
  std::string ptr_name = ptr_raw->GetOperandName();

  if (local_allocs.count(ptr_name)) {
    // 局部标量变量：优先从分配给它的寄存器读取
    auto reg_it = reg_map.find(ptr_name);
    if (reg_it != reg_map.end() && !reg_it->second.empty()) {
      std::string src_reg = reg_it->second;
      if (src_reg[0] == 'f') {
        StoreFRegToVal(&inst, src_reg);
      } else {
        StoreRegToVal(&inst, src_reg);
      }
      return;
    }
  }

  if (inst.GetType() == IR::IRType::IR_FLOAT) {
    if (dynamic_cast<const IR::IRGlobalAlloc*>(ptr_raw)) {
      std::cout << "  la t5, " << StripAt(ptr_name) << "\n";
      std::cout << "  flw ft5, 0(t5)\n";
    } else if (local_allocs.count(ptr_name)) {
      auto it = val_stack_map.find(ptr_name);
      assert(it != val_stack_map.end());
      EmitFlw("ft5", it->second, "sp");
    } else {
      LoadValueToReg(inst.GetPtr().get(), "t6");
      std::cout << "  flw ft5, 0(t6)\n";
    }
    StoreFRegToVal(&inst, "ft5");
    return;
  }
  if (dynamic_cast<const IR::IRGlobalAlloc*>(ptr_raw)) {
    // 全局变量
    std::cout << "  la t5, " << StripAt(ptr_name) << "\n";
    if (GetTypeSize(inst.GetType()) == 8) {
      std::cout << "  ld t5, 0(t5)\n";
    } else {
      std::cout << "  lw t5, 0(t5)\n";
    }
    StoreRegToVal(&inst, "t5");
  } else if (local_allocs.count(ptr_name)) {
    // 局部标量变量 (从栈加载，当没有分配寄存器时作为 fallback)
    auto it = val_stack_map.find(ptr_name);
    assert(it != val_stack_map.end());
    if (GetTypeSize(inst.GetType()) == 8) {
      EmitLd("t5", it->second, "sp");
    } else {
      EmitLw("t5", it->second, "sp");
    }
    StoreRegToVal(&inst, "t5");
  } else {
    // 计算出的指针地址
    LoadValueToReg(inst.GetPtr().get(), "t6");
    if (GetTypeSize(inst.GetType()) == 8) {
      std::cout << "  ld t5, 0(t6)\n";
    } else {
      std::cout << "  lw t5, 0(t6)\n";
    }
    StoreRegToVal(&inst, "t5");
  }
}

void Back::RiscvGenerator::Visit(const IR::IRJump& inst) {
  std::string target = StripPercent(inst.GetTarget()->GetOperandName());
  std::cout << "  j " << target << "\n";
}

void Back::RiscvGenerator::Visit(const IR::IRBranch& inst) {
  LoadValueToReg(inst.GetCond().get(), "t5");
  std::string then_label = StripPercent(inst.GetThenTarget()->GetOperandName());
  std::string else_label = StripPercent(inst.GetElseTarget()->GetOperandName());
  std::cout << "  bnez t5, " << then_label << "\n";
  std::cout << "  j " << else_label << "\n";
}

void Back::RiscvGenerator::Visit(const IR::IRCall& inst) {
  // 将参数加载到暂存区或栈上
  const auto& args = inst.GetArgs();
  int slot_size = GetRegisterSize();
  std::vector<IR::IRType> arg_types;
  for (const auto& arg : args) arg_types.push_back(arg->GetType());
  const auto locations = AssignArgumentLocations(arg_types);
  
  // 暂存区起始偏移（在参数溢出区之后）
  int arg_buf_offset = param_overflow_size;

  for (size_t i = 0; i < args.size(); i++) {
    const auto& location = locations[i];
    if (location.kind == ArgumentLocation::Kind::FLOAT_REGISTER) {
      LoadValueToFReg(args[i].get(), "ft5");
      EmitFsw("ft5", arg_buf_offset + i * slot_size, "sp");
    } else if (location.kind == ArgumentLocation::Kind::INTEGER_REGISTER) {
      if (GetTypeSize(args[i]->GetType()) == 8) {
        LoadValueToReg(args[i].get(), "t5");
        EmitSd("t5", arg_buf_offset + i * slot_size, "sp");
      } else {
        LoadValueToReg(args[i].get(), "t5");
        EmitSw("t5", arg_buf_offset + i * slot_size, "sp");
      }
    } else {
      // 参数溢出到栈上
      int param_offset = slot_size * location.index;
      if (args[i]->GetType() == IR::IRType::IR_FLOAT) {
        LoadValueToFReg(args[i].get(), "ft5");
        EmitFsw("ft5", param_offset, "sp");
      } else if (GetTypeSize(args[i]->GetType()) == 8) {
        LoadValueToReg(args[i].get(), "t5");
        EmitSd("t5", param_offset, "sp");
      } else {
        LoadValueToReg(args[i].get(), "t5");
        EmitSw("t5", param_offset, "sp");
      }
    }
  }

  // 从暂存区中将参数加载到 a0-a7 / fa0-fa7 寄存器中
  for (size_t i = 0; i < args.size(); i++) {
    const auto& location = locations[i];
    if (location.kind == ArgumentLocation::Kind::FLOAT_REGISTER) {
      EmitFlw("fa" + std::to_string(location.index), arg_buf_offset + i * slot_size, "sp");
    } else if (location.kind == ArgumentLocation::Kind::INTEGER_REGISTER) {
      if (GetTypeSize(args[i]->GetType()) == 8) {
        EmitLd("a" + std::to_string(location.index), arg_buf_offset + i * slot_size, "sp");
      } else {
        EmitLw("a" + std::to_string(location.index), arg_buf_offset + i * slot_size, "sp");
      }
    }
  }

  // 调用函数
  std::string func_name = StripAt(inst.GetFunc()->GetOperandName());
  std::cout << "  call " << func_name << "\n";

  // 如果有返回值，保存 a0 到栈
  if (inst.HasReturnValue()) {
    if (inst.GetType() == IR::IRType::IR_FLOAT) {
      StoreFRegToVal(&inst, "fa0");
    } else {
      StoreRegToVal(&inst, "a0");
    }
  }
}

std::vector<Back::RiscvGenerator::ArgumentLocation>
Back::RiscvGenerator::AssignArgumentLocations(
    const std::vector<IR::IRType>& types) const {
  int integer_register = 0;
  int float_register = 0;
  int stack_slot = 0;
  std::vector<ArgumentLocation> locations;
  locations.reserve(types.size());

  for (IR::IRType type : types) {
    if (type == IR::IRType::IR_FLOAT && float_register < 8) {
      locations.push_back(
          {ArgumentLocation::Kind::FLOAT_REGISTER, float_register++});
    } else if (integer_register < 8) {
      locations.push_back(
          {ArgumentLocation::Kind::INTEGER_REGISTER, integer_register++});
    } else {
      locations.push_back({ArgumentLocation::Kind::STACK, stack_slot++});
    }
  }
  return locations;
}

void Back::RiscvGenerator::EmitIndexScale(int elem_size) {
  if (elem_size == 1) {
    return;
  }

  unsigned shift = 0;
  if (optimization_level == OptimizationLevel::O1 &&
      elem_size > 0 &&
      GetPowerOfTwoShift(static_cast<uint64_t>(elem_size), shift)) {
    // Address offsets use XLEN arithmetic; slli is correct in both RV32 and RV64.
    std::cout << "  slli t6, t6, " << shift << "\n";
    return;
  }

  std::cout << "  li t4, " << elem_size << "\n";
  std::cout << "  mul t6, t6, t4\n";
}

void Back::RiscvGenerator::Visit(const IR::IRGetElemPtr& inst) {
  // 加载数组首地址到 t5
  // Base materialization is deferred until after the index load because a
  // large-offset load into t6 uses t5 as its address scratch register.
  // 加载索引到 t6
  LoadValueToReg(inst.GetIndex().get(), "t6");
  LoadAddressToReg(inst.GetArr().get(), "t5");

  EmitIndexScale(inst.GetElemSize());
  std::cout << "  add t5, t5, t6\n";

  // 结果是一个指针，存入栈
  StoreRegToVal(&inst, "t5");
}

void Back::RiscvGenerator::Visit(const IR::IRGetPtr& inst) {
  // 加载指针值到 t5（getptr 的 src 已经是指针值，不是 alloc 地址）
  // Preserve the same scratch-register ordering as getelemptr.
  // 加载索引到 t6
  LoadValueToReg(inst.GetIndex().get(), "t6");
  LoadValueToReg(inst.GetPtr().get(), "t5");

  EmitIndexScale(inst.GetElemSize());
  std::cout << "  add t5, t5, t6\n";

  // 结果是一个指针，存入栈
  StoreRegToVal(&inst, "t5");
}

void Back::RiscvGenerator::EmitLw(const std::string& reg, int offset, const std::string& base_reg) {
  if (offset >= -2048 && offset <= 2047) {
    std::cout << "  lw " << reg << ", " << offset << "(" << base_reg << ")\n";
  } else {
    if (reg != "t6") {
      std::cout << "  li " << reg << ", " << offset << "\n";
      std::cout << "  add " << reg << ", " << base_reg << ", " << reg << "\n";
      std::cout << "  lw " << reg << ", 0(" << reg << ")\n";
    } else {
      std::cout << "  li t5, " << offset << "\n";
      std::cout << "  add t5, " << base_reg << ", t5\n";
      std::cout << "  lw t6, 0(t5)\n";
    }
  }
}

void Back::RiscvGenerator::EmitSw(const std::string& reg, int offset, const std::string& base_reg) {
  if (offset >= -2048 && offset <= 2047) {
    std::cout << "  sw " << reg << ", " << offset << "(" << base_reg << ")\n";
  } else {
    std::string temp_reg = (reg == "t6") ? "t5" : "t6";
    std::cout << "  li " << temp_reg << ", " << offset << "\n";
    std::cout << "  add " << temp_reg << ", " << base_reg << ", " << temp_reg << "\n";
    std::cout << "  sw " << reg << ", 0(" << temp_reg << ")\n";
  }
}

void Back::RiscvGenerator::EmitLd(const std::string& reg, int offset, const std::string& base_reg) {
  if (offset >= -2048 && offset <= 2047) {
    std::cout << "  ld " << reg << ", " << offset << "(" << base_reg << ")\n";
  } else {
    if (reg != "t6") {
      std::cout << "  li " << reg << ", " << offset << "\n";
      std::cout << "  add " << reg << ", " << base_reg << ", " << reg << "\n";
      std::cout << "  ld " << reg << ", 0(" << reg << ")\n";
    } else {
      std::cout << "  li t5, " << offset << "\n";
      std::cout << "  add t5, " << base_reg << ", t5\n";
      std::cout << "  ld t6, 0(t5)\n";
    }
  }
}

void Back::RiscvGenerator::EmitSd(const std::string& reg, int offset, const std::string& base_reg) {
  if (offset >= -2048 && offset <= 2047) {
    std::cout << "  sd " << reg << ", " << offset << "(" << base_reg << ")\n";
  } else {
    std::string temp_reg = (reg == "t6") ? "t5" : "t6";
    std::cout << "  li " << temp_reg << ", " << offset << "\n";
    std::cout << "  add " << temp_reg << ", " << base_reg << ", " << temp_reg << "\n";
    std::cout << "  sd " << reg << ", 0(" << temp_reg << ")\n";
  }
}

void Back::RiscvGenerator::EmitFlw(const std::string& reg, int offset,
                                   const std::string& base_reg) {
  if (offset >= -2048 && offset <= 2047) {
    std::cout << "  flw " << reg << ", " << offset << "(" << base_reg
              << ")\n";
  } else {
    std::cout << "  li t6, " << offset << "\n";
    std::cout << "  add t6, " << base_reg << ", t6\n";
    std::cout << "  flw " << reg << ", 0(t6)\n";
  }
}

void Back::RiscvGenerator::EmitFsw(const std::string& reg, int offset,
                                   const std::string& base_reg) {
  if (offset >= -2048 && offset <= 2047) {
    std::cout << "  fsw " << reg << ", " << offset << "(" << base_reg
              << ")\n";
  } else {
    std::cout << "  li t6, " << offset << "\n";
    std::cout << "  add t6, " << base_reg << ", t6\n";
    std::cout << "  fsw " << reg << ", 0(t6)\n";
  }
}

void Back::RiscvGenerator::LoadValueToReg(const IR::IRValue* val, const std::string& reg) {
  if (const auto* const_val = dynamic_cast<const IR::IRConstant*>(val)) {
    auto raw_val = const_val->GetValue();
    if (const_val->GetType() == IR::IRType::IR_INT32) {
      int32_t int_val = std::get<int32_t>(raw_val);
      if (int_val == 0) {
        std::cout << "  mv " << reg << ", x0\n";
      } else {
        std::cout << "  li " << reg << ", " << int_val << "\n";
      }
    } else { // IR_FLOAT
      union { float f; uint32_t u; } conv;
      conv.f = std::get<float>(raw_val);
      uint32_t u = conv.u;
      if (!reg.empty() && reg[0] == 'f') {
        std::cout << "  li t5, " << u << "\n";
        std::cout << "  fmv.w.x " << reg << ", t5\n";
      } else {
        std::cout << "  li " << reg << ", " << u << "\n";
      }
    }
  } else if (dynamic_cast<const IR::IRGlobalAlloc*>(val)) {
    std::string raw_name = StripAt(val->GetOperandName());
    std::cout << "  la " << reg << ", " << raw_name << "\n";
    if (GetTypeSize(val->GetType()) == 8) {
      std::cout << "  ld " << reg << ", 0(" << reg << ")\n";
    } else {
      std::cout << "  lw " << reg << ", 0(" << reg << ")\n";
    }
  } else {
    // 临时变量、局部变量或函数参数
    std::string name = val->GetOperandName();
    auto reg_it = reg_map.find(name);
    if (reg_it != reg_map.end() && !reg_it->second.empty()) {
      if (reg != reg_it->second) {
        if (reg[0] == 'f' || reg_it->second[0] == 'f') {
          if (reg[0] == 'f' && reg_it->second[0] == 'f') {
            std::cout << "  fmv.s " << reg << ", " << reg_it->second << "\n";
          } else if (reg[0] == 'f') {
            std::cout << "  fmv.w.x " << reg << ", " << reg_it->second << "\n";
          } else {
            std::cout << "  fmv.x.w " << reg << ", " << reg_it->second << "\n";
          }
        } else {
          std::cout << "  mv " << reg << ", " << reg_it->second << "\n";
        }
      }
    } else {
      // 栈中加载
      auto it = val_stack_map.find(name);
      assert(it != val_stack_map.end());
      if (GetTypeSize(val->GetType()) == 8) {
        EmitLd(reg, it->second, "sp");
      } else {
        EmitLw(reg, it->second, "sp");
      }
    }
  }
}

void Back::RiscvGenerator::StoreRegToVal(const IR::IRValue* val, const std::string& reg) {
  std::string name = val->GetOperandName();
  auto reg_it = reg_map.find(name);
  if (reg_it != reg_map.end() && !reg_it->second.empty()) {
    if (reg != reg_it->second) {
      if (reg[0] == 'f' || reg_it->second[0] == 'f') {
        if (reg[0] == 'f' && reg_it->second[0] == 'f') {
          std::cout << "  fmv.s " << reg_it->second << ", " << reg << "\n";
        } else if (reg[0] == 'f') {
          std::cout << "  fmv.x.w " << reg_it->second << ", " << reg << "\n";
        } else {
          std::cout << "  fmv.w.x " << reg_it->second << ", " << reg << "\n";
        }
      } else {
        std::cout << "  mv " << reg_it->second << ", " << reg << "\n";
      }
    }
  } else {
    auto it = val_stack_map.find(name);
    assert(it != val_stack_map.end());
    if (GetTypeSize(val->GetType()) == 8) {
      EmitSd(reg, it->second, "sp");
    } else {
      EmitSw(reg, it->second, "sp");
    }
  }
}

void Back::RiscvGenerator::LoadValueToFReg(const IR::IRValue* val,
                                           const std::string& reg) {
  if (const auto* constant = dynamic_cast<const IR::IRConstant*>(val)) {
    if (constant->GetType() == IR::IRType::IR_FLOAT) {
      float value = std::get<float>(constant->GetValue());
      uint32_t bits = 0;
      std::memcpy(&bits, &value, sizeof(bits));
      std::cout << "  li t5, " << bits << "\n";
      std::cout << "  fmv.w.x " << reg << ", t5\n";
    } else {
      LoadValueToReg(val, "t5");
      std::cout << "  fcvt.s.w " << reg << ", t5\n";
    }
    return;
  }

  std::string name = val->GetOperandName();
  auto reg_it = reg_map.find(name);
  if (reg_it != reg_map.end() && !reg_it->second.empty()) {
    if (reg != reg_it->second) {
      if (reg_it->second[0] == 'f') {
        std::cout << "  fmv.s " << reg << ", " << reg_it->second << "\n";
      } else {
        std::cout << "  fmv.w.x " << reg << ", " << reg_it->second << "\n";
      }
    }
  } else {
    auto it = val_stack_map.find(name);
    assert(it != val_stack_map.end());
    EmitFlw(reg, it->second, "sp");
  }
}

void Back::RiscvGenerator::StoreFRegToVal(const IR::IRValue* val,
                                          const std::string& reg) {
  std::string name = val->GetOperandName();
  auto reg_it = reg_map.find(name);
  if (reg_it != reg_map.end() && !reg_it->second.empty()) {
    if (reg != reg_it->second) {
      if (reg_it->second[0] == 'f') {
        std::cout << "  fmv.s " << reg_it->second << ", " << reg << "\n";
      } else {
        std::cout << "  fmv.x.w " << reg_it->second << ", " << reg << "\n";
      }
    }
  } else {
    auto it = val_stack_map.find(name);
    assert(it != val_stack_map.end());
    EmitFsw(reg, it->second, "sp");
  }
}

void Back::RiscvGenerator::LoadAddressToReg(const IR::IRValue* val, const std::string& reg) {
  if (const auto* alloc = dynamic_cast<const IR::IRAlloc*>(val)) {
    // 局部数组/变量：计算栈上地址 sp + offset
    auto it = val_stack_map.find(alloc->GetName());
    assert(it != val_stack_map.end());
    int offset = it->second;
    if (offset >= -2048 && offset <= 2047) {
      std::cout << "  addi " << reg << ", sp, " << offset << "\n";
    } else {
      std::cout << "  li " << reg << ", " << offset << "\n";
      std::cout << "  add " << reg << ", sp, " << reg << "\n";
    }
  } else if (dynamic_cast<const IR::IRGlobalAlloc*>(val)) {
    // 全局变量：加载符号地址
    std::string raw_name = StripAt(val->GetOperandName());
    std::cout << "  la " << reg << ", " << raw_name << "\n";
  } else {
    // 其他（如 getelemptr 的结果或指针参数）：值本身就是地址
    std::string name = val->GetOperandName();
    auto reg_it = reg_map.find(name);
    if (reg_it != reg_map.end() && !reg_it->second.empty()) {
      if (reg != reg_it->second) {
        std::cout << "  mv " << reg << ", " << reg_it->second << "\n";
      }
    } else {
      auto it = val_stack_map.find(name);
      assert(it != val_stack_map.end());
      if (GetRegisterSize() == 8) {
        EmitLd(reg, it->second, "sp");
      } else {
        EmitLw(reg, it->second, "sp");
      }
    }
  }
}

void Back::RiscvGenerator::StoreRegToValByName(const std::string& name, const std::string& reg, IR::IRType type) {
  auto it = val_stack_map.find(name);
  assert(it != val_stack_map.end());
  if (reg.size() > 0 && reg[0] == 'f') {
    EmitFsw(reg, it->second, "sp");
  } else if (GetTypeSize(type) == 8) {
    EmitSd(reg, it->second, "sp");
  } else {
    EmitSw(reg, it->second, "sp");
  }
}

void Back::RiscvGenerator::LoadValueToRegByName(const std::string& name, const std::string& reg, IR::IRType type) {
  auto it = val_stack_map.find(name);
  assert(it != val_stack_map.end());
  if (reg.size() > 0 && reg[0] == 'f') {
    EmitFlw(reg, it->second, "sp");
  } else if (GetTypeSize(type) == 8) {
    EmitLd(reg, it->second, "sp");
  } else {
    EmitLw(reg, it->second, "sp");
  }
}

