#include "riscv.hpp"
#include <iostream>
#include <cassert>

void Back::RiscvGenerator::Generate(const IR::IRProgram& program) {
  Visit(program);
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

  if (global.IsZeroInit() || global.GetInitVals().empty()) {
    std::cout << "  .data\n";
    std::cout << "  .globl " << name << "\n";
    std::cout << name << ":\n";
    std::cout << "  .zero " << global.GetAllocSize() << "\n";
  } else {
    std::cout << "  .data\n";
    std::cout << "  .globl " << name << "\n";
    std::cout << name << ":\n";
    for (int32_t v : global.GetInitVals()) {
      std::cout << "  .word " << v << "\n";
    }
  }
  std::cout << "\n";
}

void Back::RiscvGenerator::Visit(const IR::IRFunction& func) {
  val_stack_map.clear();
  local_allocs.clear();
  has_call = false;
  max_param_count = 0;

  int offset = 0;

  // 为函数参数分配栈空间
  for (size_t i = 0; i < func.params.size(); i++) {
    val_stack_map[func.params[i]->GetOperandName()] = offset;
    offset += GetTypeSize(func.params[i]->GetType());
  }

  // 为所有具有返回值的指令分配栈偏移量
  for (const auto& block : func.blocks) {
    for (const auto& inst : block->insts) {
      // 检测 call 指令
      if (const auto* call = dynamic_cast<const IR::IRCall*>(inst.get())) {
        has_call = true;
        int arg_count = (int)call->GetArgs().size();
        if (arg_count > max_param_count) max_param_count = arg_count;
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

      if (has_retval) {
        if (const auto* alloc = dynamic_cast<const IR::IRAlloc*>(inst.get())) {
          // 数组或变量分配：按实际大小分配栈空间
          local_allocs.insert(val_name);
          val_stack_map[val_name] = offset;
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
        } else {
          val_stack_map[val_name] = offset;
          offset += GetTypeSize(inst->GetType());
        }
      }
    }
  }

  int slot_size = GetRegisterSize();
  int param_overflow_size = 0;
  if (max_param_count > 8) {
    param_overflow_size = slot_size * (max_param_count - 8);
  }

  // ra 保存空间
  int ra_size = has_call ? GetRegisterSize() : 0;

  // 计算总栈帧大小
  int total = param_overflow_size + offset + ra_size;
  current_stack_size = (total + 15) / 16 * 16;

  // 调整所有 val_stack_map 偏移量，加上参数溢出区的偏移
  for (auto& kv : val_stack_map) {
    kv.second += param_overflow_size;
  }

  std::cout << "  .globl " << func.name << "\n";
  std::cout << func.name << ":\n";

  // Prologue：分配栈帧
  if (current_stack_size > 0) {
    if (current_stack_size <= 2048) {
      std::cout << "  addi sp, sp, -" << current_stack_size << "\n";
    } else {
      std::cout << "  li t0, -" << current_stack_size << "\n";
      std::cout << "  add sp, sp, t0\n";
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

  // 将寄存器参数保存到栈上
  for (size_t i = 0; i < func.params.size() && i < 8; i++) {
    std::string param_name = func.params[i]->GetOperandName();
    auto it = val_stack_map.find(param_name);
    assert(it != val_stack_map.end());
    if (GetTypeSize(func.params[i]->GetType()) == 8) {
      EmitSd("a" + std::to_string(i), it->second, "sp");
    } else {
      EmitSw("a" + std::to_string(i), it->second, "sp");
    }
  }
  // 多于8个的参数映射到调用者栈帧中的位置
  for (size_t i = 8; i < func.params.size(); i++) {
    std::string param_name = func.params[i]->GetOperandName();
    int caller_offset = current_stack_size + slot_size * ((int)i - 8);
    auto it = val_stack_map.find(param_name);
    assert(it != val_stack_map.end());
    if (GetTypeSize(func.params[i]->GetType()) == 8) {
      EmitLd("t0", caller_offset, "sp");
      EmitSd("t0", it->second, "sp");
    } else {
      EmitLw("t0", caller_offset, "sp");
      EmitSw("t0", it->second, "sp");
    }
  }

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
    Visit(*inst);
  }
}

void Back::RiscvGenerator::Visit(const IR::IRInstruction& inst) {
  if (const auto* p = dynamic_cast<const IR::IRReturn*>(&inst)) {
    Visit(*p);
  } else if (const auto* p = dynamic_cast<const IR::IRBinary*>(&inst)) {
    Visit(*p);
  } else if (const auto* p = dynamic_cast<const IR::IRAlloc*>(&inst)) {
    Visit(*p);
  } else if (const auto* p = dynamic_cast<const IR::IRStore*>(&inst)) {
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
    LoadValueToReg(ret_inst.GetValue().get(), "a0");
  }
  // Epilogue
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
      std::cout << "  li t0, " << current_stack_size << "\n";
      std::cout << "  add sp, sp, t0\n";
    }
  }
  std::cout << "  ret\n";
}

