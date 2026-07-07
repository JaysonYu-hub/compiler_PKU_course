#pragma once
#include <string>
#include <unordered_map>
#include <assert.h>
#include <iostream>
#include <unordered_set>
#include "../IR/custom_ir.hpp"

namespace Back {

  class RiscvGenerator {
  public:
    RiscvGenerator(bool is_64bit = true) : is_64bit(is_64bit) {}
    void Generate(const IR::IRProgram& program);
  private:
    bool is_64bit;

    void Visit(const IR::IRProgram& program);
    void Visit(const IR::IRGlobalAlloc& global);
    void Visit(const IR::IRFunction& func);
    void Visit(const IR::IRBasicBlock& block);
    void Visit(const IR::IRInstruction& inst);
    void Visit(const IR::IRReturn& ret_inst);
    void Visit(const IR::IRBinary& inst);
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
    int max_param_count = 0;

    std::unordered_map<std::string, int> val_stack_map;
    std::unordered_set<std::string> local_allocs;

    void LoadValueToReg(const IR::IRValue* val, const std::string& reg);
    void StoreRegToVal(const IR::IRValue* val, const std::string& reg);
    void LoadAddressToReg(const IR::IRValue* val, const std::string& reg);
    void EmitLw(const std::string& reg, int offset, const std::string& base_reg = "sp");
    void EmitSw(const std::string& reg, int offset, const std::string& base_reg = "sp");
    void EmitLd(const std::string& reg, int offset, const std::string& base_reg = "sp");
    void EmitSd(const std::string& reg, int offset, const std::string& base_reg = "sp");
    std::string StripAt(const std::string& name);
    std::string StripPercent(const std::string& name);
  };
};