#pragma once

#include "../custom_ir.hpp"
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace IR {

class IRUseDefAnalysis {
public:
  void Rebuild(const IRProgram& program);
  std::size_t GetUseCount(const IRValue* value) const;
  const std::vector<const IRInstruction*>& GetUsers(
      const IRValue* value) const;

private:
  std::unordered_map<const IRValue*, std::vector<const IRInstruction*>> users_;
};

std::size_t ReplaceAllUses(IRProgram& program, const IRValue* old_value,
                           std::shared_ptr<IRValue> new_value);

class IRVerifier {
public:
  static std::vector<std::string> Verify(const IRProgram& program);
  static void VerifyOrThrow(const IRProgram& program);
};

struct IROptimizationStats {
  std::size_t constants_folded = 0;
  std::size_t algebraic_simplifications = 0;
  std::size_t dead_instructions_removed = 0;
  std::size_t constant_branches_simplified = 0;
  std::size_t unreachable_blocks_removed = 0;
};

IROptimizationStats RunO1Passes(IRProgram& program);

}

