#include "linear_scan_allocator.hpp"

#include <algorithm>

namespace Back {

LinearScanAllocator::LinearScanAllocator(
    const std::unordered_map<std::string, std::array<long long int, 2>>& intervals,
    int reg_size,
    const std::unordered_set<std::string>& crosses_calls,
    const std::unordered_set<std::string>& float_values,
    const std::unordered_map<std::string, std::uint64_t>& use_weights,
    const std::unordered_map<std::string, std::uint64_t>& use_counts,
    const std::unordered_map<std::string, std::string>& copy_hints)
    : reg_size(reg_size), copy_hints_(copy_hints) {
  for (const auto& [name, range] : intervals) {
    if (!is_virtual_register(name) || range[0] > range[1]) continue;
    const auto weight_it = use_weights.find(name);
    const auto count_it = use_counts.find(name);
    intervals_list.push_back({
        name, range[0], range[1], "", false, 0,
        crosses_calls.count(name) != 0,
        float_values.count(name) != 0,
        weight_it == use_weights.end() ? 1 : std::max<std::uint64_t>(1, weight_it->second),
        count_it == use_counts.end() ? 0 : count_it->second});
  }

  std::sort(intervals_list.begin(), intervals_list.end(),
            [](const LiveInterval& lhs, const LiveInterval& rhs) {
    if (lhs.start != rhs.start) return lhs.start < rhs.start;
    if (lhs.end != rhs.end) return lhs.end < rhs.end;
    return lhs.var < rhs.var;
  });
  for (std::size_t i = 0; i < intervals_list.size(); ++i)
    interval_index_[intervals_list[i].var] = i;
}

bool LinearScanAllocator::IsCalleeSaved(const std::string& reg) {
  if (reg.empty()) return false;
  if (reg[0] == 's') return true;
  return reg.size() >= 2 && reg[0] == 'f' && reg[1] == 's';
}

long double LinearScanAllocator::SpillPriority(const LiveInterval& interval) {
  // 权重越大、区间越短，越值得留在寄存器中。跨调用值额外乘 2，
  // 因为把它溢出通常会在调用前后产生一对访存。
  const long long span = std::max<long long>(1, interval.end - interval.start + 1);
  const long double call_factor = interval.crosses_call ? 2.0L : 1.0L;
  return call_factor * static_cast<long double>(interval.use_weight) /
         static_cast<long double>(span);
}

bool LinearScanAllocator::LowerSpillPriority(const LiveInterval& lhs,
                                             const LiveInterval& rhs) {
  const long double lhs_priority = SpillPriority(lhs);
  const long double rhs_priority = SpillPriority(rhs);
  if (lhs_priority != rhs_priority) return lhs_priority < rhs_priority;
  // 热度相同时优先溢出结束更晚的区间，尽快释放寄存器。
  if (lhs.end != rhs.end) return lhs.end > rhs.end;
  return lhs.var > rhs.var;
}

void LinearScanAllocator::Allocate() {
  for (auto& interval : intervals_list) {
    interval.reg.clear();
    interval.spilled = false;
    interval.stack_offset = 0;
  }

  auto allocate_class = [&](bool want_float,
                            const std::vector<std::string>& pool) {
    std::vector<std::string> free_regs = pool;
    std::vector<LiveInterval*> active;

    auto sort_active = [&]() {
      std::sort(active.begin(), active.end(),
                [](const LiveInterval* lhs, const LiveInterval* rhs) {
        if (lhs->end != rhs->end) return lhs->end < rhs->end;
        return lhs->var < rhs->var;
      });
    };

    for (auto& current : intervals_list) {
      if (current.is_float != want_float) continue;

      for (auto it = active.begin(); it != active.end();) {
        // 端点相等时两个值在同一程序点仍可能同时存在（尤其是函数形参
        // 都在入口处初始化），因此只能在 end < start 时释放寄存器。
        if ((*it)->end < current.start) {
          free_regs.push_back((*it)->reg);
          it = active.erase(it);
        } else {
          ++it;
        }
      }

      if (!free_regs.empty()) {
        // 跨调用值优先放入 s*/fs*；短生命周期值优先用 caller-saved，
        // 从而减少函数序言中保存被调用者保存寄存器的数量。
        // 从尾部查找：刚刚过期的区间会被追加到 free_regs 尾部。
        // 二元运算的 lhs 若在当前指令后死亡，新结果就更可能复用其寄存器，
        // 代码生成器因而无需再发出一条 mv/fmv.s。
        bool used_copy_hint = false;
        const auto hint_it = copy_hints_.find(current.var);
        if (hint_it != copy_hints_.end()) {
          const auto source_it = interval_index_.find(hint_it->second);
          if (source_it != interval_index_.end()) {
            const LiveInterval& source = intervals_list[source_it->second];
            auto hinted_reg = std::find(free_regs.begin(), free_regs.end(), source.reg);
            if (!source.spilled && !source.reg.empty() &&
                hinted_reg != free_regs.end() &&
                source.is_float == current.is_float &&
                IsCalleeSaved(source.reg) == current.crosses_call) {
              current.reg = source.reg;
              free_regs.erase(hinted_reg);
              used_copy_hint = true;
            }
          }
        }

        if (!used_copy_hint) {
          auto preferred = std::find_if(
              free_regs.rbegin(), free_regs.rend(), [&](const std::string& reg) {
                return IsCalleeSaved(reg) == current.crosses_call;
              });
          if (preferred == free_regs.rend()) {
            current.reg = free_regs.back();
            free_regs.pop_back();
          } else {
            current.reg = *preferred;
            free_regs.erase(std::next(preferred).base());
          }
        }
        active.push_back(&current);
        sort_active();
        continue;
      }

      // 在“当前区间 + 所有活动区间”中选择代价最低者溢出，不再仅按
      // 结束位置决定。这样循环中的高频变量会尽量保留在寄存器中。
      LiveInterval* victim = &current;
      for (LiveInterval* candidate : active) {
        if (LowerSpillPriority(*candidate, *victim)) victim = candidate;
      }

      if (victim == &current) {
        current.spilled = true;
        continue;
      }

      const std::string transferred_reg = victim->reg;
      victim->spilled = true;
      victim->reg.clear();
      active.erase(std::find(active.begin(), active.end(), victim));
      current.reg = transferred_reg;
      active.push_back(&current);
      sort_active();
    }
  };

  allocate_class(false, reg_pool);
  allocate_class(true, float_pool);
  AssignSpillSlots();
}

void LinearScanAllocator::AssignSpillSlots() {
  struct Slot {
    int offset;
    long long end;
  };

  std::vector<LiveInterval*> spilled;
  for (auto& interval : intervals_list)
    if (interval.spilled) spilled.push_back(&interval);
  std::sort(spilled.begin(), spilled.end(),
            [](const LiveInterval* lhs, const LiveInterval* rhs) {
    if (lhs->start != rhs->start) return lhs->start < rhs->start;
    return lhs->end < rhs->end;
  });

  std::vector<Slot> slots;
  for (LiveInterval* interval : spilled) {
    auto reusable = std::find_if(slots.begin(), slots.end(),
                                 [&](const Slot& slot) {
      return slot.end < interval->start;
    });
    if (reusable == slots.end()) {
      interval->stack_offset = static_cast<int>(slots.size()) * reg_size;
      slots.push_back({interval->stack_offset, interval->end});
    } else {
      interval->stack_offset = reusable->offset;
      reusable->end = interval->end;
    }
  }
}

std::unordered_map<std::string, std::string>
LinearScanAllocator::GetRegMap() const {
  std::unordered_map<std::string, std::string> reg_map;
  for (const auto& interval : intervals_list) {
    if (!interval.spilled && !interval.reg.empty())
      reg_map[interval.var] = interval.reg;
  }
  return reg_map;
}

std::vector<LinearScanAllocator::LiveInterval>
LinearScanAllocator::GetSpilledIntervals() const {
  std::vector<LiveInterval> spilled;
  for (const auto& interval : intervals_list)
    if (interval.spilled) spilled.push_back(interval);
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
    "fs0", "fs1", "fs2", "fs3", "fs4", "fs5", "fs6", "fs7", "fs8", "fs9", "fs10", "fs11",
    "fa0", "fa1", "fa2", "fa3", "fa4", "fa5", "fa6", "fa7"
  };
  return physical_regs.find(var) == physical_regs.end();
}

} // namespace Back
