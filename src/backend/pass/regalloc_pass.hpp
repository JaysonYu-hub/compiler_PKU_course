#pragma once
// RegAllocPass: thin wrapper around Back::LinearScanAllocator.
// Delegates actual register allocation to the allocator; the generator
// calls Allocate() per-function, not here, because per-function interval
// data is needed. This pass exists to satisfy the PassManager interface
// and can be extended with a full IR-level scan later.

#include <memory>
#include "pass_manager.hpp"
#include "../regalloc/linear_scan_allocator.hpp"

namespace Back {

class RegAllocPass : public Pass {
public:
  // Pass takes ownership of the allocator (per-program placeholder).
  // In practice, RiscvGenerator::Visit(IRFunction) creates a per-function
  // LinearScanAllocator with the actual live intervals.
  RegAllocPass() = default;

  void Run(IR::IRProgram& /*program*/) override {
    // Per-function allocation happens inside RiscvGenerator::Visit(IRFunction).
    // This pass is a no-op at the program level.
  }
};

}