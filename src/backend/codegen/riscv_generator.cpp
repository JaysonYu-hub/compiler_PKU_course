#include "riscv_generator.hpp"
#include "../../IR/passes/liveness.hpp"
#include "../regalloc/linear_scan_allocator.hpp"
#include <iostream>
#include <cassert>
#include <cstring>
#include <algorithm>

// Internal helpers
namespace {

bool GetPowerOfTwoShift(uint64_t value, unsigned& shift) {
  if (value == 0 || (value & (value - 1)) != 0) return false;
  shift = 0;
  while (value > 1) { value >>= 1; ++shift; }
  return true;
}

}

// Generate (top-level entry)
void Back::RiscvGenerator::Generate(const IR::IRProgram& program) {
  lva_ = std::make_unique<IR::Pass::LiveVarAnalysis>();
  lva_->Analyze(const_cast<IR::IRProgram*>(&program));
  Visit(program);
}

// String helpers
std::string Back::RiscvGenerator::StripAt(const std::string& name) {
  return (!name.empty() && name[0] == '@') ? name.substr(1) : name;
}
std::string Back::RiscvGenerator::StripPercent(const std::string& name) {
  return (!name.empty() && name[0] == '%') ? name.substr(1) : name;
}

// Program & global alloc
void Back::RiscvGenerator::Visit(const IR::IRProgram& program) {
  for (const auto& g : program.globals) Visit(*g);
  emitter_.EmitDirective(".text");
  for (const auto& f : program.funcs) Visit(*f);
}

void Back::RiscvGenerator::Visit(const IR::IRGlobalAlloc& global) {
  std::string name = StripAt(global.GetOperandName());
  emitter_.EmitDirective(".data");
  emitter_.EmitInst(".globl", name);
  emitter_.EmitLabel(name);

  if (global.IsZeroInit() ||
      (global.GetInitVals().empty() && global.GetInitFloatVals().empty())) {
    emitter_.EmitInst(".zero", global.GetAllocSize());
  } else if (!global.GetInitFloatVals().empty()) {
    for (float v : global.GetInitFloatVals()) {
      int32_t bits = 0;
      std::memcpy(&bits, &v, sizeof(bits));
      emitter_.EmitInst(".word", bits);
    }
  } else {
    for (int32_t v : global.GetInitVals()) emitter_.EmitInst(".word", v);
  }
  emitter_.Out() << "\n";
}

