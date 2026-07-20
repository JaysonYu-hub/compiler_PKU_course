#pragma once

#include <memory>
#include <vector>
#include "../../IR/custom_ir.hpp"

namespace Back {

class Pass {
public:
  virtual ~Pass() = default;
  virtual void Run(IR::IRProgram& program) = 0;
};

class PassManager {
public:
  void AddPass(std::unique_ptr<Pass> pass);
  void Run(IR::IRProgram& program);

private:
  std::vector<std::unique_ptr<Pass>> passes_;
};

}