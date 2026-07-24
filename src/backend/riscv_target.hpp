#pragma once
namespace Back {

enum class OptimizationLevel { O0, O1 };

struct TargetConfig {
  bool is_64bit    = true;
  int  word_size   = 4;
  int  pointer_size = 8;
  int  stack_align  = 16;
  OptimizationLevel optimization_level = OptimizationLevel::O0;

  void Set64Bit(bool enabled) {
    is_64bit = enabled;
    pointer_size = enabled ? 8 : 4;
    stack_align = enabled ? 16 : 8;
  }
};

class RiscvTarget {
public:
  explicit RiscvTarget(const TargetConfig& cfg) : cfg_(cfg) {}

  int GetRegSize()    const { return cfg_.is_64bit ? 8 : 4; }
  int GetWordSize()   const { return cfg_.word_size; }
  int GetPointerSize()const { return cfg_.pointer_size; }
  int GetStackAlign() const { return cfg_.stack_align; }
  bool Is64Bit()      const { return cfg_.is_64bit; }
  OptimizationLevel GetOptimizationLevel() const {
    return cfg_.optimization_level;
  }

private:
  TargetConfig cfg_;
};

}
