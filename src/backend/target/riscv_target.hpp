#pragma once
namespace Back {

struct TargetConfig {
  bool is_64bit    = true;
  int  word_size   = 4;
  int  pointer_size = 8;
  int  stack_align  = 16;
};

class RiscvTarget {
public:
  explicit RiscvTarget(const TargetConfig& cfg) : cfg_(cfg) {}

  int GetRegSize()    const { return cfg_.is_64bit ? 8 : 4; }
  int GetWordSize()   const { return cfg_.word_size; }
  int GetPointerSize()const { return cfg_.pointer_size; }
  int GetStackAlign() const { return cfg_.stack_align; }
  bool Is64Bit()      const { return cfg_.is_64bit; }

private:
  TargetConfig cfg_;
};

}
