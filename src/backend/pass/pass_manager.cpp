#include "pass_manager.hpp"

namespace Back {

void PassManager::AddPass(std::unique_ptr<Pass> pass) {
  passes_.push_back(std::move(pass));
}

void PassManager::Run(IR::IRProgram& program) {
  for (auto& pass : passes_) {
    pass->Run(program);
  }
}

}