#include "linear_scan_allocator.hpp"
#include <algorithm>

namespace Back {

LinearScanAllocator::LinearScanAllocator(const std::unordered_map<std::string, std::array<long long int, 2>>& intervals, int reg_size) 
    : reg_size(reg_size) {
  for (const auto& kv : intervals) {
    if (is_virtual_register(kv.first)) {
      intervals_list.push_back({kv.first, kv.second[0], kv.second[1], "", false, 0});
    }
  }

  std::sort(intervals_list.begin(), intervals_list.end(), [](const LiveInterval& a, const LiveInterval& b) {
    return a.start < b.start;
  });
}

void LinearScanAllocator::Allocate() {
  int offset = 0;
  std::vector<std::string> free_regs = reg_pool;
  std::vector<LiveInterval*> active_regs = {};
  for (auto& current : intervals_list) {
    auto it = active_regs.begin();
    while (it != active_regs.end()) {
      if ((*it)->end <= current.start) {
        free_regs.push_back((*it)->reg);
        it = active_regs.erase(it);
      } else {
        break;
      }
    }

    if (!free_regs.empty()) {
      current.reg = free_regs.back();
      free_regs.pop_back();
      active_regs.push_back(&current);
      std::sort(active_regs.begin(), active_regs.end(), [](const LiveInterval* a, const LiveInterval* b) {
        return a->end < b->end;
      });
    } else {
      if (!active_regs.empty() && active_regs.back()->end > current.end) {
        LiveInterval* spill_candidate = active_regs.back();
        spill_candidate->spilled = true;
        spill_candidate->stack_offset = offset;
        offset += reg_size;

        active_regs.pop_back();
        active_regs.push_back(&current);
        current.reg = spill_candidate->reg;
      } else { 
        current.spilled = true;
        current.stack_offset = offset;
        offset += reg_size;
      }
    }
  }
}

std::unordered_map<std::string, std::string> LinearScanAllocator::GetRegMap() const {
  std::unordered_map<std::string, std::string> reg_map;
  for (const auto& interval : intervals_list) {
    if (!interval.spilled && !interval.reg.empty()) {
      reg_map[interval.var] = interval.reg;
    }
  }
  return reg_map;
}

std::vector<LinearScanAllocator::LiveInterval> LinearScanAllocator::GetSpilledIntervals() const {
  std::vector<LiveInterval> spilled;
  for (const auto& interval : intervals_list) {
    if (interval.spilled) {
      spilled.push_back(interval);
    }
  }
  return spilled;
}

bool LinearScanAllocator::is_virtual_register(const std::string& var) const {
  if (var.empty()) return false;
  
  static const std::unordered_set<std::string> physical_regs = {
    "zero", "ra", "sp", "gp", "tp", 
    "t0", "t1", "t2", "t3", "t4", "t5", "t6",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
    "ft0", "ft1", "ft2", "ft3", "ft4", "ft5", "ft6", "ft7", "ft8", "ft9", "ft10", "ft11",
    "fa0", "fa1", "fa2", "fa3", "fa4", "fa5", "fa6", "fa7"
  };
  return physical_regs.find(var) == physical_regs.end();
}

}