void Back::RiscvGenerator::Visit(const IR::IRBinary& inst) {
  LoadValueToReg(inst.GetLhs().get(), "t0");
  LoadValueToReg(inst.GetRhs().get(), "t1");

  std::string op_add = is_64bit ? "addw" : "add";
  std::string op_sub = is_64bit ? "subw" : "sub";
  std::string op_mul = is_64bit ? "mulw" : "mul";
  std::string op_div = is_64bit ? "divw" : "div";
  std::string op_rem = is_64bit ? "remw" : "rem";

  switch (inst.GetOp()) {
    case IR::IRBinaryOp::ADD:
      std::cout << "  " << op_add << " t0, t0, t1\n";
      break;
    case IR::IRBinaryOp::SUB:
      std::cout << "  " << op_sub << " t0, t0, t1\n";
      break;
    case IR::IRBinaryOp::MUL:
      std::cout << "  " << op_mul << " t0, t0, t1\n";
      break;
    case IR::IRBinaryOp::DIV:
      std::cout << "  " << op_div << " t0, t0, t1\n";
      break;
    case IR::IRBinaryOp::MOD:
      std::cout << "  " << op_rem << " t0, t0, t1\n";
      break;
    case IR::IRBinaryOp::EQ:
      std::cout << "  sub t0, t0, t1\n";
      std::cout << "  seqz t0, t0\n";
      break;
    case IR::IRBinaryOp::NE:
      std::cout << "  sub t0, t0, t1\n";
      std::cout << "  snez t0, t0\n";
      break;
    case IR::IRBinaryOp::LT:
      std::cout << "  slt t0, t0, t1\n";
      break;
    case IR::IRBinaryOp::GT:
      std::cout << "  slt t0, t1, t0\n";
      break;
    case IR::IRBinaryOp::LE:
      std::cout << "  slt t0, t1, t0\n";
      std::cout << "  xori t0, t0, 1\n";
      break;
    case IR::IRBinaryOp::GE:
      std::cout << "  slt t0, t0, t1\n";
      std::cout << "  xori t0, t0, 1\n";
      break;
    case IR::IRBinaryOp::AND:
      std::cout << "  and t0, t0, t1\n";
      break;
    case IR::IRBinaryOp::OR:
      std::cout << "  or t0, t0, t1\n";
      break;
    case IR::IRBinaryOp::XOR:
      std::cout << "  xor t0, t0, t1\n";
      break;
    default:
      assert(false && "Unsupported binary operator in RiscvGenerator");
  }

  StoreRegToVal(&inst, "t0");
}

void Back::RiscvGenerator::Visit(const IR::IRAlloc& inst) {
  return;
}

void Back::RiscvGenerator::Visit(const IR::IRStore& inst) {
  const auto* ptr_raw = inst.GetPtr().get();
  std::string ptr_name = ptr_raw->GetOperandName();
  if (dynamic_cast<const IR::IRGlobalAlloc*>(ptr_raw)) {
    // 全局变量
    LoadValueToReg(inst.GetVal().get(), "t0");
    std::cout << "  la t1, " << StripAt(ptr_name) << "\n";
    if (GetTypeSize(inst.GetVal()->GetType()) == 8) {
      std::cout << "  sd t0, 0(t1)\n";
    } else {
      std::cout << "  sw t0, 0(t1)\n";
    }
  } else if (local_allocs.count(ptr_name)) {
    // 局部标量变量
    LoadValueToReg(inst.GetVal().get(), "t0");
    auto it = val_stack_map.find(ptr_name);
    assert(it != val_stack_map.end());
    if (GetTypeSize(inst.GetVal()->GetType()) == 8) {
      EmitSd("t0", it->second, "sp");
    } else {
      EmitSw("t0", it->second, "sp");
    }
  } else {
    // 计算出的指针地址
    LoadValueToReg(inst.GetVal().get(), "t0");
    LoadValueToReg(inst.GetPtr().get(), "t1");
    if (GetTypeSize(inst.GetVal()->GetType()) == 8) {
      std::cout << "  sd t0, 0(t1)\n";
    } else {
      std::cout << "  sw t0, 0(t1)\n";
    }
  }
}

void Back::RiscvGenerator::Visit(const IR::IRLoad& inst) {
  const auto* ptr_raw = inst.GetPtr().get();
  std::string ptr_name = ptr_raw->GetOperandName();
  if (dynamic_cast<const IR::IRGlobalAlloc*>(ptr_raw)) {
    // 全局变量
    std::cout << "  la t0, " << StripAt(ptr_name) << "\n";
    if (GetTypeSize(inst.GetType()) == 8) {
      std::cout << "  ld t0, 0(t0)\n";
    } else {
      std::cout << "  lw t0, 0(t0)\n";
    }
    StoreRegToVal(&inst, "t0");
  } else if (local_allocs.count(ptr_name)) {
    // 局部标量变量
    auto it = val_stack_map.find(ptr_name);
    assert(it != val_stack_map.end());
    if (GetTypeSize(inst.GetType()) == 8) {
      EmitLd("t0", it->second, "sp");
    } else {
      EmitLw("t0", it->second, "sp");
    }
    StoreRegToVal(&inst, "t0");
  } else {
    // 计算出的指针地址
    LoadValueToReg(inst.GetPtr().get(), "t1");
    if (GetTypeSize(inst.GetType()) == 8) {
      std::cout << "  ld t0, 0(t1)\n";
    } else {
      std::cout << "  lw t0, 0(t1)\n";
    }
    StoreRegToVal(&inst, "t0");
  }
}

