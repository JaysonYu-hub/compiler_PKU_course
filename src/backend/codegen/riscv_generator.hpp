#pragma once
// RiscvGenerator: thin orchestrator for the RISC-V backend.
// Drives the code-generation pipeline:
//
//   IRProgram
//     ↓  Mem2RegPass    (mem2reg)
//     ↓  LivenessPass   (liveness analysis)
//     ↓  RegAllocPass   (per-function linear-scan register allocation)
//     ↓  StackLayout    (per-function stack-frame computation)
//     ↓  Visit/Emit     (instruction-level lowering + AsmEmitter)
//     →  RISC-V assembly on stdout

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cassert>
#include <iostream>

#include "../../IR/custom_ir.hpp"
#include "../riscv_target.hpp"
#include "../calling_convention.hpp"
#include "../stack_layout.hpp"
#include "../asm_emitter.hpp"
#include "../../IR/passes/liveness.hpp"

namespace Back {

class RiscvGenerator {
public:
  explicit RiscvGenerator(const TargetConfig& cfg)
      : target_(cfg),
        calling_convention_(cfg.is_64bit) {}

  // generate assembly for the whole program.
  void Generate(const IR::IRProgram& program);

private:
  // ----- Modular backend objects -----
  RiscvTarget        target_;
  CallingConvention  calling_convention_;
  AsmEmitter         emitter_;

  // Liveness analysis result 
  std::unique_ptr<IR::Pass::LiveVarAnalysis> lva_;

  // ----- Per-function state (reset in Visit(IRFunction)) -----
  std::unordered_map<std::string, std::string> reg_map_;
  std::unordered_map<std::string, int>         spill_offsets_;
  std::unordered_map<std::string, int>         val_stack_map_;
  std::unordered_map<std::string, IR::IRType>  val_type_map_;
  std::unordered_set<std::string>              local_allocs_;
  std::vector<std::string>                     used_s_regs_;
  StackLayout  stack_layout_;
  int  current_stack_size_  = 0;
  int  param_overflow_size_ = 0;
  int  s_reg_start_offset_  = 0;
  bool has_call_            = false;
  int  max_stack_arg_count_ = 0;
  int  max_call_arg_count_  = 0;
  int  max_edge_arg_count_  = 0;
  int  edge_label_counter_  = 0;

  // ----- pipeline -----
  void Visit(const IR::IRProgram&    program);
  void Visit(const IR::IRGlobalAlloc& global);
  void Visit(const IR::IRFunction&   func);
  void Visit(const IR::IRBasicBlock& block);
  void Visit(const IR::IRInstruction& inst);

  void Visit(const IR::IRReturn&     inst);
  void Visit(const IR::IRBinary&     inst);
  void Visit(const IR::IRCast&       inst);
  void Visit(const IR::IRAlloc&      inst);
  void Visit(const IR::IRStore&      inst);
  void Visit(const IR::IRLoad&       inst);
  void Visit(const IR::IRJump&       inst);
  void Visit(const IR::IRBranch&     inst);
  void Visit(const IR::IRCall&       inst);
  void Visit(const IR::IRGetElemPtr& inst);
  void Visit(const IR::IRGetPtr&     inst);
  void Visit(const IR::IRZeroInit&   inst);

  // ----- Helpers -----
  int zero_init_label_counter_ = 0;
  int GetRegSize()  const { return target_.GetRegSize(); }
  OptimizationLevel GetOptimizationLevel() const {
    return target_.GetOptimizationLevel();
  }
  int GetTypeSize(IR::IRType t) const {
    return (t == IR::IRType::IR_POINTER) ? target_.GetRegSize() : 4;
  }
  int GetTypeAlignment(IR::IRType t) const {
    return (t == IR::IRType::IR_POINTER) ? target_.GetRegSize() : 4;
  }
  static int AlignUp(int value, int alignment) {
    assert(alignment > 0);
    return (value + alignment - 1) / alignment * alignment;
  }
  int ReserveStackSlot(int& offset, int size, int alignment) const {
    offset = AlignUp(offset, alignment);
    const int slot = offset;
    offset += size;
    return slot;
  }

  std::string StripAt(const std::string& name);
  std::string StripPercent(const std::string& name);

  void LoadValueToReg  (const IR::IRValue* val, const std::string& reg);
  void StoreRegToVal   (const IR::IRValue* val, const std::string& reg);
  void LoadValueToFReg (const IR::IRValue* val, const std::string& reg);
  void StoreFRegToVal  (const IR::IRValue* val, const std::string& reg);
  void LoadAddressToReg(const IR::IRValue* val, const std::string& reg);
  void EmitEdgeCopies(const std::vector<std::shared_ptr<IR::IRValue>>& args,
                      const IR::IRBasicBlock& target);

  void StoreRegToValByName (const std::string& name, const std::string& reg, IR::IRType t);
  void LoadValueToRegByName(const std::string& name, const std::string& reg, IR::IRType t);

  void EmitIndexScale(int elem_size);
  bool TryEmitMulStrengthReduction(const IR::IRBinary& inst);
};

}
