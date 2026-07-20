#pragma once
// Calling convention for RISC-V (RV32/RV64 ILAP).
// Argument classification: first 8 int args → a0-a7,
//                          first 8 float args → fa0-fa7,
//                          overflow → stack (index is slot index from sp).

#include <vector>
#include "../../IR/custom_ir.hpp"

namespace Back {

struct ArgumentLocation {
  enum class Kind { INTEGER_REGISTER, FLOAT_REGISTER, STACK } kind;
  int index = 0;
};

class CallingConvention {
public:
  explicit CallingConvention(bool is_64bit) : is_64bit_(is_64bit) {}

  // Assigns each argument to a physical location following the RISC-V ILAP ABI.
  std::vector<ArgumentLocation> AssignArgumentLocations(
      const std::vector<IR::IRType>& types) const;

  // Size in bytes of one stack argument slot.
  int GetStackArgSlotSize() const { return is_64bit_ ? 8 : 4; }

private:
  bool is_64bit_;
};

}