// Function
void Back::RiscvGenerator::Visit(const IR::IRFunction& func) {
  val_stack_map_.clear();
  val_type_map_.clear();
  local_allocs_.clear();
  reg_map_.clear();
  spill_offsets_.clear();
  used_s_regs_.clear();
  has_call_            = false;
  max_stack_arg_count_ = 0;

  std::unordered_set<std::string> virtual_regs;
  for (const auto& block : func.blocks) {
    for (const auto& inst : block->insts) {
      bool has_retval = false;
      std::string val_name;
      if (const auto* ni = dynamic_cast<const IR::IRNameInstruction*>(inst.get())) {
        if (const auto* al = dynamic_cast<const IR::IRAlloc*>(inst.get())) {
          if (!al->IsArray()) { has_retval = true; val_name = ni->GetName(); }
        } else {
          has_retval = true; val_name = ni->GetName();
        }
      } else if (const auto* ci = dynamic_cast<const IR::IRCall*>(inst.get())) {
        if (ci->HasReturnValue()) { has_retval = true; val_name = ci->GetOperandName(); }
      }
      if (has_retval) virtual_regs.insert(val_name);
    }
  }

  if (lva_) {
    auto it = lva_->func_intervals.find(func.name);
    if (it != lva_->func_intervals.end()) {
      std::unordered_map<std::string, std::array<long long int, 2>> filtered;
      for (const auto& [var, range] : it->second)
        if (virtual_regs.count(var)) filtered[var] = range;

      LinearScanAllocator alloc(filtered, GetRegSize());
      alloc.Allocate();
      reg_map_ = alloc.GetRegMap();
      for (const auto& s : alloc.GetSpilledIntervals())
        spill_offsets_[s.var] = s.stack_offset;
    }
  }

  for (const auto& [var, reg] : reg_map_) {
    if (!reg.empty() && reg[0] == 's') {
      if (std::find(used_s_regs_.begin(), used_s_regs_.end(), reg) == used_s_regs_.end())
        used_s_regs_.push_back(reg);
    }
  }

  int spill_size = 0;
  for (const auto& [v, off] : spill_offsets_)
    if (off + GetRegSize() > spill_size) spill_size = off + GetRegSize();

  int offset = spill_size;
  s_reg_start_offset_ = offset;
  offset += (int)used_s_regs_.size() * GetRegSize();

  for (const auto& block : func.blocks) {
    for (const auto& inst : block->insts) {
      if (const auto* ni = dynamic_cast<const IR::IRNameInstruction*>(inst.get())) {
        if (const auto* al = dynamic_cast<const IR::IRAlloc*>(inst.get())) {
          local_allocs_.insert(ni->GetName());
          val_stack_map_[ni->GetName()] = offset;
          val_type_map_[ni->GetName()]  = al->GetAllocatedType();
          int sz = al->IsArray() ? al->GetAllocSize() : GetTypeSize(al->GetAllocatedType());
          offset += sz;
        }
      }
    }
  }

  for (size_t i = 0; i < func.params.size(); ++i) {
    std::string pn = func.params[i]->GetOperandName();
    val_type_map_[pn] = func.params[i]->GetType();
    if (spill_offsets_.count(pn)) {
      val_stack_map_[pn] = spill_offsets_[pn];
    } else {
      val_stack_map_[pn] = offset;
      offset += GetTypeSize(func.params[i]->GetType());
    }
  }

  for (const auto& block : func.blocks) {
    for (const auto& inst : block->insts) {
      bool has_retval = false; std::string val_name;
      if (const auto* ni = dynamic_cast<const IR::IRNameInstruction*>(inst.get())) {
        has_retval = true; val_name = ni->GetName();
      } else if (const auto* ci = dynamic_cast<const IR::IRCall*>(inst.get())) {
        if (ci->HasReturnValue()) { has_retval = true; val_name = ci->GetOperandName(); }
      }
      if (has_retval) {
        if (const auto* al = dynamic_cast<const IR::IRAlloc*>(inst.get()))
          val_type_map_[val_name] = al->GetAllocatedType();
        else
          val_type_map_[val_name] = inst->GetType();
      }
    }
  }
  for (const auto& [v, off] : spill_offsets_)
    if (!val_stack_map_.count(v)) val_stack_map_[v] = off;

  for (const auto& block : func.blocks) {
    for (const auto& inst : block->insts) {
      if (const auto* ci = dynamic_cast<const IR::IRCall*>(inst.get())) {
        has_call_ = true;
        std::vector<IR::IRType> arg_types;
        for (const auto& a : ci->GetArgs()) arg_types.push_back(a->GetType());
        int stack_args = 0;
        for (const auto& loc : calling_convention_.AssignArgumentLocations(arg_types))
          if (loc.kind == ArgumentLocation::Kind::STACK) ++stack_args;
        if (stack_args > max_stack_arg_count_) max_stack_arg_count_ = stack_args;
      }
      bool has_retval = false; std::string val_name;
      if (const auto* ni = dynamic_cast<const IR::IRNameInstruction*>(inst.get())) {
        has_retval = true; val_name = ni->GetName();
      } else if (const auto* ci = dynamic_cast<const IR::IRCall*>(inst.get())) {
        if (ci->HasReturnValue()) { has_retval = true; val_name = ci->GetOperandName(); }
      }
      if (has_retval && !val_stack_map_.count(val_name)) {
        val_stack_map_[val_name] = offset;
        if (const auto* al = dynamic_cast<const IR::IRAlloc*>(inst.get())) {
          val_type_map_[val_name] = al->GetAllocatedType();
          offset += GetTypeSize(al->GetAllocatedType());
        } else {
          val_type_map_[val_name] = inst->GetType();
          offset += GetTypeSize(inst->GetType());
        }
      }
    }
  }

  int local_alloc_size = offset - spill_size - (int)used_s_regs_.size() * GetRegSize();

  StackLayoutBuilder builder;
  stack_layout_ = builder.Build(used_s_regs_, spill_offsets_,
                                max_stack_arg_count_, has_call_,
                                local_alloc_size,
                                GetRegSize(), target_.GetPointerSize() == 8 ? 16 : 8);
  current_stack_size_  = stack_layout_.total_size;
  param_overflow_size_ = stack_layout_.param_overflow_size;
  int arg_buf_size     = stack_layout_.arg_buffer_size;

  for (auto& [v, off] : val_stack_map_) off += param_overflow_size_ + arg_buf_size;
  s_reg_start_offset_ += param_overflow_size_ + arg_buf_size;

  emitter_.EmitInst(".globl", func.name);
  emitter_.EmitLabel(func.name);

  if (current_stack_size_ > 0) {
    if (current_stack_size_ <= 2048)
      emitter_.EmitInst("addi", "sp", "sp", "-" + std::to_string(current_stack_size_));
    else {
      emitter_.EmitInst("li", "t5", "-" + std::to_string(current_stack_size_));
      emitter_.EmitInst("add", "sp", "sp", "t5");
    }
  }

  if (has_call_) {
    int ra_off = current_stack_size_ - GetRegSize();
    if (target_.Is64Bit()) emitter_.EmitSd("ra", ra_off);
    else           emitter_.EmitSw("ra", ra_off);
  }

  for (size_t i = 0; i < used_s_regs_.size(); ++i) {
    int s_off = s_reg_start_offset_ + (int)i * GetRegSize();
    if (target_.Is64Bit()) emitter_.EmitSd(used_s_regs_[i], s_off);
    else           emitter_.EmitSw(used_s_regs_[i], s_off);
  }

  std::vector<IR::IRType> param_types;
  for (const auto& p : func.params) param_types.push_back(p->GetType());
  const auto param_locs = calling_convention_.AssignArgumentLocations(param_types);

  for (size_t i = 0; i < func.params.size(); ++i) {
    std::string pn  = func.params[i]->GetOperandName();
    int         stk = val_stack_map_.at(pn);
    const auto& loc = param_locs[i];
    auto reg_it     = reg_map_.find(pn);
    std::string p_reg = (reg_it != reg_map_.end()) ? reg_it->second : "";

    if (loc.kind == ArgumentLocation::Kind::FLOAT_REGISTER) {
      std::string src = "fa" + std::to_string(loc.index);
      if (!p_reg.empty() && p_reg != src)
        emitter_.EmitInst(p_reg[0] == 'f' ? "fmv.s" : "fmv.x.w", p_reg, src);
      emitter_.EmitFsw(src, stk);
    } else if (loc.kind == ArgumentLocation::Kind::INTEGER_REGISTER) {
      std::string src = "a" + std::to_string(loc.index);
      if (!p_reg.empty() && p_reg != src)
        emitter_.EmitInst(p_reg[0] == 'f' ? "fmv.w.x" : "mv", p_reg, src);
      if (GetTypeSize(func.params[i]->GetType()) == 8) emitter_.EmitSd(src, stk);
      else                                              emitter_.EmitSw(src, stk);
    } else {
      int caller_off = current_stack_size_ + GetRegSize() * loc.index;
      std::string treg = !p_reg.empty() ? p_reg : "t5";
      if (func.params[i]->GetType() == IR::IRType::IR_FLOAT) {
        std::string freg = treg[0] == 'f' ? treg : "ft5";
        emitter_.EmitFlw(freg, caller_off);
        emitter_.EmitFsw(freg, stk);
      } else if (GetTypeSize(func.params[i]->GetType()) == 8) {
        emitter_.EmitLd(treg, caller_off);
        emitter_.EmitSd(treg, stk);
      } else {
        emitter_.EmitLw(treg, caller_off);
        emitter_.EmitSw(treg, stk);
      }
    }
  }

  for (const auto& block : func.blocks) Visit(*block);
}

