#pragma once
// LivenessPass: thin wrapper around IR::Pass::LiveVarAnalysis.
// Runs liveness analysis on the whole program so downstream passes
// (RegAllocPass, call-site save/restore) can query live-variable sets.

#include <memory>
#include "pass_manager.hpp"
#include "../../IR/passes/liveness.hpp"

namespace Back {

class LivenessPass : public Pass {
public:
  explicit LivenessPass(std::unique_ptr<IR::Pass::LiveVarAnalysis> analysis)
      : analysis_(std::move(analysis)) {}

  void Run(IR::IRProgram& program) override {
    if (analysis_) analysis_->Analyze(&program);
  }

  // Expose the analysis result to sibling passes.
  IR::Pass::LiveVarAnalysis* GetAnalysis() const { return analysis_.get(); }

private:
  std::unique_ptr<IR::Pass::LiveVarAnalysis> analysis_;
};

}