#include "linear_scan_allocator.hpp"
#include <algorithm>

namespace Back {

LinearScanAllocator::LinearScanAllocator(const std::unordered_map<std::string, std::array<long long int, 2>>& intervals, 
                                         int reg_size,
                                         const std::unordered_set<std::string>& crosses_calls,
                                         const std::unordered_set<std::string>& float_values) 
: reg_size(reg_size) {
  for (const auto& kv : intervals) {
    // 过滤物理寄存器
    if (is_virtual_register(kv.first)) {
      intervals_list.push_back({kv.first, kv.second[0], kv.second[1], "", false, 0,
                                crosses_calls.count(kv.first) != 0, float_values.count(kv.first) != 0});
    }
  }

  std::sort(intervals_list.begin(), intervals_list.end(), [](const LiveInterval& a, const LiveInterval& b) {
    if (a.start != b.start) return a.start < b.start;
    if (a.end != b.end) return a.end < b.end;
    return a.var < b.var;
  });
}

void LinearScanAllocator::Allocate() {
  int offset = 0;
  
  auto allocate_class = [&](bool want_float, const std::vector<std::string>& pool) {
    std::vector<std::string> free_regs = pool;
    std::vector<LiveInterval*> active_regs = {};
    for (auto& current : intervals_list) {
      if (current.is_float != want_float) continue;
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
        
        auto preferred = std::find_if(free_regs.rbegin(), free_regs.rend(),
        [&](const std::string& reg) {
          if (want_float) return true;
          const bool callee_saved = !reg.empty() && reg[0] == 's';
          return callee_saved == current.crosses_call;
        });
        if (preferred == free_regs.rend()) {
          current.reg = free_regs.back();
          free_regs.pop_back();
        } else {
          current.reg = *preferred;
          free_regs.erase(std::next(preferred).base());
        }
        active_regs.push_back(&current);
        std::sort(active_regs.begin(), active_regs.end(), [](const LiveInterval* a, const LiveInterval* b) {
          return a->end < b->end;
        });
      } else {
        // 选择最晚结束的区间溢出
        if (!active_regs.empty() && active_regs.back()->end > current.end) {
          LiveInterval* spill_candidate = active_regs.back();
          spill_candidate->spilled = true;
          spill_candidate->stack_offset = offset;
          offset += reg_size;

          active_regs.pop_back();
          active_regs.push_back(&current);
          current.reg = spill_candidate->reg;
          std::sort(active_regs.begin(), active_regs.end(), [](const LiveInterval* a, const LiveInterval* b) {
            if (a->end != b->end) return a->end < b->end;
            return a->var < b->var;
          });
        } else { 
          current.spilled = true;
          current.stack_offset = offset;
          offset += reg_size;
        }
      }
    }
  };
  allocate_class(false, reg_pool);
  allocate_class(true, float_pool);
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