// Basic block
void Back::RiscvGenerator::Visit(const IR::IRBasicBlock& block) {
  std::string label = StripPercent(block.name);
  if (label != "entry") emitter_.EmitLabel(label);

  for (const auto& inst : block.insts) {
    bool is_call = (inst->inst_type == IR::IRInstructionType::IR_INST_CALL);

    std::vector<std::pair<std::string, std::string>> regs_to_save;
    if (is_call && lva_) {
      auto live_it = lva_->live_out_inst.find(inst);
      if (live_it != lva_->live_out_inst.end()) {
        std::string def_name;
        if (auto cp = std::dynamic_pointer_cast<IR::IRCall>(inst))
          if (cp->HasReturnValue()) def_name = cp->GetOperandName();
        for (const auto& var : live_it->second) {
          if (var == def_name) continue;
          auto ri = reg_map_.find(var);
          if (ri != reg_map_.end() && !ri->second.empty() && ri->second[0] != 's')
            regs_to_save.push_back({var, ri->second});
        }
      }
    }

    for (const auto& [var, reg] : regs_to_save) {
      auto ti = val_type_map_.find(var);
      IR::IRType t = (ti != val_type_map_.end()) ? ti->second : IR::IRType::IR_INT32;
      StoreRegToValByName(var, reg, t);
    }

    Visit(*inst);

    for (const auto& [var, reg] : regs_to_save) {
      auto ti = val_type_map_.find(var);
      IR::IRType t = (ti != val_type_map_.end()) ? ti->second : IR::IRType::IR_INT32;
      LoadValueToRegByName(var, reg, t);
    }
  }
}

// Instruction dispatch
void Back::RiscvGenerator::Visit(const IR::IRInstruction& inst) {
  if (auto* p = dynamic_cast<const IR::IRReturn*>    (&inst)) { Visit(*p); return; }
  if (auto* p = dynamic_cast<const IR::IRBinary*>    (&inst)) { Visit(*p); return; }
  if (auto* p = dynamic_cast<const IR::IRCast*>      (&inst)) { Visit(*p); return; }
  if (auto* p = dynamic_cast<const IR::IRAlloc*>     (&inst)) { Visit(*p); return; }
  if (auto* p = dynamic_cast<const IR::IRStore*>     (&inst)) { Visit(*p); return; }
  if (auto* p = dynamic_cast<const IR::IRLoad*>      (&inst)) { Visit(*p); return; }
  if (auto* p = dynamic_cast<const IR::IRJump*>      (&inst)) { Visit(*p); return; }
  if (auto* p = dynamic_cast<const IR::IRBranch*>    (&inst)) { Visit(*p); return; }
  if (auto* p = dynamic_cast<const IR::IRCall*>      (&inst)) { Visit(*p); return; }
  if (auto* p = dynamic_cast<const IR::IRGetElemPtr*>(&inst)) { Visit(*p); return; }
  if (auto* p = dynamic_cast<const IR::IRGetPtr*>    (&inst)) { Visit(*p); return; }
}

