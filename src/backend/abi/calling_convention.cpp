#include "calling_convention.hpp"

namespace Back {
// 用于获取函数参数位置
std::vector<ArgumentLocation> CallingConvention::AssignArgumentLocations(
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

}  // namespace Back
