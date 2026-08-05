#pragma once
// Calling convention for RISC-V (RV32/RV64 psABI).
// Argument classification: first 8 int args → a0-a7,
//                          first 8 float args → fa0-fa7,
//                          overflow → stack (index is slot index from sp).

#include <vector>
#include "../IR/custom_ir.hpp"

namespace Back {

struct ArgumentLocation {
  enum class Kind { INTEGER_REGISTER, FLOAT_REGISTER, STACK } kind;
  int index = 0;
};

class CallingConvention {
public:
  explicit CallingConvention(bool is_64bit) : is_64bit_(is_64bit) {}

  // Assigns each argument to a physical location following the RISC-V psABI.
  std::vector<ArgumentLocation> AssignArgumentLocations(
      const std::vector<IR::IRType>& types) const {
    std::vector<ArgumentLocation> locs;
    locs.reserve(types.size());

    int int_idx   = 0;
    int float_idx = 0;
    int stack_idx = 0;

    for (IR::IRType t : types) {
      ArgumentLocation loc;
      if (t == IR::IRType::IR_FLOAT && float_idx < 8) {
        loc.kind  = ArgumentLocation::Kind::FLOAT_REGISTER;
        loc.index = float_idx++;
      } else if (t != IR::IRType::IR_FLOAT && int_idx < 8) {
        loc.kind  = ArgumentLocation::Kind::INTEGER_REGISTER;
        loc.index = int_idx++;
      } else {
        loc.kind  = ArgumentLocation::Kind::STACK;
        loc.index = stack_idx++;
      }
      locs.push_back(loc);
    }
    return locs;
  }

  // Size in bytes of one stack argument slot.
  int GetStackArgSlotSize() const { return is_64bit_ ? 8 : 4; }

private:
  bool is_64bit_;
};

}