// Return
void Back::RiscvGenerator::Visit(const IR::IRReturn& inst) {
  if (inst.GetValue()) {
    if (inst.GetValue()->GetType() == IR::IRType::IR_FLOAT)
      LoadValueToFReg(inst.GetValue().get(), "fa0");
    else
      LoadValueToReg(inst.GetValue().get(), "a0");
  }

  for (size_t i = 0; i < used_s_regs_.size(); ++i) {
    int off = s_reg_start_offset_ + (int)i * GetRegSize();
    if (target_.Is64Bit()) emitter_.EmitLd(used_s_regs_[i], off);
    else           emitter_.EmitLw(used_s_regs_[i], off);
  }
  if (has_call_) {
    int ra_off = current_stack_size_ - GetRegSize();
    if (target_.Is64Bit()) emitter_.EmitLd("ra", ra_off);
    else           emitter_.EmitLw("ra", ra_off);
  }
  if (current_stack_size_ > 0) {
    if (current_stack_size_ <= 2047)
      emitter_.EmitInst("addi", "sp", "sp", std::to_string(current_stack_size_));
    else {
      emitter_.EmitInst("li", "t5", std::to_string(current_stack_size_));
      emitter_.EmitInst("add", "sp", "sp", "t5");
    }
  }
  emitter_.EmitInst("ret");
}

// Mul strength reduction (O1 only)
bool Back::RiscvGenerator::TryEmitMulStrengthReduction(const IR::IRBinary& inst) {
  if (opt_level_ != OptimizationLevel::O1 ||
      inst.GetOp() != IR::IRBinaryOp::MUL ||
      inst.GetOperandType() != IR::IRType::IR_INT32) return false;

  auto try_const = [&](const IR::IRConstant* c, const IR::IRValue* other) -> bool {
    if (!c || c->GetType() != IR::IRType::IR_INT32) return false;
    int64_t sv = std::get<int32_t>(c->GetValue());
    if (sv == 0) {
      emitter_.EmitInst("mv", "t5", "x0");
      StoreRegToVal(&inst, "t5");
      return true;
    }
    bool neg = sv < 0;
    uint64_t mag = neg ? static_cast<uint64_t>(-sv) : static_cast<uint64_t>(sv);
    unsigned shift = 0;
    if (!GetPowerOfTwoShift(mag, shift)) return false;
    LoadValueToReg(other, "t5");
    if (shift != 0) emitter_.EmitInst(target_.Is64Bit() ? "slliw" : "slli", "t5", "t5", std::to_string(shift));
    if (neg)        emitter_.EmitInst(target_.Is64Bit() ? "subw"  : "sub",  "t5", "x0", "t5");
    StoreRegToVal(&inst, "t5");
    return true;
  };

  const auto* rhs_c = dynamic_cast<const IR::IRConstant*>(inst.GetRhs().get());
  if (try_const(rhs_c, inst.GetLhs().get())) return true;
  const auto* lhs_c = dynamic_cast<const IR::IRConstant*>(inst.GetLhs().get());
  return try_const(lhs_c, inst.GetRhs().get());
}

