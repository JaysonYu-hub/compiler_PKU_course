#pragma once
// StackLayoutPass: convenience wrapper around StackLayoutBuilder.
// Computes the full stack frame layout for a function given allocation results.

#include <vector>
#include <unordered_map>
#include <string>
#include "../stack/stack_layout.hpp"

namespace Back {

class StackLayoutPass {
public:
  StackLayout Run(const std::vector<std::string>& used_s_regs,
                  const std::unordered_map<std::string, int>& spill_offsets,
                  int max_stack_arg_count,
                  bool has_call,
                  int local_alloc_size,
                  int reg_size,
                  int stack_align) {
    StackLayoutBuilder builder;
    return builder.Build(used_s_regs, spill_offsets, max_stack_arg_count,
                         has_call, local_alloc_size, reg_size, stack_align);
  }
};

}