void Back::RiscvGenerator::Visit(const IR::IRJump& inst) {
  std::string target = StripPercent(inst.GetTarget()->GetOperandName());
  std::cout << "  j " << target << "\n";
}

void Back::RiscvGenerator::Visit(const IR::IRBranch& inst) {
  LoadValueToReg(inst.GetCond().get(), "t0");
  std::string then_label = StripPercent(inst.GetThenTarget()->GetOperandName());
  std::string else_label = StripPercent(inst.GetElseTarget()->GetOperandName());
  std::cout << "  bnez t0, " << then_label << "\n";
  std::cout << "  j " << else_label << "\n";
}

void Back::RiscvGenerator::Visit(const IR::IRCall& inst) {
  // 将参数加载到 a0-a7 或栈上
  const auto& args = inst.GetArgs();
  int slot_size = GetRegisterSize();
  for (size_t i = 0; i < args.size(); i++) {
    if (i < 8) {
      LoadValueToReg(args[i].get(), "a" + std::to_string(i));
    } else {
      // 参数溢出到栈上
      LoadValueToReg(args[i].get(), "t0");
      int param_offset = slot_size * ((int)i - 8);
      if (GetTypeSize(args[i]->GetType()) == 8) {
        EmitSd("t0", param_offset, "sp");
      } else {
        EmitSw("t0", param_offset, "sp");
      }
    }
  }

  // 调用函数
  std::string func_name = StripAt(inst.GetFunc()->GetOperandName());
  std::cout << "  call " << func_name << "\n";

  // 如果有返回值，保存 a0 到栈
  if (inst.HasReturnValue()) {
    StoreRegToVal(&inst, "a0");
  }
}

void Back::RiscvGenerator::Visit(const IR::IRGetElemPtr& inst) {
  // 加载数组首地址到 t0
  LoadAddressToReg(inst.GetArr().get(), "t0");
  // 加载索引到 t1
  LoadValueToReg(inst.GetIndex().get(), "t1");

  int elem_size = inst.GetElemSize();
  if (elem_size != 1) {
    std::cout << "  li t2, " << elem_size << "\n";
    std::cout << "  mul t1, t1, t2\n";
  }
  std::cout << "  add t0, t0, t1\n";

  // 结果是一个指针，存入栈
  StoreRegToVal(&inst, "t0");
}

void Back::RiscvGenerator::Visit(const IR::IRGetPtr& inst) {
  // 加载指针值到 t0（getptr 的 src 已经是指针值，不是 alloc 地址）
  LoadValueToReg(inst.GetPtr().get(), "t0");
  // 加载索引到 t1
  LoadValueToReg(inst.GetIndex().get(), "t1");

  int elem_size = inst.GetElemSize();
  if (elem_size != 1) {
    std::cout << "  li t2, " << elem_size << "\n";
    std::cout << "  mul t1, t1, t2\n";
  }
  std::cout << "  add t0, t0, t1\n";

  // 结果是一个指针，存入栈
  StoreRegToVal(&inst, "t0");
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
    std::cout << "  li t6, " << offset << "\n";
    std::cout << "  add t6, " << base_reg << ", t6\n";
    std::cout << "  sw " << reg << ", 0(t6)\n";
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
    std::cout << "  li t6, " << offset << "\n";
    std::cout << "  add t6, " << base_reg << ", t6\n";
    std::cout << "  sd " << reg << ", 0(t6)\n";
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
    // 临时变量、局部变量或函数参数：从栈中加载
    std::string name = val->GetOperandName();
    auto it = val_stack_map.find(name);
    assert(it != val_stack_map.end());
    if (GetTypeSize(val->GetType()) == 8) {
      EmitLd(reg, it->second, "sp");
    } else {
      EmitLw(reg, it->second, "sp");
    }
  }
}

void Back::RiscvGenerator::StoreRegToVal(const IR::IRValue* val, const std::string& reg) {
  auto it = val_stack_map.find(val->GetOperandName());
  assert(it != val_stack_map.end());
  if (GetTypeSize(val->GetType()) == 8) {
    EmitSd(reg, it->second, "sp");
  } else {
    EmitSw(reg, it->second, "sp");
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
    // 其他（如 getelemptr 的结果或指针参数）：值本身就是地址，从栈加载
    std::string name = val->GetOperandName();
    auto it = val_stack_map.find(name);
    assert(it != val_stack_map.end());
    if (GetRegisterSize() == 8) {
      EmitLd(reg, it->second, "sp");
    } else {
      EmitLw(reg, it->second, "sp");
    }
  }
}