// Binary
void Back::RiscvGenerator::Visit(const IR::IRBinary& inst) {
  if (inst.GetOperandType() == IR::IRType::IR_FLOAT) {
    std::string name = inst.GetOperandName();
    auto ri = reg_map_.find(name);
    bool spilled = true;
    std::string dest_reg = "ft5";
    if (ri != reg_map_.end() && !ri->second.empty() && ri->second[0] == 'f') {
      dest_reg = ri->second; spilled = false;
    }
    std::string rhs_reg = "ft6";
    LoadValueToFReg(inst.GetRhs().get(), "ft6");
    LoadValueToFReg(inst.GetLhs().get(), dest_reg);
    std::string int_dest = "t5";
    if (ri != reg_map_.end() && !ri->second.empty()) int_dest = ri->second;

    switch (inst.GetOp()) {
      case IR::IRBinaryOp::ADD: emitter_.EmitInst("fadd.s", dest_reg, dest_reg, rhs_reg); break;
      case IR::IRBinaryOp::SUB: emitter_.EmitInst("fsub.s", dest_reg, dest_reg, rhs_reg); break;
      case IR::IRBinaryOp::MUL: emitter_.EmitInst("fmul.s", dest_reg, dest_reg, rhs_reg); break;
      case IR::IRBinaryOp::DIV: emitter_.EmitInst("fdiv.s", dest_reg, dest_reg, rhs_reg); break;
      case IR::IRBinaryOp::EQ:  emitter_.EmitInst("feq.s",  int_dest, dest_reg, rhs_reg); break;
      case IR::IRBinaryOp::NE:
        emitter_.EmitInst("feq.s", int_dest, dest_reg, rhs_reg);
        emitter_.EmitInst("xori",  int_dest, int_dest, "1"); break;
      case IR::IRBinaryOp::LT:  emitter_.EmitInst("flt.s", int_dest, dest_reg, rhs_reg); break;
      case IR::IRBinaryOp::LE:  emitter_.EmitInst("fle.s", int_dest, dest_reg, rhs_reg); break;
      case IR::IRBinaryOp::GT:  emitter_.EmitInst("flt.s", int_dest, rhs_reg,  dest_reg); break;
      case IR::IRBinaryOp::GE:  emitter_.EmitInst("fle.s", int_dest, rhs_reg,  dest_reg); break;
      default: assert(false && "Unsupported float binary op");
    }
    if (inst.GetType() == IR::IRType::IR_FLOAT && spilled) StoreFRegToVal(&inst, dest_reg);
    else if (inst.GetType() != IR::IRType::IR_FLOAT) {
      if (ri == reg_map_.end() || ri->second.empty()) StoreRegToVal(&inst, int_dest);
    }
    return;
  }

  if (TryEmitMulStrengthReduction(inst)) return;

  std::string name = inst.GetOperandName();
  auto ri = reg_map_.find(name);
  bool spilled = true;
  std::string dest_reg = "t5";
  if (ri != reg_map_.end() && !ri->second.empty() && ri->second[0] != 'f') {
    dest_reg = ri->second; spilled = false;
  }

  std::string rhs_reg = "t6";
  auto rri = reg_map_.find(inst.GetRhs()->GetOperandName());
  if (rri != reg_map_.end() && !rri->second.empty()) rhs_reg = rri->second;
  if (rhs_reg == dest_reg) rhs_reg = "t6";

  LoadValueToReg(inst.GetRhs().get(), rhs_reg);
  LoadValueToReg(inst.GetLhs().get(), dest_reg);

  const char* add_op = target_.Is64Bit() ? "addw" : "add";
  const char* sub_op = target_.Is64Bit() ? "subw" : "sub";
  const char* mul_op = target_.Is64Bit() ? "mulw" : "mul";
  const char* div_op = target_.Is64Bit() ? "divw" : "div";
  const char* rem_op = target_.Is64Bit() ? "remw" : "rem";

  switch (inst.GetOp()) {
    case IR::IRBinaryOp::ADD: emitter_.EmitInst(add_op, dest_reg, dest_reg, rhs_reg); break;
    case IR::IRBinaryOp::SUB: emitter_.EmitInst(sub_op, dest_reg, dest_reg, rhs_reg); break;
    case IR::IRBinaryOp::MUL: emitter_.EmitInst(mul_op, dest_reg, dest_reg, rhs_reg); break;
    case IR::IRBinaryOp::DIV: emitter_.EmitInst(div_op, dest_reg, dest_reg, rhs_reg); break;
    case IR::IRBinaryOp::MOD: emitter_.EmitInst(rem_op, dest_reg, dest_reg, rhs_reg); break;
    case IR::IRBinaryOp::EQ:
      emitter_.EmitInst("sub",  dest_reg, dest_reg, rhs_reg);
      emitter_.EmitInst("seqz", dest_reg, dest_reg); break;
    case IR::IRBinaryOp::NE:
      emitter_.EmitInst("sub",  dest_reg, dest_reg, rhs_reg);
      emitter_.EmitInst("snez", dest_reg, dest_reg); break;
    case IR::IRBinaryOp::LT: emitter_.EmitInst("slt", dest_reg, dest_reg, rhs_reg); break;
    case IR::IRBinaryOp::GT: emitter_.EmitInst("slt", dest_reg, rhs_reg,  dest_reg); break;
    case IR::IRBinaryOp::LE:
      emitter_.EmitInst("slt",  dest_reg, rhs_reg, dest_reg);
      emitter_.EmitInst("xori", dest_reg, dest_reg, "1"); break;
    case IR::IRBinaryOp::GE:
      emitter_.EmitInst("slt",  dest_reg, dest_reg, rhs_reg);
      emitter_.EmitInst("xori", dest_reg, dest_reg, "1"); break;
    case IR::IRBinaryOp::AND: emitter_.EmitInst("and", dest_reg, dest_reg, rhs_reg); break;
    case IR::IRBinaryOp::OR:  emitter_.EmitInst("or",  dest_reg, dest_reg, rhs_reg); break;
    case IR::IRBinaryOp::XOR: emitter_.EmitInst("xor", dest_reg, dest_reg, rhs_reg); break;
    default: assert(false && "Unsupported binary op");
  }
  if (spilled) StoreRegToVal(&inst, dest_reg);
}

// Cast / Alloc / Store / Load / Jump / Branch / Call / GEP / GetPtr
void Back::RiscvGenerator::Visit(const IR::IRCast& inst) {
  if (inst.GetOp() == IR::IRCastOp::INT_TO_FLOAT) {
    LoadValueToReg(inst.GetValue().get(), "t5");
    emitter_.EmitInst("fcvt.s.w", "ft5", "t5");
    StoreFRegToVal(&inst, "ft5");
  } else {
    LoadValueToFReg(inst.GetValue().get(), "ft5");
    emitter_.EmitInst("fcvt.w.s", "t5", "ft5", "rtz");
    StoreRegToVal(&inst, "t5");
  }
}

void Back::RiscvGenerator::Visit(const IR::IRAlloc& /*inst*/) {}

