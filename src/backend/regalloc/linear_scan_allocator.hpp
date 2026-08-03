#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <cstdint>

namespace Back {

class LinearScanAllocator {
public:
  struct LiveInterval {
    std::string var;
    long long int start;
    long long int end;
    std::string reg;
    bool spilled = false;
    int stack_offset = 0;
    bool crosses_call = false;
    bool is_float = false;
    std::uint64_t use_weight = 1;
    std::uint64_t use_count = 0;
  };

  const std::vector<std::string> reg_pool = {
    "t0", "t1", "t2", "t3",
    "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"
  };
  const std::vector<std::string> float_pool = {
    "ft0", "ft1", "ft2", "ft3",
    "fs1", "fs2", "fs3", "fs4", "fs5", "fs6", "fs7", "fs8", "fs9", "fs10", "fs11",
    "fa0", "fa1", "fa2", "fa3", "fa4", "fa5", "fa6", "fa7"
  };
  int reg_size;

  LinearScanAllocator(const std::unordered_map<std::string, 
                      std::array<long long int, 2>>& intervals, 
                      int reg_size = 8,
                      const std::unordered_set<std::string>& crosses_calls = {},
                      const std::unordered_set<std::string>& float_values = {},
                      const std::unordered_map<std::string, std::uint64_t>& use_weights = {},
                      const std::unordered_map<std::string, std::uint64_t>& use_counts = {}
                     );

  void Allocate();

  std::unordered_map<std::string, std::string> GetRegMap() const;

  std::vector<LiveInterval> GetSpilledIntervals() const;

private:
  std::vector<LiveInterval> intervals_list;

  bool is_virtual_register(const std::string& var) const;
  static bool IsCalleeSaved(const std::string& reg);
  static long double SpillPriority(const LiveInterval& interval);
  static bool LowerSpillPriority(const LiveInterval& lhs,
                                 const LiveInterval& rhs);
  void AssignSpillSlots();
};

}
