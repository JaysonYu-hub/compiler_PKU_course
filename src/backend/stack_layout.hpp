#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace Back {

struct StackLayout {
  int total_size = 0;
  int param_overflow_size = 0;
  int arg_buffer_size = 0;
  int ra_save_size = 0;
  int s_reg_save_size = 0;
  int spill_size = 0;
  int local_alloc_size = 0;

  std::unordered_map<std::string, int> value_offsets;
};

class StackLayoutBuilder {
public:
  StackLayout Build(const std::vector<std::string>& used_s_regs,
                    const std::unordered_map<std::string, int>& spill_offsets,
                    int max_stack_arg_count,
                    int max_call_arg_count,
                    bool has_call,
                    int local_alloc_size,
                    int reg_size,
                    int stack_align) {
    StackLayout layout;

    layout.spill_size = 0;
    for (const auto& kv : spill_offsets) {
      if (kv.second + reg_size > layout.spill_size) {
        layout.spill_size = kv.second + reg_size;
      }
    }

    // 1. Parameter overflow size
    layout.param_overflow_size = max_stack_arg_count * reg_size;

    // 2. Parallel-copy staging buffer
    // 函数调用实参和 SSA 块参数共同使用这块空间。即使函数本身没有 call，
    // 循环回边上的 stack/stack copy 也可能需要先保存被覆盖的旧值，因此
    // 不能再用 has_call 作为是否分配缓冲区的条件。
    layout.arg_buffer_size =
        max_call_arg_count > 0 ? max_call_arg_count * reg_size : 0;

    // 3. Saved RA size
    layout.ra_save_size = has_call ? reg_size : 0;

    // 4. Saved S registers size
    layout.s_reg_save_size = used_s_regs.size() * reg_size;

    int total = layout.param_overflow_size + layout.arg_buffer_size +
                local_alloc_size + layout.s_reg_save_size +
                layout.spill_size + layout.ra_save_size;

    // 5. Total stack frame size (aligned)
    layout.total_size = AlignUp(total, stack_align);

    return layout;
  }

private:
  int AlignUp(int value, int align) const {
    return (value + align - 1) / align * align;
  }
};

}