void Back::RiscvGenerator::Visit(const IR::IRStore& inst) {
  const auto* ptr_raw  = inst.GetPtr().get();
  std::string ptr_name = ptr_raw->GetOperandName();

  if (local_allocs_.count(ptr_name)) {
    auto ri = reg_map_.find(ptr_name);
    if (ri != reg_map_.end() && !ri->second.empty()) {
      if (ri->second[0] == 'f') LoadValueToFReg(inst.GetVal().get(), ri->second);
      else                       LoadValueToReg (inst.GetVal().get(), ri->second);
      return;
    }
  }

  if (inst.GetVal()->GetType() == IR::IRType::IR_FLOAT) {
    LoadValueToFReg(inst.GetVal().get(), "ft5");
    if (dynamic_cast<const IR::IRGlobalAlloc*>(ptr_raw)) {
      emitter_.EmitInst("la", "t6", StripAt(ptr_name));
      emitter_.Out() << "  fsw ft5, 0(t6)\n";
    } else if (local_allocs_.count(ptr_name)) {
      emitter_.EmitFsw("ft5", val_stack_map_.at(ptr_name));
    } else {
      LoadValueToReg(inst.GetPtr().get(), "t6");
      emitter_.Out() << "  fsw ft5, 0(t6)\n";
    }
    return;
  }

  LoadValueToReg(inst.GetVal().get(), "t5");
  if (dynamic_cast<const IR::IRGlobalAlloc*>(ptr_raw)) {
    emitter_.EmitInst("la", "t6", StripAt(ptr_name));
    if (GetTypeSize(inst.GetVal()->GetType()) == 8) emitter_.Out() << "  sd t5, 0(t6)\n";
    else                                            emitter_.Out() << "  sw t5, 0(t6)\n";
  } else if (local_allocs_.count(ptr_name)) {
    int off = val_stack_map_.at(ptr_name);
    if (GetTypeSize(inst.GetVal()->GetType()) == 8) emitter_.EmitSd("t5", off);
    else                                            emitter_.EmitSw("t5", off);
  } else {
    LoadValueToReg(inst.GetPtr().get(), "t6");
    if (GetTypeSize(inst.GetVal()->GetType()) == 8) emitter_.Out() << "  sd t5, 0(t6)\n";
    else                                            emitter_.Out() << "  sw t5, 0(t6)\n";
  }
}

void Back::RiscvGenerator::Visit(const IR::IRLoad& inst) {
  const auto* ptr_raw  = inst.GetPtr().get();
  std::string ptr_name = ptr_raw->GetOperandName();

  if (local_allocs_.count(ptr_name)) {
    auto ri = reg_map_.find(ptr_name);
    if (ri != reg_map_.end() && !ri->second.empty()) {
      if (ri->second[0] == 'f') StoreFRegToVal(&inst, ri->second);
      else                       StoreRegToVal (&inst, ri->second);
      return;
    }
  }

  if (inst.GetType() == IR::IRType::IR_FLOAT) {
    if (dynamic_cast<const IR::IRGlobalAlloc*>(ptr_raw)) {
      emitter_.EmitInst("la", "t5", StripAt(ptr_name));
      emitter_.Out() << "  flw ft5, 0(t5)\n";
    } else if (local_allocs_.count(ptr_name)) {
      emitter_.EmitFlw("ft5", val_stack_map_.at(ptr_name));
    } else {
      LoadValueToReg(inst.GetPtr().get(), "t6");
      emitter_.Out() << "  flw ft5, 0(t6)\n";
    }
    StoreFRegToVal(&inst, "ft5");
    return;
  }

  if (dynamic_cast<const IR::IRGlobalAlloc*>(ptr_raw)) {
    emitter_.EmitInst("la", "t5", StripAt(ptr_name));
    if (GetTypeSize(inst.GetType()) == 8) emitter_.Out() << "  ld t5, 0(t5)\n";
    else                                  emitter_.Out() << "  lw t5, 0(t5)\n";
  } else if (local_allocs_.count(ptr_name)) {
    int off = val_stack_map_.at(ptr_name);
    if (GetTypeSize(inst.GetType()) == 8) emitter_.EmitLd("t5", off);
    else                                  emitter_.EmitLw("t5", off);
  } else {
    LoadValueToReg(inst.GetPtr().get(), "t6");
    if (GetTypeSize(inst.GetType()) == 8) emitter_.Out() << "  ld t5, 0(t6)\n";
    else                                  emitter_.Out() << "  lw t5, 0(t6)\n";
  }
  StoreRegToVal(&inst, "t5");
}

void Back::RiscvGenerator::Visit(const IR::IRJump& inst) {
  emitter_.EmitInst("j", StripPercent(inst.GetTarget()->GetOperandName()));
}

void Back::RiscvGenerator::Visit(const IR::IRBranch& inst) {
  LoadValueToReg(inst.GetCond().get(), "t5");
  emitter_.EmitInst("bnez", "t5", StripPercent(inst.GetThenTarget()->GetOperandName()));
  emitter_.EmitInst("j",           StripPercent(inst.GetElseTarget()->GetOperandName()));
}

