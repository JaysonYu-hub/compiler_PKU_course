#pragma once
#include <string>
#include <unordered_map>
#include <assert.h>
#include <iostream>
#include <unordered_set>
#include <vector>
#include "../IR/custom_ir.hpp"

// Global architecture flag. Set in main() before code generation.
extern bool g_is_64bit;
inline int RegSize() { return g_is_64bit ? 8 : 4; }

namespace Back {

  enum class OptimizationLevel {
    O0,
    O1
  };

  class RiscvGenerator {
  public:
    RiscvGenerator(OptimizationLevel optimization_level = OptimizationLevel::O0)
      : is_64bit(g_is_64bit), optimization_level(optimization_level) {}
    void Generate(const IR::IRProgram& program);
  private:
    bool is_64bit;
    OptimizationLevel optimization_level;
    std::unordered_map<std::string, std::string> reg_map;
    

    void Visit(const IR::IRProgram& program);
    void Visit(const IR::IRGlobalAlloc& global);
    void Visit(const IR::IRFunction& func);
    void Visit(const IR::IRBasicBlock& block);
    void Visit(const IR::IRInstruction& inst);
    void Visit(const IR::IRReturn& ret_inst);
    void Visit(const IR::IRBinary& inst);
    void Visit(const IR::IRCast& inst);
    void Visit(const IR::IRAlloc& inst);
    void Visit(const IR::IRStore& inst);
    void Visit(const IR::IRLoad& inst);
    void Visit(const IR::IRJump& inst);
    void Visit(const IR::IRBranch& inst);
    void Visit(const IR::IRCall& inst);
    void Visit(const IR::IRGetElemPtr& inst);
    void Visit(const IR::IRGetPtr& inst);

    int current_stack_size = 0;
    bool has_call = false;
    int max_stack_arg_count = 0;

    std::unordered_map<std::string, int> val_stack_map;
    std::unordered_set<std::string> local_allocs;

    void LoadValueToReg(const IR::IRValue* val, const std::string& reg);
    void StoreRegToVal(const IR::IRValue* val, const std::string& reg);
    void LoadValueToFReg(const IR::IRValue* val, const std::string& reg);
    void StoreFRegToVal(const IR::IRValue* val, const std::string& reg);
    void LoadAddressToReg(const IR::IRValue* val, const std::string& reg);
    void EmitLw(const std::string& reg, int offset, const std::string& base_reg = "sp");
    void EmitSw(const std::string& reg, int offset, const std::string& base_reg = "sp");
    void EmitLd(const std::string& reg, int offset, const std::string& base_reg = "sp");
    void EmitSd(const std::string& reg, int offset, const std::string& base_reg = "sp");
    void EmitFlw(const std::string& reg, int offset, const std::string& base_reg = "sp");
    void EmitFsw(const std::string& reg, int offset, const std::string& base_reg = "sp");
    bool TryEmitMulStrengthReduction(const IR::IRBinary& inst);
    void EmitIndexScale(int elem_size);
    std::string StripAt(const std::string& name);
    std::string StripPercent(const std::string& name);
    struct ArgumentLocation {
      enum class Kind { INTEGER_REGISTER, FLOAT_REGISTER, STACK } kind;
      int index;
    };
    std::vector<ArgumentLocation> AssignArgumentLocations(
        const std::vector<IR::IRType>& types) const;
    int GetTypeSize(IR::IRType type) const {
      return (type == IR::IRType::IR_POINTER) ? RegSize() : 4;
    }
    int GetRegisterSize() const {
      return RegSize();
    }
  };
};
