#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <array>

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
  };

  const std::vector<std::string> reg_pool = {
    "t0", "t1", "t2", "t3",
    "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"
  };

  int reg_size;

  LinearScanAllocator(const std::unordered_map<std::string, std::array<long long int, 2>>& intervals, int reg_size = 8);

  void Allocate();

  std::unordered_map<std::string, std::string> GetRegMap() const;

  std::vector<LiveInterval> GetSpilledIntervals() const;

private:
  std::vector<LiveInterval> intervals_list;

  bool is_virtual_register(const std::string& var) const;
};

}