void Back::RiscvGenerator::Visit(const IR::IRCall& inst) {
  const auto& args = inst.GetArgs();
  std::vector<IR::IRType> arg_types;
  for (const auto& a : args) arg_types.push_back(a->GetType());
  const auto locs = calling_convention_.AssignArgumentLocations(arg_types);
  int arg_buf = param_overflow_size_;

  for (size_t i = 0; i < args.size(); ++i) {
    const auto& loc = locs[i];
    if (loc.kind == ArgumentLocation::Kind::FLOAT_REGISTER) {
      LoadValueToFReg(args[i].get(), "ft5");
      emitter_.EmitFsw("ft5", arg_buf + (int)i * 8);
    } else if (loc.kind == ArgumentLocation::Kind::INTEGER_REGISTER) {
      LoadValueToReg(args[i].get(), "t5");
      if (GetTypeSize(args[i]->GetType()) == 8) emitter_.EmitSd("t5", arg_buf + (int)i * 8);
      else                                      emitter_.EmitSw("t5", arg_buf + (int)i * 8);
    } else {
      int param_off = GetRegSize() * loc.index;
      if (args[i]->GetType() == IR::IRType::IR_FLOAT) {
        LoadValueToFReg(args[i].get(), "ft5");
        emitter_.EmitFsw("ft5", param_off);
      } else if (GetTypeSize(args[i]->GetType()) == 8) {
        LoadValueToReg(args[i].get(), "t5");
        emitter_.EmitSd("t5", param_off);
      } else {
        LoadValueToReg(args[i].get(), "t5");
        emitter_.EmitSw("t5", param_off);
      }
    }
  }

  for (size_t i = 0; i < args.size(); ++i) {
    const auto& loc = locs[i];
    if (loc.kind == ArgumentLocation::Kind::FLOAT_REGISTER)
      emitter_.EmitFlw("fa" + std::to_string(loc.index), arg_buf + (int)i * 8);
    else if (loc.kind == ArgumentLocation::Kind::INTEGER_REGISTER) {
      if (GetTypeSize(args[i]->GetType()) == 8)
        emitter_.EmitLd("a" + std::to_string(loc.index), arg_buf + (int)i * 8);
      else
        emitter_.EmitLw("a" + std::to_string(loc.index), arg_buf + (int)i * 8);
    }
  }

  emitter_.EmitInst("call", StripAt(inst.GetFunc()->GetOperandName()));

  if (inst.HasReturnValue()) {
    if (inst.GetType() == IR::IRType::IR_FLOAT) StoreFRegToVal(&inst, "fa0");
    else                                         StoreRegToVal (&inst, "a0");
  }
}

void Back::RiscvGenerator::EmitIndexScale(int elem_size) {
  if (elem_size == 1) return;
  unsigned shift = 0;
  if (opt_level_ == OptimizationLevel::O1 && elem_size > 0 &&
      GetPowerOfTwoShift(static_cast<uint64_t>(elem_size), shift))
    emitter_.EmitInst("slli", "t6", "t6", std::to_string(shift));
  else {
    emitter_.EmitInst("li",  "t4", std::to_string(elem_size));
    emitter_.EmitInst("mul", "t6", "t6", "t4");
  }
}

void Back::RiscvGenerator::Visit(const IR::IRGetElemPtr& inst) {
  LoadAddressToReg(inst.GetArr().get(),   "t5");
  LoadValueToReg  (inst.GetIndex().get(), "t6");
  EmitIndexScale(inst.GetElemSize());
  emitter_.EmitInst("add", "t5", "t5", "t6");
  StoreRegToVal(&inst, "t5");
}

void Back::RiscvGenerator::Visit(const IR::IRGetPtr& inst) {
  LoadValueToReg(inst.GetPtr().get(),   "t5");
  LoadValueToReg(inst.GetIndex().get(), "t6");
  EmitIndexScale(inst.GetElemSize());
  emitter_.EmitInst("add", "t5", "t5", "t6");
  StoreRegToVal(&inst, "t5");
}

// Value load/store helpers
void Back::RiscvGenerator::LoadValueToReg(const IR::IRValue* val, const std::string& reg) {
  if (const auto* c = dynamic_cast<const IR::IRConstant*>(val)) {
    if (c->GetType() == IR::IRType::IR_INT32) {
      int32_t iv = std::get<int32_t>(c->GetValue());
      if (iv == 0) emitter_.EmitInst("mv", reg, "x0");
      else         emitter_.EmitInst("li", reg, std::to_string(iv));
    } else {
      union { float f; uint32_t u; } conv; conv.f = std::get<float>(c->GetValue());
      if (!reg.empty() && reg[0] == 'f') {
        emitter_.EmitInst("li",      "t5", std::to_string(conv.u));
        emitter_.EmitInst("fmv.w.x", reg,  "t5");
      } else {
        emitter_.EmitInst("li", reg, std::to_string(conv.u));
      }
    }
    return;
  }
  if (dynamic_cast<const IR::IRGlobalAlloc*>(val)) {
    emitter_.EmitInst("la", reg, StripAt(val->GetOperandName()));
    if (GetTypeSize(val->GetType()) == 8) emitter_.Out() << "  ld " << reg << ", 0(" << reg << ")\n";
    else                                  emitter_.Out() << "  lw " << reg << ", 0(" << reg << ")\n";
    return;
  }
  std::string name = val->GetOperandName();
  auto ri = reg_map_.find(name);
  if (ri != reg_map_.end() && !ri->second.empty()) {
    if (reg != ri->second) {
      bool dr = (!reg.empty() && reg[0] == 'f'), sr = (ri->second[0] == 'f');
      if (dr && sr)       emitter_.EmitInst("fmv.s",   reg, ri->second);
      else if (dr && !sr) emitter_.EmitInst("fmv.w.x", reg, ri->second);
      else if (!dr && sr) emitter_.EmitInst("fmv.x.w", reg, ri->second);
      else                emitter_.EmitInst("mv",       reg, ri->second);
    }
  } else {
    auto it = val_stack_map_.find(name);
    assert(it != val_stack_map_.end());
    if (GetTypeSize(val->GetType()) == 8) emitter_.EmitLd(reg, it->second);
    else                                  emitter_.EmitLw(reg, it->second);
  }
}

void Back::RiscvGenerator::StoreRegToVal(const IR::IRValue* val, const std::string& reg) {
  std::string name = val->GetOperandName();
  auto ri = reg_map_.find(name);
  if (ri != reg_map_.end() && !ri->second.empty()) {
    if (reg != ri->second) {
      bool dr = (ri->second[0] == 'f'), sr = (!reg.empty() && reg[0] == 'f');
      if (dr && sr)       emitter_.EmitInst("fmv.s",   ri->second, reg);
      else if (dr && !sr) emitter_.EmitInst("fmv.w.x", ri->second, reg);
      else if (!dr && sr) emitter_.EmitInst("fmv.x.w", ri->second, reg);
      else                emitter_.EmitInst("mv",       ri->second, reg);
    }
  } else {
    auto it = val_stack_map_.find(name);
    assert(it != val_stack_map_.end());
    if (GetTypeSize(val->GetType()) == 8) emitter_.EmitSd(reg, it->second);
    else                                  emitter_.EmitSw(reg, it->second);
  }
}

void Back::RiscvGenerator::LoadValueToFReg(const IR::IRValue* val, const std::string& reg) {
  if (const auto* c = dynamic_cast<const IR::IRConstant*>(val)) {
    if (c->GetType() == IR::IRType::IR_FLOAT) {
      float fv = std::get<float>(c->GetValue());
      uint32_t bits = 0; std::memcpy(&bits, &fv, sizeof(bits));
      emitter_.EmitInst("li",      "t5", std::to_string(bits));
      emitter_.EmitInst("fmv.w.x", reg,  "t5");
    } else {
      LoadValueToReg(val, "t5");
      emitter_.EmitInst("fcvt.s.w", reg, "t5");
    }
    return;
  }
  std::string name = val->GetOperandName();
  auto ri = reg_map_.find(name);
  if (ri != reg_map_.end() && !ri->second.empty()) {
    if (reg != ri->second)
      emitter_.EmitInst(ri->second[0] == 'f' ? "fmv.s" : "fmv.w.x", reg, ri->second);
  } else {
    auto it = val_stack_map_.find(name);
    assert(it != val_stack_map_.end());
    emitter_.EmitFlw(reg, it->second);
  }
}

void Back::RiscvGenerator::StoreFRegToVal(const IR::IRValue* val, const std::string& reg) {
  std::string name = val->GetOperandName();
  auto ri = reg_map_.find(name);
  if (ri != reg_map_.end() && !ri->second.empty()) {
    if (reg != ri->second)
      emitter_.EmitInst(ri->second[0] == 'f' ? "fmv.s" : "fmv.x.w", ri->second, reg);
  } else {
    auto it = val_stack_map_.find(name);
    assert(it != val_stack_map_.end());
    emitter_.EmitFsw(reg, it->second);
  }
}

void Back::RiscvGenerator::LoadAddressToReg(const IR::IRValue* val, const std::string& reg) {
  if (const auto* al = dynamic_cast<const IR::IRAlloc*>(val)) {
    auto it = val_stack_map_.find(al->GetName());
    assert(it != val_stack_map_.end());
    int off = it->second;
    if (off >= -2048 && off <= 2047) emitter_.EmitInst("addi", reg, "sp", std::to_string(off));
    else {
      emitter_.EmitInst("li",  reg, std::to_string(off));
      emitter_.EmitInst("add", reg, "sp", reg);
    }
  } else if (dynamic_cast<const IR::IRGlobalAlloc*>(val)) {
    emitter_.EmitInst("la", reg, StripAt(val->GetOperandName()));
  } else {
    std::string name = val->GetOperandName();
    auto ri = reg_map_.find(name);
    if (ri != reg_map_.end() && !ri->second.empty()) {
      if (reg != ri->second) emitter_.EmitInst("mv", reg, ri->second);
    } else {
      auto it = val_stack_map_.find(name);
      assert(it != val_stack_map_.end());
      if (target_.Is64Bit()) emitter_.EmitLd(reg, it->second);
      else           emitter_.EmitLw(reg, it->second);
    }
  }
}

void Back::RiscvGenerator::StoreRegToValByName(const std::string& name,
                                               const std::string& reg,
                                               IR::IRType type) {
  auto it = val_stack_map_.find(name);
  assert(it != val_stack_map_.end());
  if (!reg.empty() && reg[0] == 'f')  emitter_.EmitFsw(reg, it->second);
  else if (GetTypeSize(type) == 8)    emitter_.EmitSd (reg, it->second);
  else                                emitter_.EmitSw (reg, it->second);
}

void Back::RiscvGenerator::LoadValueToRegByName(const std::string& name,
                                                const std::string& reg,
                                                IR::IRType type) {
  auto it = val_stack_map_.find(name);
  assert(it != val_stack_map_.end());
  if (!reg.empty() && reg[0] == 'f')  emitter_.EmitFlw(reg, it->second);
  else if (GetTypeSize(type) == 8)    emitter_.EmitLd (reg, it->second);
  else                                emitter_.EmitLw (reg, it->second);
}
