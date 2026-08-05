#include "riscv_generator.hpp"
#include "../regalloc/linear_scan_allocator.hpp"
#include <iostream>
#include <cassert>
#include <cstring>
#include <algorithm>
#include <cstdint>
#include <cstdlib>

// Internal helpers
namespace {

bool GetPowerOfTwoShift(uint64_t value, unsigned& shift) {
  if (value == 0 || (value & (value - 1)) != 0) return false;
  shift = 0;
  while (value > 1) { value >>= 1; ++shift; }
  return true;
}

bool IsCalleeSavedRegister(const std::string& reg) {
  if (reg.empty()) return false;
  if (reg[0] == 's') return true;
  return reg.size() >= 2 && reg[0] == 'f' && reg[1] == 's';
}

bool IsFloatRegister(const std::string& reg) {
  return !reg.empty() && reg[0] == 'f';
}

bool GetVirtualValueName(const IR::IRValue* value, std::string& name) {
  if (!value) return false;
  if (IR::isa<IR::IRNameInstruction>(value) ||
      IR::isa<IR::IRFuncParam>(value) ||
      IR::isa<IR::IRBlockParam>(value)) {
    name = value->GetOperandName();
    return !name.empty();
  }
  if (const auto* call = IR::dyn_cast<IR::IRCall>(value)) {
    if (!call->HasReturnValue()) return false;
    name = call->GetOperandName();
    return !name.empty();
  }
  return false;
}

}

// Generate (top-level entry)
void Back::RiscvGenerator::Generate(const IR::IRProgram& program) {
  lva_ = std::make_unique<IR::Pass::LiveVarAnalysis>();
  lva_->Analyze(const_cast<IR::IRProgram*>(&program));
  Visit(program);
}

// String helpers
std::string Back::RiscvGenerator::StripAt(const std::string& name) {
  return (!name.empty() && name[0] == '@') ? name.substr(1) : name;
}
std::string Back::RiscvGenerator::StripPercent(const std::string& name) {
  return (!name.empty() && name[0] == '%') ? name.substr(1) : name;
}
std::string Back::RiscvGenerator::GetBlockAsmLabel(const std::string& raw_name) {
  std::string label = StripPercent(raw_name);
  if (label == "entry") return label;
  if (current_func_) return current_func_->name + "_" + label;
  return label;
}

void Back::RiscvGenerator::EmitParallelRegisterCopies(
    std::vector<std::pair<std::string, std::string>> moves,
    bool is_float) {
  moves.erase(std::remove_if(moves.begin(), moves.end(),
                             [](const auto& move) {
    return move.first == move.second;
  }), moves.end());

  const std::string scratch = is_float ? "ft5" : "t5";
  const std::string move_op = is_float ? "fmv.s" : "mv";

  while (!moves.empty()) {
    // dst 不再作为任何待处理 move 的源时，可以安全覆盖。
    auto safe = std::find_if(moves.begin(), moves.end(), [&](const auto& move) {
      return std::none_of(moves.begin(), moves.end(), [&](const auto& other) {
        return other.second == move.first;
      });
    });
    if (safe != moves.end()) {
      emitter_.EmitInst(move_op, safe->first, safe->second);
      moves.erase(safe);
      continue;
    }

    // 剩余图中存在环。先保存一个即将被覆盖的 dst，再把所有对该旧值的
    // 引用改为保留的临时寄存器，环就会转化为可按拓扑顺序执行的链。
    const std::string saved = moves.front().first;
    emitter_.EmitInst(move_op, scratch, saved);
    for (auto& move : moves)
      if (move.second == saved) move.second = scratch;
  }
}

void Back::RiscvGenerator::EmitParallelValueCopies(
    const std::vector<std::shared_ptr<IR::IRValue>>& sources,
    const std::vector<CopyDestination>& destinations,
    int staging_base) {
  assert(sources.size() == destinations.size() &&
         "parallel copy source/destination arity mismatch");

  struct PendingCopy {
    const IR::IRValue* source = nullptr;
    CopyDestination destination;
    std::string source_reg;
    int source_stack = -1;
    int staged_stack = -1;
    bool skip = false;
  };

  std::vector<PendingCopy> copies;
  copies.reserve(sources.size());
  std::unordered_set<int> destination_stack_offsets;
  for (const auto& destination : destinations) {
    if (destination.kind == CopyDestination::Kind::STACK)
      destination_stack_offsets.insert(destination.stack_offset);
  }

  for (size_t i = 0; i < sources.size(); ++i) {
    PendingCopy copy;
    copy.source = sources[i].get();
    copy.destination = destinations[i];

    std::string source_name;
    if (GetVirtualValueName(copy.source, source_name)) {
      const auto reg_it = reg_map_.find(source_name);
      if (reg_it != reg_map_.end() && !reg_it->second.empty()) {
        copy.source_reg = reg_it->second;
      } else {
        const auto stack_it = val_stack_map_.find(source_name);
        if (stack_it != val_stack_map_.end())
          copy.source_stack = stack_it->second;
      }

      // jump %next(%next_arg) 表示该块参数保持原值，不产生真实 copy。
      if (!copy.destination.value_name.empty() &&
          copy.destination.value_name == source_name)
        copy.skip = true;
    }

    if (copy.destination.kind == CopyDestination::Kind::REGISTER &&
        !copy.source_reg.empty() &&
        copy.destination.reg == copy.source_reg)
      copy.skip = true;
    if (copy.destination.kind == CopyDestination::Kind::STACK &&
        copy.source_stack >= 0 &&
        copy.destination.stack_offset == copy.source_stack)
      copy.skip = true;

    copies.push_back(std::move(copy));
  }

  const int slot_size = GetRegSize();
  auto load_source = [&](const PendingCopy& copy, const std::string& reg) {
    if (copy.staged_stack >= 0) {
      if (copy.destination.type == IR::IRType::IR_FLOAT)
        emitter_.EmitFlw(reg, copy.staged_stack);
      else if (GetTypeSize(copy.destination.type) == 8)
        emitter_.EmitLd(reg, copy.staged_stack);
      else
        emitter_.EmitLw(reg, copy.staged_stack);
      return;
    }
    if (copy.destination.type == IR::IRType::IR_FLOAT)
      LoadValueToFReg(copy.source, reg);
    else
      LoadValueToReg(copy.source, reg);
  };
  auto store_destination = [&](const CopyDestination& destination,
                               const std::string& reg) {
    assert(destination.kind == CopyDestination::Kind::STACK);
    if (destination.type == IR::IRType::IR_FLOAT)
      emitter_.EmitFsw(reg, destination.stack_offset);
    else if (GetTypeSize(destination.type) == 8)
      emitter_.EmitSd(reg, destination.stack_offset);
    else
      emitter_.EmitSw(reg, destination.stack_offset);
  };

  // 只有“某个栈源位置同时会被另一条 copy 覆盖”时才使用共享缓冲区。
  // 常量、全局值以及互不覆盖的栈源可在寄存器环处理完成后直接装入。
  for (size_t i = 0; i < copies.size(); ++i) {
    auto& copy = copies[i];
    if (copy.skip || copy.source_stack < 0 ||
        destination_stack_offsets.count(copy.source_stack) == 0)
      continue;
    const int staged_stack =
        staging_base + static_cast<int>(i) * slot_size;
    const std::string scratch =
        copy.destination.type == IR::IRType::IR_FLOAT ? "ft5" : "t5";
    // 此时必须仍从 original source_stack 读取。若先设置 staged_stack，
    // load_source 会误以为暂存已经完成，并从尚未初始化的缓冲槽加载。
    load_source(copy, scratch);
    copy.staged_stack = staged_stack;
    if (copy.destination.type == IR::IRType::IR_FLOAT)
      emitter_.EmitFsw(scratch, copy.staged_stack);
    else if (GetTypeSize(copy.destination.type) == 8)
      emitter_.EmitSd(scratch, copy.staged_stack);
    else
      emitter_.EmitSw(scratch, copy.staged_stack);
  }

  std::vector<std::pair<std::string, std::string>> integer_moves;
  std::vector<std::pair<std::string, std::string>> float_moves;

  // 先完成所有寄存器源到栈目的的写入，避免后续寄存器并行搬运覆盖源值。
  // register -> register 则统一收集后交给环安全调度器。
  for (const auto& copy : copies) {
    if (copy.skip || copy.source_reg.empty()) continue;
    if (copy.destination.kind == CopyDestination::Kind::STACK) {
      store_destination(copy.destination, copy.source_reg);
    } else {
      auto& moves = copy.destination.type == IR::IRType::IR_FLOAT
                        ? float_moves
                        : integer_moves;
      moves.push_back({copy.destination.reg, copy.source_reg});
    }
  }

  EmitParallelRegisterCopies(std::move(integer_moves), false);
  EmitParallelRegisterCopies(std::move(float_moves), true);

  // 最后物化常量、全局值及栈源。此时寄存器交换已经完成；可能被栈目的
  // 覆盖的源已经提前保存到缓冲区，因此不会破坏并行语义。
  for (const auto& copy : copies) {
    if (copy.skip || !copy.source_reg.empty()) continue;
    if (copy.destination.kind == CopyDestination::Kind::REGISTER) {
      load_source(copy, copy.destination.reg);
    } else {
      const std::string scratch =
          copy.destination.type == IR::IRType::IR_FLOAT ? "ft5" : "t5";
      load_source(copy, scratch);
      store_destination(copy.destination, scratch);
    }
  }
}

// Program & global alloc
void Back::RiscvGenerator::Visit(const IR::IRProgram& program) {
  for (const auto& g : program.globals) Visit(*g);
  emitter_.EmitDirective(".text");
  for (const auto& f : program.funcs) Visit(*f);
}

void Back::RiscvGenerator::Visit(const IR::IRGlobalAlloc& global) {
  std::string name = StripAt(global.GetOperandName());
  emitter_.EmitDirective(".data");
  emitter_.EmitInst(".globl", name);
  emitter_.EmitLabel(name);

  if (global.IsZeroInit() ||
      (global.GetInitVals().empty() && global.GetInitFloatVals().empty())) {
    emitter_.EmitInst(".zero", global.GetAllocSize());
  } else if (!global.GetInitFloatVals().empty()) {
    for (float v : global.GetInitFloatVals()) {
      int32_t bits = 0;
      std::memcpy(&bits, &v, sizeof(bits));
      emitter_.EmitInst(".word", bits);
    }
  } else {
    for (int32_t v : global.GetInitVals()) emitter_.EmitInst(".word", v);
  }
  emitter_.Out() << "\n";
}

// Function
void Back::RiscvGenerator::Visit(const IR::IRFunction& func) {
  current_func_ = &func;
  val_stack_map_.clear();
  val_type_map_.clear();
  local_allocs_.clear();
  reg_map_.clear();
  spill_offsets_.clear();
  call_backup_reg_map_.clear();
  loop_reg_overrides_.clear();
  loop_preloads_.clear();
  used_s_regs_.clear();
  has_call_            = false;
  max_stack_arg_count_ = 0;
  max_call_arg_count_  = 0;
  max_edge_arg_count_  = 0;

  std::unordered_set<std::string> virtual_regs;
  // 形参同样是 SSA 值，必须参与活跃分析和寄存器分配。
  for (const auto& param : func.params)
    if (param) virtual_regs.insert(param->GetOperandName());
  for (const auto& block : func.blocks) {
    for (const auto& inst : block->insts) {
      bool has_retval = false;
      std::string val_name;
      if (IR::isa<IR::IRNameInstruction>(inst.get())) {
        const auto* ni = IR::cast<IR::IRNameInstruction>(inst.get());
        if (IR::isa<IR::IRAlloc>(inst.get())) {
          const auto* al = IR::cast<IR::IRAlloc>(inst.get());
          if (!al->IsArray()) { has_retval = true; val_name = ni->GetOperandName(); }
        } else {
          has_retval = true; val_name = ni->GetOperandName();
        }
      } else if (IR::isa<IR::IRCall>(inst.get())) {
        const auto* ci = IR::cast<IR::IRCall>(inst.get());
        if (ci->HasReturnValue()) { has_retval = true; val_name = ci->GetOperandName(); }
      }
      if (has_retval) virtual_regs.insert(val_name);
    }
  }

  std::unordered_map<std::string, std::string> copy_hints;
  for (const auto& block : func.blocks) {
    for (const auto& param : block->params)
      if (param) virtual_regs.insert(param->GetOperandName());
    for (const auto& inst : block->insts) {
      const auto* binary = IR::dyn_cast<IR::IRBinary>(inst.get());
      if (!binary) continue;
      std::string lhs_name;
      if (!GetVirtualValueName(binary->GetLhs().get(), lhs_name)) continue;
      const bool lhs_float = binary->GetLhs()->GetType() == IR::IRType::IR_FLOAT;
      const bool result_float = binary->GetType() == IR::IRType::IR_FLOAT;
      if (lhs_float == result_float)
        copy_hints[binary->GetOperandName()] = lhs_name;
    }
  }

  std::vector<LinearScanAllocator::LiveInterval> allocated_intervals;
  if (lva_) {
    auto it = lva_->func_intervals.find(func.name);
    if (it != lva_->func_intervals.end()) {
      std::unordered_map<std::string, std::array<long long int, 2>> filtered;
      for (const auto& [var, range] : it->second)
        if (virtual_regs.count(var)) filtered[var] = range;

      const auto& crosses_calls = lva_->func_crosses_calls[func.name];
      const auto& float_values = lva_->func_float_values[func.name];
      const auto& use_weights = lva_->func_use_weights[func.name];
      const auto& use_counts = lva_->func_use_counts[func.name];
      LinearScanAllocator alloc(filtered, GetRegSize(), crosses_calls,
                                float_values, use_weights, use_counts,
                                copy_hints);
      alloc.Allocate();
      allocated_intervals = alloc.GetIntervals();
      reg_map_ = alloc.GetRegMap();
      for (const auto& s : alloc.GetSpilledIntervals())
        spill_offsets_[s.var] = s.stack_offset;

      const char* stats_mode = std::getenv("COMPILER_REGALLOC_STATS");
      if (stats_mode && std::string(stats_mode) != "0") {
        std::unordered_set<int> spill_slots;
        std::size_t spill_count = 0;
        for (const auto& interval : alloc.GetIntervals()) {
          if (!interval.spilled) continue;
          ++spill_count;
          spill_slots.insert(interval.stack_offset);
        }
        std::cerr << "[regalloc] function=" << func.name
                  << " intervals=" << alloc.GetIntervals().size()
                  << " registers=" << reg_map_.size()
                  << " spills=" << spill_count
                  << " spill_slots=" << spill_slots.size() << "\n";

        if (std::string(stats_mode) == "detail") {
          for (const auto& interval : alloc.GetIntervals()) {
            std::cerr << "  " << interval.var
                      << " [" << interval.start << "," << interval.end << "]"
                      << " uses=" << interval.use_count
                      << " weight=" << interval.use_weight
                      << " cross_call=" << (interval.crosses_call ? 1 : 0)
                      << " class=" << (interval.is_float ? "float" : "integer")
                      << " location=";
            if (interval.spilled)
              std::cerr << "stack+" << interval.stack_offset;
            else
              std::cerr << interval.reg;
            auto hint = copy_hints.find(interval.var);
            if (hint != copy_hints.end()) std::cerr << " hint=" << hint->second;
            std::cerr << "\n";
          }
        }
      }
    }
  }

  // ------------------------------------------------------------------
  // 有限活跃区间切分
  // ------------------------------------------------------------------
  if (GetOptimizationLevel() == OptimizationLevel::O1) {
  // 全局线性扫描已经占用的 callee-saved 寄存器不能再用于短分段。
  // t5/ft5 仍保留给代码生成和并行 copy 调度器，不参与此处选择。
  std::unordered_set<std::string> occupied_registers;
  for (const auto& [var, reg] : reg_map_) occupied_registers.insert(reg);
  std::vector<std::string> free_integer_callee;
  std::vector<std::string> free_float_callee;
  // s0/fs0 未被当前后端用作 frame pointer，也没有放入全局线性扫描池，
  // 因而天然适合作为第一个分段寄存器；使用时仍会按 ABI 保存和恢复。
  for (int i = 0; i <= 11; ++i) {
    const std::string integer_reg = "s" + std::to_string(i);
    const std::string float_reg = "fs" + std::to_string(i);
    if (occupied_registers.count(integer_reg) == 0)
      free_integer_callee.push_back(integer_reg);
    if (occupied_registers.count(float_reg) == 0)
      free_float_callee.push_back(float_reg);
  }

  struct CallSplitCandidate {
    std::string value;
    std::string source_reg;
    bool is_float = false;
    std::uint64_t call_count = 0;
    std::uint64_t use_weight = 0;
  };
  std::vector<CallSplitCandidate> call_candidates;
  const auto& call_cross_counts = lva_->func_call_cross_counts[func.name];
  const auto& value_use_weights = lva_->func_use_weights[func.name];
  const auto& float_values = lva_->func_float_values[func.name];
  for (const auto& [value, count] : call_cross_counts) {
    // 只为跨越至少两个 call 的值保留备份寄存器。单次 call 使用栈式
    // save/restore 往往与函数序言保存 s*/fs* 的成本相当。
    if (count < 2) continue;
    const auto reg_it = reg_map_.find(value);
    if (reg_it == reg_map_.end() || reg_it->second.empty() ||
        IsCalleeSavedRegister(reg_it->second))
      continue;
    const auto weight_it = value_use_weights.find(value);
    call_candidates.push_back({
        value, reg_it->second, float_values.count(value) != 0, count,
        weight_it == value_use_weights.end() ? 0 : weight_it->second});
  }
  std::sort(call_candidates.begin(), call_candidates.end(),
            [](const CallSplitCandidate& lhs, const CallSplitCandidate& rhs) {
    if (lhs.call_count != rhs.call_count) return lhs.call_count > rhs.call_count;
    if (lhs.use_weight != rhs.use_weight) return lhs.use_weight > rhs.use_weight;
    return lhs.value < rhs.value;
  });
  for (const auto& candidate : call_candidates) {
    auto& pool = candidate.is_float ? free_float_callee : free_integer_callee;
    if (pool.empty()) continue;
    call_backup_reg_map_[candidate.value] = pool.back();
    pool.pop_back();
  }

  // 循环切分只考虑“全局分配已经 spill、定义位于循环外、在循环内只读
  // 使用”的 SSA 值。SSA 值不可变，因此在 preheader 加载一次即可，循环
  // 出口不需要写回。循环携带的 block parameter 和循环内定义全部跳过。
  std::unordered_map<std::string, std::string> definition_blocks;
  std::unordered_set<std::string> non_promotable_values;
  for (const auto& block : func.blocks) {
    for (const auto& param : block->params)
      definition_blocks[param->GetOperandName()] = block->name;
    for (const auto& inst : block->insts) {
      std::string definition;
      if (IR::isa<IR::IRNameInstruction>(inst.get())) {
        definition = inst->GetOperandName();
        if (IR::isa<IR::IRAlloc>(inst.get()))
          non_promotable_values.insert(definition);
      } else if (const auto* call = IR::dyn_cast<IR::IRCall>(inst.get())) {
        if (call->HasReturnValue()) definition = call->GetOperandName();
      }
      if (!definition.empty()) definition_blocks[definition] = block->name;
    }
  }

  struct LoopSplitCandidate {
    std::string value;
    const IR::Pass::LiveVarAnalysis::NaturalLoopRegion* loop = nullptr;
    bool is_float = false;
    std::uint64_t weighted_uses = 0;
    std::uint64_t static_uses = 0;
  };
  std::vector<LoopSplitCandidate> loop_candidates;
  const auto& loop_depths = lva_->func_loop_depth[func.name];
  const auto& loop_regions = lva_->func_natural_loops[func.name];
  for (const auto& interval : allocated_intervals) {
    if (!interval.spilled || non_promotable_values.count(interval.var) != 0)
      continue;
    for (const auto& loop : loop_regions) {
      if (loop.preheader.empty()) continue;
      const auto def_it = definition_blocks.find(interval.var);
      if (def_it != definition_blocks.end() &&
          loop.blocks.count(def_it->second) != 0)
        continue;

      std::uint64_t weighted_uses = 0;
      std::uint64_t static_uses = 0;
      for (const auto& block : func.blocks) {
        if (loop.blocks.count(block->name) == 0) continue;
        const auto depth_it = loop_depths.find(block->name);
        const int depth = depth_it == loop_depths.end() ? 0 : depth_it->second;
        const std::uint64_t weight =
            std::uint64_t{1} << (3 * std::min(depth, 3));
        for (const auto& inst : block->insts) {
          for (const auto& use : inst->GetUses()) {
            std::string use_name;
            if (GetVirtualValueName(use.get(), use_name) &&
                use_name == interval.var) {
              ++static_uses;
              weighted_uses += weight;
            }
          }
        }
      }
      if (static_uses != 0)
        loop_candidates.push_back({interval.var, &loop, interval.is_float,
                                   weighted_uses, static_uses});
    }
  }
  std::sort(loop_candidates.begin(), loop_candidates.end(),
            [](const LoopSplitCandidate& lhs, const LoopSplitCandidate& rhs) {
    if (lhs.weighted_uses != rhs.weighted_uses)
      return lhs.weighted_uses > rhs.weighted_uses;
    if (lhs.static_uses != rhs.static_uses)
      return lhs.static_uses > rhs.static_uses;
    if (lhs.loop->depth != rhs.loop->depth)
      return lhs.loop->depth > rhs.loop->depth;
    return lhs.value < rhs.value;
  });

  std::unordered_set<std::string> promoted_loop_values;
  for (const auto& candidate : loop_candidates) {
    if (!candidate.loop || promoted_loop_values.count(candidate.value) != 0)
      continue;
    auto& pool = candidate.is_float ? free_float_callee : free_integer_callee;
    if (pool.empty()) continue;
    const std::string reg = pool.back();
    pool.pop_back();
    promoted_loop_values.insert(candidate.value);
    loop_preloads_[candidate.loop->preheader].push_back({candidate.value, reg});
    for (const auto& block_name : candidate.loop->blocks)
      loop_reg_overrides_[block_name][candidate.value] = reg;
  }

  const char* split_stats_mode = std::getenv("COMPILER_REGALLOC_STATS");
  if (split_stats_mode && std::string(split_stats_mode) != "0") {
    std::cerr << "[regalloc-split] function=" << func.name
              << " call_backups=" << call_backup_reg_map_.size()
              << " loop_promotions=" << promoted_loop_values.size() << "\n";
    if (std::string(split_stats_mode) == "detail") {
      for (const auto& [value, reg] : call_backup_reg_map_)
        std::cerr << "  call " << value << " -> " << reg << "\n";
      for (const auto& [preheader, preloads] : loop_preloads_)
        for (const auto& [value, reg] : preloads)
          std::cerr << "  loop " << value << " -> " << reg
                    << " preheader=" << preheader << "\n";
    }
  }
  }

  for (const auto& [var, reg] : reg_map_) {
    if (IsCalleeSavedRegister(reg)) {
      if (std::find(used_s_regs_.begin(), used_s_regs_.end(), reg) == used_s_regs_.end())
        used_s_regs_.push_back(reg);
    }
  }
  for (const auto& [value, reg] : call_backup_reg_map_) {
    if (std::find(used_s_regs_.begin(), used_s_regs_.end(), reg) == used_s_regs_.end())
      used_s_regs_.push_back(reg);
  }
  for (const auto& [preheader, preloads] : loop_preloads_) {
    for (const auto& [value, reg] : preloads) {
      if (std::find(used_s_regs_.begin(), used_s_regs_.end(), reg) == used_s_regs_.end())
        used_s_regs_.push_back(reg);
    }
  }

  int spill_size = 0;
  for (const auto& [v, off] : spill_offsets_)
    if (off + GetRegSize() > spill_size) spill_size = off + GetRegSize();

  int offset = AlignUp(spill_size, GetRegSize());
  s_reg_start_offset_ = offset;
  offset += (int)used_s_regs_.size() * GetRegSize();

  const auto& values_crossing_calls = lva_->func_crosses_calls[func.name];
  auto needs_stack_home = [&](const std::string& value) {
    const auto reg_it = reg_map_.find(value);
    if (reg_it == reg_map_.end() || reg_it->second.empty()) return true;
    if (call_backup_reg_map_.count(value) != 0) return false;
    return !IsCalleeSavedRegister(reg_it->second) &&
           values_crossing_calls.count(value) != 0;
  };

  for (const auto& block : func.blocks) {
    for (const auto& inst : block->insts) {
      if (IR::isa<IR::IRNameInstruction>(inst.get())) {
        const auto* ni = IR::cast<IR::IRNameInstruction>(inst.get());
        if (IR::isa<IR::IRAlloc>(inst.get())) {
          const auto* al = IR::cast<IR::IRAlloc>(inst.get());
          local_allocs_.insert(ni->GetOperandName());
          val_type_map_[ni->GetOperandName()]  = al->GetAllocatedType();
          int sz = al->IsArray() ? al->GetAllocSize() : GetTypeSize(al->GetAllocatedType());
          int alignment = al->IsArray() ? 4 : GetTypeAlignment(al->GetAllocatedType());
          val_stack_map_[ni->GetOperandName()] =
              ReserveStackSlot(offset, sz, alignment);
        }
      }
    }
  }

  for (size_t i = 0; i < func.params.size(); ++i) {
    std::string pn = func.params[i]->GetOperandName();
    val_type_map_[pn] = func.params[i]->GetType();
    if (spill_offsets_.count(pn)) {
      val_stack_map_[pn] = spill_offsets_[pn];
    } else if (needs_stack_home(pn)) {
      const IR::IRType type = func.params[i]->GetType();
      val_stack_map_[pn] =
          ReserveStackSlot(offset, GetTypeSize(type), GetTypeAlignment(type));
    }
  }

  for (const auto& block : func.blocks) {
    for (const auto& inst : block->insts) {
      bool has_retval = false; std::string val_name;
      if (IR::isa<IR::IRNameInstruction>(inst.get())) {
        const auto* ni = IR::cast<IR::IRNameInstruction>(inst.get());
        has_retval = true; val_name = ni->GetOperandName();
      } else if (IR::isa<IR::IRCall>(inst.get())) {
        const auto* ci = IR::cast<IR::IRCall>(inst.get());
        if (ci->HasReturnValue()) { has_retval = true; val_name = ci->GetOperandName(); }
      }
      if (has_retval) {
        if (IR::isa<IR::IRAlloc>(inst.get())) {
          const auto* al = IR::cast<IR::IRAlloc>(inst.get());
          val_type_map_[val_name] = al->GetAllocatedType();
        } else {
          val_type_map_[val_name] = inst->GetType();
        }
      }
    }
  }
  for (const auto& [v, off] : spill_offsets_)
    if (!val_stack_map_.count(v)) val_stack_map_[v] = off;

  for (const auto& block : func.blocks) {
    for (const auto& inst : block->insts) {
      if (IR::isa<IR::IRCall>(inst.get())) {
        const auto* ci = IR::cast<IR::IRCall>(inst.get());
        has_call_ = true;
        max_call_arg_count_ = std::max(max_call_arg_count_,
                                       static_cast<int>(ci->GetArgs().size()));
        std::vector<IR::IRType> arg_types;
        for (const auto& a : ci->GetArgs()) arg_types.push_back(a->GetType());
        int stack_args = 0;
        for (const auto& loc : calling_convention_.AssignArgumentLocations(arg_types))
          if (loc.kind == ArgumentLocation::Kind::STACK) ++stack_args;
        if (stack_args > max_stack_arg_count_) max_stack_arg_count_ = stack_args;
      }
      bool has_retval = false; std::string val_name;
      if (IR::isa<IR::IRNameInstruction>(inst.get())) {
        const auto* ni = IR::cast<IR::IRNameInstruction>(inst.get());
        has_retval = true; val_name = ni->GetOperandName();
      } else if (IR::isa<IR::IRCall>(inst.get())) {
        const auto* ci = IR::cast<IR::IRCall>(inst.get());
        if (ci->HasReturnValue()) { has_retval = true; val_name = ci->GetOperandName(); }
      }
      if (has_retval && !val_stack_map_.count(val_name) &&
          needs_stack_home(val_name)) {
        IR::IRType value_type;
        if (IR::isa<IR::IRAlloc>(inst.get())) {
          const auto* al = IR::cast<IR::IRAlloc>(inst.get());
          value_type = al->GetAllocatedType();
        } else {
          value_type = inst->GetType();
        }
        val_type_map_[val_name] = value_type;
        val_stack_map_[val_name] =
            ReserveStackSlot(offset, GetTypeSize(value_type),
                             GetTypeAlignment(value_type));
      }
    }
  }

  for (const auto& block : func.blocks) {
    for (const auto& param : block->params) {
      const std::string name = param->GetOperandName();
      val_type_map_[name] = param->GetType();
      if (spill_offsets_.count(name)) {
        val_stack_map_[name] = spill_offsets_[name];
      } else if (needs_stack_home(name)) {
        val_stack_map_[name] =
            ReserveStackSlot(offset, GetTypeSize(param->GetType()),
                             GetTypeAlignment(param->GetType()));
      }
    }
    if (block->insts.empty()) continue;
    if (auto* jump = IR::dyn_cast<IR::IRJump>(block->insts.back().get())) {
      max_edge_arg_count_ = std::max(max_edge_arg_count_,
                                      static_cast<int>(jump->GetArgs().size()));
    } else if (auto* branch = IR::dyn_cast<IR::IRBranch>(block->insts.back().get())) {
      max_edge_arg_count_ = std::max(
          max_edge_arg_count_,
          static_cast<int>(std::max(branch->GetThenArgs().size(),
                                    branch->GetElseArgs().size())));
    }
  }

  int local_alloc_size = offset - spill_size - (int)used_s_regs_.size() * GetRegSize();

  StackLayoutBuilder builder;
  // 基本块参数和call的共享空间
  max_call_arg_count_ = std::max(max_call_arg_count_, max_edge_arg_count_);
  stack_layout_ = builder.Build(used_s_regs_, spill_offsets_,
                                max_stack_arg_count_, max_call_arg_count_,
                                has_call_,
                                local_alloc_size,
                                GetRegSize(), target_.GetStackAlign());
  current_stack_size_  = stack_layout_.total_size;
  param_overflow_size_ = stack_layout_.param_overflow_size;
  int arg_buf_size     = stack_layout_.arg_buffer_size;

  for (auto& [v, off] : val_stack_map_) off += param_overflow_size_ + arg_buf_size;
  s_reg_start_offset_ += param_overflow_size_ + arg_buf_size;

  emitter_.EmitInst(".globl", func.name);
  emitter_.EmitLabel(func.name);

  if (current_stack_size_ > 0) {
    if (current_stack_size_ <= 2048)
      emitter_.EmitInst("addi", "sp", "sp", "-" + std::to_string(current_stack_size_));
    else {
      emitter_.EmitInst("li", "t5", "-" + std::to_string(current_stack_size_));
      emitter_.EmitInst("add", "sp", "sp", "t5");
    }
  }

  if (has_call_) {
    int ra_off = current_stack_size_ - GetRegSize();
    if (target_.Is64Bit()) emitter_.EmitSd("ra", ra_off);
    else           emitter_.EmitSw("ra", ra_off);
  }

  for (size_t i = 0; i < used_s_regs_.size(); ++i) {
    int s_off = s_reg_start_offset_ + (int)i * GetRegSize();
    if (IsFloatRegister(used_s_regs_[i])) {
      if (target_.Is64Bit()) emitter_.EmitFsd(used_s_regs_[i], s_off);
      else                   emitter_.EmitFsw(used_s_regs_[i], s_off);
    }
    else if (target_.Is64Bit()) emitter_.EmitSd(used_s_regs_[i], s_off);
    else                        emitter_.EmitSw(used_s_regs_[i], s_off);
  }

  std::vector<IR::IRType> param_types;
  for (const auto& p : func.params) param_types.push_back(p->GetType());
  const auto param_locs = calling_convention_.AssignArgumentLocations(param_types);

  std::vector<std::pair<std::string, std::string>> integer_param_moves;
  std::vector<std::pair<std::string, std::string>> float_param_moves;

  // 没有分配到寄存器的寄存器形参必须先保存到 home。其余形参构成
  // “目标寄存器 <- ABI 源寄存器”的并行 copy，交给环安全调度器处理。
  for (size_t i = 0; i < func.params.size(); ++i) {
    const std::string pn = func.params[i]->GetOperandName();
    const auto& loc = param_locs[i];
    if (loc.kind == ArgumentLocation::Kind::STACK) continue;

    const bool is_float = loc.kind == ArgumentLocation::Kind::FLOAT_REGISTER;
    const std::string source = (is_float ? "fa" : "a") +
                               std::to_string(loc.index);
    const auto reg_it = reg_map_.find(pn);
    if (reg_it != reg_map_.end() && !reg_it->second.empty()) {
      auto& moves = is_float ? float_param_moves : integer_param_moves;
      moves.push_back({reg_it->second, source});
      continue;
    }

    const int stack_offset = val_stack_map_.at(pn);
    if (is_float) emitter_.EmitFsw(source, stack_offset);
    else if (GetTypeSize(func.params[i]->GetType()) == 8)
      emitter_.EmitSd(source, stack_offset);
    else
      emitter_.EmitSw(source, stack_offset);
  }

  EmitParallelRegisterCopies(std::move(integer_param_moves), false);
  EmitParallelRegisterCopies(std::move(float_param_moves), true);

  // ABI 栈形参在并行寄存器 copy 完成后处理。若该值有物理寄存器，直接
  // 从调用者栈加载到最终位置；只有真正 spill 的形参才复制到本地 home。
  for (size_t i = 0; i < func.params.size(); ++i) {
    if (param_locs[i].kind != ArgumentLocation::Kind::STACK) continue;
    const std::string pn = func.params[i]->GetOperandName();
    const auto& loc = param_locs[i];
    const int caller_offset = current_stack_size_ + GetRegSize() * loc.index;
    const auto reg_it = reg_map_.find(pn);
    if (reg_it != reg_map_.end() && !reg_it->second.empty()) {
      if (func.params[i]->GetType() == IR::IRType::IR_FLOAT)
        emitter_.EmitFlw(reg_it->second, caller_offset);
      else if (GetTypeSize(func.params[i]->GetType()) == 8)
        emitter_.EmitLd(reg_it->second, caller_offset);
      else
        emitter_.EmitLw(reg_it->second, caller_offset);
      continue;
    }

    const int stack_offset = val_stack_map_.at(pn);
    if (func.params[i]->GetType() == IR::IRType::IR_FLOAT) {
      emitter_.EmitFlw("ft5", caller_offset);
      emitter_.EmitFsw("ft5", stack_offset);
    } else if (GetTypeSize(func.params[i]->GetType()) == 8) {
      emitter_.EmitLd("t5", caller_offset);
      emitter_.EmitSd("t5", stack_offset);
    } else {
      emitter_.EmitLw("t5", caller_offset);
      emitter_.EmitSw("t5", stack_offset);
    }
  }

  for (const auto& block : func.blocks) Visit(*block);
}

// Basic block
void Back::RiscvGenerator::Visit(const IR::IRBasicBlock& block) {
  std::string label = GetBlockAsmLabel(block.name);
  if (label != "entry") emitter_.EmitLabel(label);

  // 循环边界切分只在循环块的生成期间覆盖物理位置。离开该块后恢复
  // 全函数分配结果，循环外的使用仍然从原 spill slot 读取。
  std::vector<std::pair<std::string, std::string>> previous_registers;
  std::vector<std::string> previously_unassigned;
  const auto override_it = loop_reg_overrides_.find(block.name);
  if (override_it != loop_reg_overrides_.end()) {
    for (const auto& [value, reg] : override_it->second) {
      const auto old = reg_map_.find(value);
      if (old == reg_map_.end())
        previously_unassigned.push_back(value);
      else
        previous_registers.push_back({value, old->second});
      reg_map_[value] = reg;
    }
  }

  for (size_t instruction_index = 0;
       instruction_index < block.insts.size(); ++instruction_index) {
    const auto& inst = block.insts[instruction_index];
    if (inst->inst_type == IR::IRInstructionType::IR_INST_COMMENT) {
      if (const auto* c = IR::dyn_cast<IR::IRComment>(inst.get())) {
        emitter_.EmitComment(c->comment);
      }
      continue;
    }

    // preheader 的所有普通指令已经生成后、terminator 执行前，把循环外
    // spilled 值装入专用寄存器。这样定义恰好位于 preheader 的值也已经
    // 可用，而循环每次迭代不会重复 load。
    if (instruction_index + 1 == block.insts.size()) {
      const auto preload_it = loop_preloads_.find(block.name);
      if (preload_it != loop_preloads_.end()) {
        for (const auto& [value, reg] : preload_it->second) {
          const auto type_it = val_type_map_.find(value);
          const IR::IRType type = type_it == val_type_map_.end()
                                      ? IR::IRType::IR_INT32
                                      : type_it->second;
          LoadValueToRegByName(value, reg, type);
        }
      }
    }
    bool is_call = (inst->inst_type == IR::IRInstructionType::IR_INST_CALL);

    std::vector<std::pair<std::string, std::string>> regs_to_save;
    if (is_call && lva_) {
      auto live_it = lva_->live_out_inst.find(inst);
      if (live_it != lva_->live_out_inst.end()) {
        std::string def_name;
        if (auto cp = std::dynamic_pointer_cast<IR::IRCall>(inst))
          if (cp->HasReturnValue()) def_name = cp->GetOperandName();
        for (const auto& var : live_it->second) {
          if (var == def_name) continue;
          auto ri = reg_map_.find(var);
          if (ri != reg_map_.end() && !ri->second.empty() &&
              !IsCalleeSavedRegister(ri->second))
            regs_to_save.push_back({var, ri->second});
        }
      }
    }

    for (const auto& [var, reg] : regs_to_save) {
      const auto backup_it = call_backup_reg_map_.find(var);
      if (backup_it != call_backup_reg_map_.end()) {
        emitter_.EmitInst(IsFloatRegister(reg) ? "fmv.s" : "mv",
                          backup_it->second, reg);
      } else {
        auto ti = val_type_map_.find(var);
        IR::IRType t = (ti != val_type_map_.end()) ? ti->second : IR::IRType::IR_INT32;
        StoreRegToValByName(var, reg, t);
      }
    }

    Visit(*inst);

    for (const auto& [var, reg] : regs_to_save) {
      const auto backup_it = call_backup_reg_map_.find(var);
      if (backup_it != call_backup_reg_map_.end()) {
        emitter_.EmitInst(IsFloatRegister(reg) ? "fmv.s" : "mv",
                          reg, backup_it->second);
      } else {
        auto ti = val_type_map_.find(var);
        IR::IRType t = (ti != val_type_map_.end()) ? ti->second : IR::IRType::IR_INT32;
        LoadValueToRegByName(var, reg, t);
      }
    }
  }

  for (const auto& value : previously_unassigned) reg_map_.erase(value);
  for (const auto& [value, reg] : previous_registers) reg_map_[value] = reg;
}

// Instruction dispatch
void Back::RiscvGenerator::Visit(const IR::IRInstruction& inst) {
  switch (inst.inst_type) {
    case IR::IRInstructionType::IR_INST_RET:
      Visit(*IR::cast<IR::IRReturn>(&inst));
      break;
    case IR::IRInstructionType::IR_INST_BINARY:
      Visit(*IR::cast<IR::IRBinary>(&inst));
      break;
    case IR::IRInstructionType::IR_INST_CAST:
      Visit(*IR::cast<IR::IRCast>(&inst));
      break;
    case IR::IRInstructionType::IR_INST_ALLOC:
      Visit(*IR::cast<IR::IRAlloc>(&inst));
      break;
    case IR::IRInstructionType::IR_INST_STORE:
      Visit(*IR::cast<IR::IRStore>(&inst));
      break;
    case IR::IRInstructionType::IR_INST_LOAD:
      Visit(*IR::cast<IR::IRLoad>(&inst));
      break;
    case IR::IRInstructionType::IR_INST_JUMP:
      Visit(*IR::cast<IR::IRJump>(&inst));
      break;
    case IR::IRInstructionType::IR_INST_BR:
      Visit(*IR::cast<IR::IRBranch>(&inst));
      break;
    case IR::IRInstructionType::IR_INST_CALL:
      Visit(*IR::cast<IR::IRCall>(&inst));
      break;
    case IR::IRInstructionType::IR_INST_GETELEMPTR:
      Visit(*IR::cast<IR::IRGetElemPtr>(&inst));
      break;
    case IR::IRInstructionType::IR_INST_GETPTR:
      Visit(*IR::cast<IR::IRGetPtr>(&inst));
      break;
    case IR::IRInstructionType::IR_INST_ZERO_INIT:
      Visit(*IR::cast<IR::IRZeroInit>(&inst));
      break;
    default:
      assert(false && "Unknown instruction type");
  }
}

// Return
void Back::RiscvGenerator::Visit(const IR::IRReturn& inst) {
  if (inst.GetValue()) {
    if (inst.GetValue()->GetType() == IR::IRType::IR_FLOAT)
      LoadValueToFReg(inst.GetValue().get(), "fa0");
    else
      LoadValueToReg(inst.GetValue().get(), "a0");
  }

  for (size_t i = 0; i < used_s_regs_.size(); ++i) {
    int off = s_reg_start_offset_ + (int)i * GetRegSize();
    if (IsFloatRegister(used_s_regs_[i])) {
      if (target_.Is64Bit()) emitter_.EmitFld(used_s_regs_[i], off);
      else                   emitter_.EmitFlw(used_s_regs_[i], off);
    }
    else if (target_.Is64Bit()) emitter_.EmitLd(used_s_regs_[i], off);
    else                        emitter_.EmitLw(used_s_regs_[i], off);
  }
  if (has_call_) {
    int ra_off = current_stack_size_ - GetRegSize();
    if (target_.Is64Bit()) emitter_.EmitLd("ra", ra_off);
    else           emitter_.EmitLw("ra", ra_off);
  }
  if (current_stack_size_ > 0) {
    if (current_stack_size_ <= 2047)
      emitter_.EmitInst("addi", "sp", "sp", std::to_string(current_stack_size_));
    else {
      emitter_.EmitInst("li", "t5", std::to_string(current_stack_size_));
      emitter_.EmitInst("add", "sp", "sp", "t5");
    }
  }
  emitter_.EmitInst("ret");
}

// Mul strength reduction (O1 only)
bool Back::RiscvGenerator::TryEmitMulStrengthReduction(const IR::IRBinary& inst) {
  if (GetOptimizationLevel() != OptimizationLevel::O1 ||
      inst.GetOp() != IR::IRBinaryOp::MUL ||
      inst.GetOperandType() != IR::IRType::IR_INT32) return false;

  auto try_const = [&](const IR::IRConstant* c, const IR::IRValue* other) -> bool {
    if (!c || c->GetType() != IR::IRType::IR_INT32) return false;
    int64_t sv = std::get<int32_t>(c->GetValue());
    if (sv == 0) {
      emitter_.EmitInst("mv", "t5", "x0");
      StoreRegToVal(&inst, "t5");
      return true;
    }
    bool neg = sv < 0;
    uint64_t mag = neg ? static_cast<uint64_t>(-sv) : static_cast<uint64_t>(sv);
    unsigned shift = 0;
    if (!GetPowerOfTwoShift(mag, shift)) return false;
    LoadValueToReg(other, "t5");
    if (shift != 0) emitter_.EmitInst(target_.Is64Bit() ? "slliw" : "slli", "t5", "t5", std::to_string(shift));
    if (neg)        emitter_.EmitInst(target_.Is64Bit() ? "subw"  : "sub",  "t5", "x0", "t5");
    StoreRegToVal(&inst, "t5");
    return true;
  };

  const auto* rhs_c = IR::dyn_cast<IR::IRConstant>(inst.GetRhs().get());
  if (try_const(rhs_c, inst.GetLhs().get())) return true;
  const auto* lhs_c = IR::dyn_cast<IR::IRConstant>(inst.GetLhs().get());
  return try_const(lhs_c, inst.GetRhs().get());
}

// 有符号二次幂除法/取模强度削弱（只在 O1 启用）。
// C/SysY 的有符号除法向 0 取整，负数不能直接使用算术右移：
//   q = (x + ((x >> 31) & (2^k - 1))) >> k
// 取模再由 r = x - q * 2^k 得到。除数为负只影响商的符号，不影响余数。
bool Back::RiscvGenerator::TryEmitSignedPowerOfTwoDivRem(
    const IR::IRBinary& inst) {
  if (GetOptimizationLevel() != OptimizationLevel::O1 ||
      inst.GetOperandType() != IR::IRType::IR_INT32 ||
      (inst.GetOp() != IR::IRBinaryOp::DIV &&
       inst.GetOp() != IR::IRBinaryOp::MOD))
    return false;

  const auto* divisor_value =
      IR::dyn_cast<IR::IRConstant>(inst.GetRhs().get());
  if (!divisor_value || divisor_value->GetType() != IR::IRType::IR_INT32)
    return false;
  const int64_t divisor = std::get<int32_t>(divisor_value->GetValue());
  if (divisor == 0) return false;

  const bool negative_divisor = divisor < 0;
  const uint64_t magnitude = negative_divisor
                                 ? static_cast<uint64_t>(-divisor)
                                 : static_cast<uint64_t>(divisor);
  unsigned shift = 0;
  if (!GetPowerOfTwoShift(magnitude, shift)) return false;

  LoadValueToReg(inst.GetLhs().get(), "t5");
  if (inst.GetOp() == IR::IRBinaryOp::MOD && shift == 0) {
    emitter_.EmitInst("mv", "t5", "x0");
    StoreRegToVal(&inst, "t5");
    return true;
  }

  const char* arithmetic_shift = target_.Is64Bit() ? "sraiw" : "srai";
  const char* logical_shift = target_.Is64Bit() ? "slliw" : "slli";
  const char* add = target_.Is64Bit() ? "addw" : "add";
  const char* subtract = target_.Is64Bit() ? "subw" : "sub";

  // t6 保存“按正二次幂除数计算”的商，t5 始终保留原被除数。
  if (shift == 0) {
    emitter_.EmitInst("mv", "t6", "t5");
  } else {
    emitter_.EmitInst(arithmetic_shift, "t6", "t5", "31");
    const uint64_t bias_mask = magnitude - 1;
    if (bias_mask <= 2047) {
      emitter_.EmitInst("andi", "t6", "t6", std::to_string(bias_mask));
    } else {
      // t4/t5/t6 均不在寄存器分配池中，可以安全作为 lowering 临时寄存器。
      emitter_.EmitInst("li", "t4", std::to_string(bias_mask));
      emitter_.EmitInst("and", "t6", "t6", "t4");
    }
    emitter_.EmitInst(add, "t6", "t5", "t6");
    emitter_.EmitInst(arithmetic_shift, "t6", "t6",
                      std::to_string(shift));
  }

  if (inst.GetOp() == IR::IRBinaryOp::DIV) {
    if (negative_divisor)
      emitter_.EmitInst(subtract, "t6", "x0", "t6");
    StoreRegToVal(&inst, "t6");
    return true;
  }

  // x % (-2^k) 与 x % 2^k 相同，因此余数路径不需要处理除数符号。
  if (shift != 0)
    emitter_.EmitInst(logical_shift, "t6", "t6", std::to_string(shift));
  emitter_.EmitInst(subtract, "t5", "t5", "t6");
  StoreRegToVal(&inst, "t5");
  return true;
}

// Hacker's Delight 风格的 32 位有符号除法 magic number。multiplier 是
// 有符号高半乘法使用的常量，shift 是乘积高 32 位后的附加右移量。
namespace {
struct SignedDivisionMagic {
  std::int32_t multiplier = 0;
  unsigned shift = 0;
};

SignedDivisionMagic ComputeSignedDivisionMagic(std::int32_t divisor) {
  assert(divisor != 0 && divisor != 1 && divisor != -1);
  const std::uint64_t two31 = std::uint64_t{1} << 31U;
  const std::uint64_t absolute_divisor =
      divisor < 0 ? static_cast<std::uint64_t>(-
                        static_cast<std::int64_t>(divisor))
                  : static_cast<std::uint64_t>(divisor);
  const std::uint64_t sign =
      static_cast<std::uint32_t>(divisor) >> 31U;
  const std::uint64_t t = two31 + sign;
  const std::uint64_t anc = t - 1U - t % absolute_divisor;

  unsigned p = 31;
  std::uint64_t q1 = two31 / anc;
  std::uint64_t r1 = two31 - q1 * anc;
  std::uint64_t q2 = two31 / absolute_divisor;
  std::uint64_t r2 = two31 - q2 * absolute_divisor;
  std::uint64_t delta = 0;
  do {
    ++p;
    q1 *= 2U;
    r1 *= 2U;
    if (r1 >= anc) {
      ++q1;
      r1 -= anc;
    }
    q2 *= 2U;
    r2 *= 2U;
    if (r2 >= absolute_divisor) {
      ++q2;
      r2 -= absolute_divisor;
    }
    delta = absolute_divisor - r2;
  } while (q1 < delta || (q1 == delta && r1 == 0));

  std::uint32_t multiplier_bits =
      static_cast<std::uint32_t>(q2 + 1U);
  if (divisor < 0) multiplier_bits = 0U - multiplier_bits;
  return {static_cast<std::int32_t>(multiplier_bits), p - 32U};
}
}  // namespace

// 非二次幂有符号常量除法/取模。RISC-V 硬件 div/rem 延迟较高，使用
// 预计算乘数的高半乘法、移位和修正得到向 0 取整的商：
//   q = trunc(x / C)
// 余数继续使用 r = x - q * C，确保符号与被除数一致。
bool Back::RiscvGenerator::TryEmitSignedConstantDivRem(
    const IR::IRBinary& inst) {
  if (GetOptimizationLevel() != OptimizationLevel::O1 ||
      inst.GetOperandType() != IR::IRType::IR_INT32 ||
      (inst.GetOp() != IR::IRBinaryOp::DIV &&
       inst.GetOp() != IR::IRBinaryOp::MOD))
    return false;

  const auto* constant = IR::dyn_cast<IR::IRConstant>(inst.GetRhs().get());
  if (!constant || constant->GetType() != IR::IRType::IR_INT32)
    return false;
  const std::int32_t divisor =
      std::get<std::int32_t>(constant->GetValue());
  if (divisor == 0 || divisor == 1 || divisor == -1) return false;
  const std::uint64_t magnitude =
      divisor < 0 ? static_cast<std::uint64_t>(-
                        static_cast<std::int64_t>(divisor))
                  : static_cast<std::uint64_t>(divisor);
  unsigned power_of_two_shift = 0;
  if (GetPowerOfTwoShift(magnitude, power_of_two_shift)) return false;

  const SignedDivisionMagic magic = ComputeSignedDivisionMagic(divisor);
  LoadValueToReg(inst.GetLhs().get(), "t5");
  emitter_.EmitInst("li", "t4", std::to_string(magic.multiplier));
  if (target_.Is64Bit()) {
    // 两个操作数均为符号扩展的 i32，64 位低乘积包含完整的 32x32
    // 结果；算术右移 32 位即可得到有符号高半部分。
    emitter_.EmitInst("mul", "t6", "t5", "t4");
    emitter_.EmitInst("srai", "t6", "t6", "32");
  } else {
    emitter_.EmitInst("mulh", "t6", "t5", "t4");
  }

  const char* add = target_.Is64Bit() ? "addw" : "add";
  const char* subtract = target_.Is64Bit() ? "subw" : "sub";
  const char* arithmetic_shift = target_.Is64Bit() ? "sraiw" : "srai";
  const char* logical_shift = target_.Is64Bit() ? "srliw" : "srli";
  if (divisor > 0 && magic.multiplier < 0)
    emitter_.EmitInst(add, "t6", "t6", "t5");
  else if (divisor < 0 && magic.multiplier > 0)
    emitter_.EmitInst(subtract, "t6", "t6", "t5");
  if (magic.shift != 0)
    emitter_.EmitInst(arithmetic_shift, "t6", "t6",
                      std::to_string(magic.shift));

  // 将向负无穷取整的算术右移修正为 C/SysY 的向 0 取整。
  emitter_.EmitInst(logical_shift, "t4", "t6", "31");
  emitter_.EmitInst(add, "t6", "t6", "t4");

  if (inst.GetOp() == IR::IRBinaryOp::DIV) {
    StoreRegToVal(&inst, "t6");
    return true;
  }

  emitter_.EmitInst("li", "t4", std::to_string(divisor));
  emitter_.EmitInst(target_.Is64Bit() ? "mulw" : "mul", "t4", "t6",
                    "t4");
  emitter_.EmitInst(subtract, "t5", "t5", "t4");
  StoreRegToVal(&inst, "t5");
  return true;
}

// 直接选择 RISC-V I-type 指令，避免先 li 常量再执行寄存器运算。
bool Back::RiscvGenerator::TryEmitIntegerImmediate(
    const IR::IRBinary& inst) {
  if (GetOptimizationLevel() != OptimizationLevel::O1 ||
      inst.GetOperandType() != IR::IRType::IR_INT32)
    return false;

  auto get_constant = [](const std::shared_ptr<IR::IRValue>& value,
                         int32_t& result) {
    const auto* constant = IR::dyn_cast<IR::IRConstant>(value.get());
    if (!constant || constant->GetType() != IR::IRType::IR_INT32)
      return false;
    result = std::get<int32_t>(constant->GetValue());
    return true;
  };
  auto fits_signed_12 = [](int64_t value) {
    return value >= -2048 && value <= 2047;
  };

  int32_t lhs_constant = 0;
  int32_t rhs_constant = 0;
  const bool lhs_is_constant = get_constant(inst.GetLhs(), lhs_constant);
  const bool rhs_is_constant = get_constant(inst.GetRhs(), rhs_constant);
  const IR::IRValue* value = nullptr;
  std::string opcode;
  int64_t immediate = 0;

  switch (inst.GetOp()) {
    case IR::IRBinaryOp::ADD:
      if (rhs_is_constant && fits_signed_12(rhs_constant)) {
        value = inst.GetLhs().get();
        immediate = rhs_constant;
      } else if (lhs_is_constant && fits_signed_12(lhs_constant)) {
        value = inst.GetRhs().get();
        immediate = lhs_constant;
      } else {
        return false;
      }
      opcode = target_.Is64Bit() ? "addiw" : "addi";
      break;
    case IR::IRBinaryOp::SUB:
      if (!rhs_is_constant || !fits_signed_12(-int64_t{rhs_constant}))
        return false;
      value = inst.GetLhs().get();
      immediate = -int64_t{rhs_constant};
      opcode = target_.Is64Bit() ? "addiw" : "addi";
      break;
    case IR::IRBinaryOp::AND:
    case IR::IRBinaryOp::OR:
    case IR::IRBinaryOp::XOR:
      if (rhs_is_constant && fits_signed_12(rhs_constant)) {
        value = inst.GetLhs().get();
        immediate = rhs_constant;
      } else if (lhs_is_constant && fits_signed_12(lhs_constant)) {
        value = inst.GetRhs().get();
        immediate = lhs_constant;
      } else {
        return false;
      }
      opcode = inst.GetOp() == IR::IRBinaryOp::AND
                   ? "andi"
                   : (inst.GetOp() == IR::IRBinaryOp::OR ? "ori" : "xori");
      break;
    case IR::IRBinaryOp::SHL:
    case IR::IRBinaryOp::SHR:
    case IR::IRBinaryOp::SAR:
      if (!rhs_is_constant) return false;
      value = inst.GetLhs().get();
      immediate = static_cast<uint32_t>(rhs_constant) & 31U;
      if (inst.GetOp() == IR::IRBinaryOp::SHL)
        opcode = target_.Is64Bit() ? "slliw" : "slli";
      else if (inst.GetOp() == IR::IRBinaryOp::SHR)
        opcode = target_.Is64Bit() ? "srliw" : "srli";
      else
        opcode = target_.Is64Bit() ? "sraiw" : "srai";
      break;
    default:
      return false;
  }

  std::string destination = "t5";
  bool spilled = true;
  const auto assigned = reg_map_.find(inst.GetOperandName());
  if (assigned != reg_map_.end() && !assigned->second.empty() &&
      assigned->second.front() != 'f') {
    destination = assigned->second;
    spilled = false;
  }
  LoadValueToReg(value, destination);
  emitter_.EmitInst(opcode, destination, destination,
                    std::to_string(immediate));
  if (spilled) StoreRegToVal(&inst, destination);
  return true;
}

// Binary
void Back::RiscvGenerator::Visit(const IR::IRBinary& inst) {
  if (inst.GetOperandType() == IR::IRType::IR_FLOAT) {
    std::string name = inst.GetOperandName();
    auto ri = reg_map_.find(name);
    bool spilled = true;
    std::string dest_reg = "ft5";
    if (ri != reg_map_.end() && !ri->second.empty() && ri->second[0] == 'f') {
      dest_reg = ri->second; spilled = false;
    }
    std::string rhs_reg = "ft6";
    LoadValueToFReg(inst.GetRhs().get(), "ft6");
    LoadValueToFReg(inst.GetLhs().get(), dest_reg);
    std::string int_dest = "t5";
    if (ri != reg_map_.end() && !ri->second.empty()) int_dest = ri->second;

    switch (inst.GetOp()) {
      case IR::IRBinaryOp::ADD: emitter_.EmitInst("fadd.s", dest_reg, dest_reg, rhs_reg); break;
      case IR::IRBinaryOp::SUB: emitter_.EmitInst("fsub.s", dest_reg, dest_reg, rhs_reg); break;
      case IR::IRBinaryOp::MUL: emitter_.EmitInst("fmul.s", dest_reg, dest_reg, rhs_reg); break;
      case IR::IRBinaryOp::DIV: emitter_.EmitInst("fdiv.s", dest_reg, dest_reg, rhs_reg); break;
      case IR::IRBinaryOp::EQ:  emitter_.EmitInst("feq.s",  int_dest, dest_reg, rhs_reg); break;
      case IR::IRBinaryOp::NE:
        emitter_.EmitInst("feq.s", int_dest, dest_reg, rhs_reg);
        emitter_.EmitInst("xori",  int_dest, int_dest, "1"); break;
      case IR::IRBinaryOp::LT:  emitter_.EmitInst("flt.s", int_dest, dest_reg, rhs_reg); break;
      case IR::IRBinaryOp::LE:  emitter_.EmitInst("fle.s", int_dest, dest_reg, rhs_reg); break;
      case IR::IRBinaryOp::GT:  emitter_.EmitInst("flt.s", int_dest, rhs_reg,  dest_reg); break;
      case IR::IRBinaryOp::GE:  emitter_.EmitInst("fle.s", int_dest, rhs_reg,  dest_reg); break;
      default: assert(false && "Unsupported float binary op");
    }
    if (inst.GetType() == IR::IRType::IR_FLOAT && spilled) StoreFRegToVal(&inst, dest_reg);
    else if (inst.GetType() != IR::IRType::IR_FLOAT) {
      if (ri == reg_map_.end() || ri->second.empty()) StoreRegToVal(&inst, int_dest);
    }
    return;
  }

  if (TryEmitMulStrengthReduction(inst)) return;
  if (TryEmitSignedPowerOfTwoDivRem(inst)) return;
  if (TryEmitSignedConstantDivRem(inst)) return;
  if (TryEmitIntegerImmediate(inst)) return;

  std::string name = inst.GetOperandName();
  auto ri = reg_map_.find(name);
  bool spilled = true;
  std::string dest_reg = "t5";
  if (ri != reg_map_.end() && !ri->second.empty() && ri->second[0] != 'f') {
    dest_reg = ri->second; spilled = false;
  }

  std::string rhs_reg = "t6";
  auto rri = reg_map_.find(inst.GetRhs()->GetOperandName());
  if (rri != reg_map_.end() && !rri->second.empty()) rhs_reg = rri->second;
  if (rhs_reg == dest_reg) rhs_reg = "t6";

  LoadValueToReg(inst.GetRhs().get(), rhs_reg);
  LoadValueToReg(inst.GetLhs().get(), dest_reg);

  const char* add_op = target_.Is64Bit() ? "addw" : "add";
  const char* sub_op = target_.Is64Bit() ? "subw" : "sub";
  const char* mul_op = target_.Is64Bit() ? "mulw" : "mul";
  const char* div_op = target_.Is64Bit() ? "divw" : "div";
  const char* rem_op = target_.Is64Bit() ? "remw" : "rem";

  switch (inst.GetOp()) {
    case IR::IRBinaryOp::ADD: emitter_.EmitInst(add_op, dest_reg, dest_reg, rhs_reg); break;
    case IR::IRBinaryOp::SUB: emitter_.EmitInst(sub_op, dest_reg, dest_reg, rhs_reg); break;
    case IR::IRBinaryOp::MUL: emitter_.EmitInst(mul_op, dest_reg, dest_reg, rhs_reg); break;
    case IR::IRBinaryOp::DIV: emitter_.EmitInst(div_op, dest_reg, dest_reg, rhs_reg); break;
    case IR::IRBinaryOp::MOD: emitter_.EmitInst(rem_op, dest_reg, dest_reg, rhs_reg); break;
    case IR::IRBinaryOp::EQ:
      emitter_.EmitInst("sub",  dest_reg, dest_reg, rhs_reg);
      emitter_.EmitInst("seqz", dest_reg, dest_reg); break;
    case IR::IRBinaryOp::NE:
      emitter_.EmitInst("sub",  dest_reg, dest_reg, rhs_reg);
      emitter_.EmitInst("snez", dest_reg, dest_reg); break;
    case IR::IRBinaryOp::LT: emitter_.EmitInst("slt", dest_reg, dest_reg, rhs_reg); break;
    case IR::IRBinaryOp::GT: emitter_.EmitInst("slt", dest_reg, rhs_reg,  dest_reg); break;
    case IR::IRBinaryOp::LE:
      emitter_.EmitInst("slt",  dest_reg, rhs_reg, dest_reg);
      emitter_.EmitInst("xori", dest_reg, dest_reg, "1"); break;
    case IR::IRBinaryOp::GE:
      emitter_.EmitInst("slt",  dest_reg, dest_reg, rhs_reg);
      emitter_.EmitInst("xori", dest_reg, dest_reg, "1"); break;
    case IR::IRBinaryOp::AND: emitter_.EmitInst("and", dest_reg, dest_reg, rhs_reg); break;
    case IR::IRBinaryOp::OR:  emitter_.EmitInst("or",  dest_reg, dest_reg, rhs_reg); break;
    case IR::IRBinaryOp::XOR: emitter_.EmitInst("xor", dest_reg, dest_reg, rhs_reg); break;
    case IR::IRBinaryOp::SHL: emitter_.EmitInst(target_.Is64Bit() ? "sllw" : "sll", dest_reg, dest_reg, rhs_reg); break;
    case IR::IRBinaryOp::SHR: emitter_.EmitInst(target_.Is64Bit() ? "srlw" : "srl", dest_reg, dest_reg, rhs_reg); break;
    case IR::IRBinaryOp::SAR: emitter_.EmitInst(target_.Is64Bit() ? "sraw" : "sra", dest_reg, dest_reg, rhs_reg); break;
    default: assert(false && "Unsupported binary op");
  }
  if (spilled) StoreRegToVal(&inst, dest_reg);
}

// Cast / Alloc / Store / Load / Jump / Branch / Call / GEP / GetPtr
void Back::RiscvGenerator::Visit(const IR::IRCast& inst) {
  if (inst.GetOp() == IR::IRCastOp::INT_TO_FLOAT) {
    LoadValueToReg(inst.GetValue().get(), "t5");
    emitter_.EmitInst("fcvt.s.w", "ft5", "t5");
    StoreFRegToVal(&inst, "ft5");
  } else {
    LoadValueToFReg(inst.GetValue().get(), "ft5");
    emitter_.EmitInst("fcvt.w.s", "t5", "ft5", "rtz");
    StoreRegToVal(&inst, "t5");
  }
}

void Back::RiscvGenerator::Visit(const IR::IRAlloc& /*inst*/) {}

void Back::RiscvGenerator::Visit(const IR::IRStore& inst) {
  const auto* ptr_raw  = inst.GetPtr().get();
  std::string ptr_name = ptr_raw->GetOperandName();

  if (local_allocs_.count(ptr_name)) {
    auto ri = reg_map_.find(ptr_name);
    if (ri != reg_map_.end() && !ri->second.empty()) {
      if (ri->second[0] == 'f') LoadValueToFReg(inst.GetVal().get(), ri->second);
      else                       LoadValueToReg (inst.GetVal().get(), ri->second);
      return;
    }
  }

  if (inst.GetVal()->GetType() == IR::IRType::IR_FLOAT) {
    LoadValueToFReg(inst.GetVal().get(), "ft5");
    if (IR::isa<IR::IRGlobalAlloc>(ptr_raw)) {
      emitter_.EmitInst("la", "t6", StripAt(ptr_name));
      emitter_.Out() << "  fsw ft5, 0(t6)\n";
    } else if (local_allocs_.count(ptr_name)) {
      emitter_.EmitFsw("ft5", val_stack_map_.at(ptr_name));
    } else {
      LoadValueToReg(inst.GetPtr().get(), "t6");
      emitter_.Out() << "  fsw ft5, 0(t6)\n";
    }
    return;
  }

  LoadValueToReg(inst.GetVal().get(), "t5");
  if (IR::isa<IR::IRGlobalAlloc>(ptr_raw)) {
    emitter_.EmitInst("la", "t6", StripAt(ptr_name));
    if (GetTypeSize(inst.GetVal()->GetType()) == 8) emitter_.Out() << "  sd t5, 0(t6)\n";
    else                                            emitter_.Out() << "  sw t5, 0(t6)\n";
  } else if (local_allocs_.count(ptr_name)) {
    int off = val_stack_map_.at(ptr_name);
    if (GetTypeSize(inst.GetVal()->GetType()) == 8) emitter_.EmitSd("t5", off);
    else                                            emitter_.EmitSw("t5", off);
  } else {
    LoadValueToReg(inst.GetPtr().get(), "t6");
    if (GetTypeSize(inst.GetVal()->GetType()) == 8) emitter_.Out() << "  sd t5, 0(t6)\n";
    else                                            emitter_.Out() << "  sw t5, 0(t6)\n";
  }
}

void Back::RiscvGenerator::Visit(const IR::IRLoad& inst) {
  const auto* ptr_raw  = inst.GetPtr().get();
  std::string ptr_name = ptr_raw->GetOperandName();

  if (local_allocs_.count(ptr_name)) {
    auto ri = reg_map_.find(ptr_name);
    if (ri != reg_map_.end() && !ri->second.empty()) {
      if (ri->second[0] == 'f') StoreFRegToVal(&inst, ri->second);
      else                       StoreRegToVal (&inst, ri->second);
      return;
    }
  }

  if (inst.GetType() == IR::IRType::IR_FLOAT) {
    if (IR::isa<IR::IRGlobalAlloc>(ptr_raw)) {
      emitter_.EmitInst("la", "t5", StripAt(ptr_name));
      emitter_.Out() << "  flw ft5, 0(t5)\n";
    } else if (local_allocs_.count(ptr_name)) {
      emitter_.EmitFlw("ft5", val_stack_map_.at(ptr_name));
    } else {
      LoadValueToReg(inst.GetPtr().get(), "t6");
      emitter_.Out() << "  flw ft5, 0(t6)\n";
    }
    StoreFRegToVal(&inst, "ft5");
    return;
  }

  if (IR::isa<IR::IRGlobalAlloc>(ptr_raw)) {
    emitter_.EmitInst("la", "t5", StripAt(ptr_name));
    if (GetTypeSize(inst.GetType()) == 8) emitter_.Out() << "  ld t5, 0(t5)\n";
    else                                  emitter_.Out() << "  lw t5, 0(t5)\n";
  } else if (local_allocs_.count(ptr_name)) {
    int off = val_stack_map_.at(ptr_name);
    if (GetTypeSize(inst.GetType()) == 8) emitter_.EmitLd("t5", off);
    else                                  emitter_.EmitLw("t5", off);
  } else {
    LoadValueToReg(inst.GetPtr().get(), "t6");
    if (GetTypeSize(inst.GetType()) == 8) emitter_.Out() << "  ld t5, 0(t6)\n";
    else                                  emitter_.Out() << "  lw t5, 0(t6)\n";
  }
  StoreRegToVal(&inst, "t5");
}

void Back::RiscvGenerator::Visit(const IR::IRJump& inst) {
  auto* target = IR::dyn_cast<IR::IRBasicBlock>(inst.GetTarget().get());
  assert(target && "jump target is not a basic block");
  EmitEdgeCopies(inst.GetArgs(), *target);
  emitter_.EmitInst("j", GetBlockAsmLabel(inst.GetTarget()->GetOperandName()));
}
void Back::RiscvGenerator::Visit(const IR::IRBranch& inst) {
  auto* then_target = IR::dyn_cast<IR::IRBasicBlock>(inst.GetThenTarget().get());
  auto* else_target = IR::dyn_cast<IR::IRBasicBlock>(inst.GetElseTarget().get());
  assert(then_target && else_target && "branch targets must be basic blocks");
  LoadValueToReg(inst.GetCond().get(), "t5");
  const auto then_args = inst.GetThenArgs();
  const auto else_args = inst.GetElseArgs();
  if (then_args.empty() && else_args.empty()) {
    emitter_.EmitInst("bnez", "t5", GetBlockAsmLabel(inst.GetThenTarget()->GetOperandName()));
    emitter_.EmitInst("j", GetBlockAsmLabel(inst.GetElseTarget()->GetOperandName()));
    return;
  }

  const std::string then_copy = current_func_->name + "_edge_then_" +
                                std::to_string(edge_label_counter_++);
  emitter_.EmitInst("bnez", "t5", then_copy);
  EmitEdgeCopies(else_args, *else_target);
  emitter_.EmitInst("j", GetBlockAsmLabel(inst.GetElseTarget()->GetOperandName()));
  emitter_.EmitLabel(then_copy);
  EmitEdgeCopies(then_args, *then_target);
  emitter_.EmitInst("j", GetBlockAsmLabel(inst.GetThenTarget()->GetOperandName()));
}
void Back::RiscvGenerator::EmitEdgeCopies(
    const std::vector<std::shared_ptr<IR::IRValue>>& args,
    const IR::IRBasicBlock& target) {
  assert(args.size() == target.params.size() && "block argument arity mismatch");
  std::vector<CopyDestination> destinations;
  destinations.reserve(target.params.size());
  for (const auto& parameter : target.params) {
    CopyDestination destination;
    destination.type = parameter->GetType();
    destination.value_name = parameter->GetOperandName();
    const auto register_location = reg_map_.find(destination.value_name);
    if (register_location != reg_map_.end() &&
        !register_location->second.empty()) {
      destination.kind = CopyDestination::Kind::REGISTER;
      destination.reg = register_location->second;
    } else {
      const auto stack_location = val_stack_map_.find(destination.value_name);
      assert(stack_location != val_stack_map_.end() &&
             "block parameter has no physical destination");
      destination.kind = CopyDestination::Kind::STACK;
      destination.stack_offset = stack_location->second;
    }
    destinations.push_back(std::move(destination));
  }

  // 块参数与调用实参具有相同的并行赋值语义。统一调度器只在真正存在
  // 栈槽覆盖环时才使用 staging buffer；寄存器同位置、栈槽同位置和
  // param <- param 自保持边都可以直接省略，避免回边产生多余访存。
  EmitParallelValueCopies(args, destinations, param_overflow_size_);
}

void Back::RiscvGenerator::Visit(const IR::IRCall& inst) {
  const auto& args = inst.GetArgs();
  std::vector<IR::IRType> arg_types;
  for (const auto& a : args) arg_types.push_back(a->GetType());
  const auto locs = calling_convention_.AssignArgumentLocations(arg_types);
  std::vector<CopyDestination> destinations;
  destinations.reserve(args.size());
  for (size_t i = 0; i < args.size(); ++i) {
    CopyDestination destination;
    destination.type = args[i]->GetType();
    const auto& loc = locs[i];
    if (loc.kind == ArgumentLocation::Kind::FLOAT_REGISTER) {
      destination.kind = CopyDestination::Kind::REGISTER;
      destination.reg = "fa" + std::to_string(loc.index);
    } else if (loc.kind == ArgumentLocation::Kind::INTEGER_REGISTER) {
      destination.kind = CopyDestination::Kind::REGISTER;
      destination.reg = "a" + std::to_string(loc.index);
    } else {
      destination.kind = CopyDestination::Kind::STACK;
      destination.stack_offset = GetRegSize() * loc.index;
    }
    destinations.push_back(std::move(destination));
  }

  EmitParallelValueCopies(args, destinations, param_overflow_size_);

  emitter_.EmitInst("call", StripAt(inst.GetFunc()->GetOperandName()));

  if (inst.HasReturnValue()) {
    if (inst.GetType() == IR::IRType::IR_FLOAT) StoreFRegToVal(&inst, "fa0");
    else                                         StoreRegToVal (&inst, "a0");
  }
}

void Back::RiscvGenerator::EmitIndexScale(int elem_size) {
  if (elem_size == 1) return;
  unsigned shift = 0;
  if (GetOptimizationLevel() == OptimizationLevel::O1 && elem_size > 0 &&
      GetPowerOfTwoShift(static_cast<uint64_t>(elem_size), shift))
    emitter_.EmitInst("slli", "t6", "t6", std::to_string(shift));
  else {
    emitter_.EmitInst("li",  "t4", std::to_string(elem_size));
    emitter_.EmitInst("mul", "t6", "t6", "t4");
  }
}

bool Back::RiscvGenerator::TryEmitConstantPointerOffset(
    const IR::IRValue* base, const IR::IRValue* index, int element_size,
    const IR::IRValue* result, bool base_is_address) {
  if (GetOptimizationLevel() != OptimizationLevel::O1 || !base || !index ||
      !result || element_size <= 0)
    return false;
  const auto* constant = IR::dyn_cast<IR::IRConstant>(index);
  if (!constant || constant->GetType() != IR::IRType::IR_INT32)
    return false;

  const std::int64_t offset =
      static_cast<std::int64_t>(std::get<std::int32_t>(constant->GetValue())) *
      static_cast<std::int64_t>(element_size);
  if (base_is_address)
    LoadAddressToReg(base, "t5");
  else
    LoadValueToReg(base, "t5");

  if (offset >= -2048 && offset <= 2047) {
    if (offset != 0)
      emitter_.EmitInst("addi", "t5", "t5", std::to_string(offset));
  } else {
    emitter_.EmitInst("li", "t6", std::to_string(offset));
    emitter_.EmitInst("add", "t5", "t5", "t6");
  }
  StoreRegToVal(result, "t5");
  return true;
}

void Back::RiscvGenerator::Visit(const IR::IRGetElemPtr& inst) {
  if (TryEmitConstantPointerOffset(inst.GetArr().get(), inst.GetIndex().get(),
                                   inst.GetElemSize(), &inst, true))
    return;
  LoadValueToReg  (inst.GetIndex().get(), "t6");
  // Loading t6 from a large stack offset uses t5 as an address scratch.
  // Materialize the base afterwards so it cannot be clobbered.
  LoadAddressToReg(inst.GetArr().get(),   "t5");
  EmitIndexScale(inst.GetElemSize());
  emitter_.EmitInst("add", "t5", "t5", "t6");
  StoreRegToVal(&inst, "t5");
}

void Back::RiscvGenerator::Visit(const IR::IRGetPtr& inst) {
  if (TryEmitConstantPointerOffset(inst.GetPtr().get(), inst.GetIndex().get(),
                                   inst.GetElemSize(), &inst, false))
    return;
  LoadValueToReg(inst.GetIndex().get(), "t6");
  LoadValueToReg(inst.GetPtr().get(),   "t5");
  EmitIndexScale(inst.GetElemSize());
  emitter_.EmitInst("add", "t5", "t5", "t6");
  StoreRegToVal(&inst, "t5");
}

void Back::RiscvGenerator::Visit(const IR::IRZeroInit& inst) {
  int size = inst.GetByteSize();
  if (size <= 0) return;

  LoadAddressToReg(inst.GetPtr().get(), "t5");

  if (size <= 32) {
    // Loop unrolling for small sizes to maximize performance
    for (int offset = 0; offset < size; offset += 4) {
      emitter_.EmitSw("zero", offset, "t5");
    }
  } else {
    // Large array: emit an assembly loop
    emitter_.EmitInst("li", "t4", std::to_string(size));
    emitter_.EmitInst("add", "t6", "t5", "t4");

    std::string loop_label = "zero_init_loop_" + std::to_string(zero_init_label_counter_);
    std::string end_label = "zero_init_end_" + std::to_string(zero_init_label_counter_++);

    emitter_.EmitLabel(loop_label);
    emitter_.EmitInst("bge", "t5", "t6", end_label);
    emitter_.EmitSw("zero", 0, "t5");
    emitter_.EmitInst("addi", "t5", "t5", "4");
    emitter_.EmitInst("j", loop_label);
    emitter_.EmitLabel(end_label);
  }
}

// Value load/store helpers
void Back::RiscvGenerator::LoadValueToReg(const IR::IRValue* val, const std::string& reg) {
  if (IR::isa<IR::IRConstant>(val)) {
    const auto* c = IR::cast<IR::IRConstant>(val);
    if (c->GetType() == IR::IRType::IR_INT32) {
      int32_t iv = std::get<int32_t>(c->GetValue());
      if (iv == 0) emitter_.EmitInst("mv", reg, "x0");
      else         emitter_.EmitInst("li", reg, std::to_string(iv));
    } else {
      union { float f; uint32_t u; } conv; conv.f = std::get<float>(c->GetValue());
      if (!reg.empty() && reg[0] == 'f') {
        emitter_.EmitInst("li",      "t5", std::to_string(conv.u));
        emitter_.EmitInst("fmv.w.x", reg,  "t5");
      } else {
        emitter_.EmitInst("li", reg, std::to_string(conv.u));
      }
    }
    return;
  }
  if (IR::isa<IR::IRGlobalAlloc>(val)) {
    emitter_.EmitInst("la", reg, StripAt(val->GetOperandName()));
    if (GetTypeSize(val->GetType()) == 8) emitter_.Out() << "  ld " << reg << ", 0(" << reg << ")\n";
    else                                  emitter_.Out() << "  lw " << reg << ", 0(" << reg << ")\n";
    return;
  }
  std::string name = val->GetOperandName();
  auto ri = reg_map_.find(name);
  if (ri != reg_map_.end() && !ri->second.empty()) {
    if (reg != ri->second) {
      bool dr = (!reg.empty() && reg[0] == 'f'), sr = (ri->second[0] == 'f');
      if (dr && sr)       emitter_.EmitInst("fmv.s",   reg, ri->second);
      else if (dr && !sr) emitter_.EmitInst("fmv.w.x", reg, ri->second);
      else if (!dr && sr) emitter_.EmitInst("fmv.x.w", reg, ri->second);
      else                emitter_.EmitInst("mv",       reg, ri->second);
    }
  } else {
    auto it = val_stack_map_.find(name);
    assert(it != val_stack_map_.end());
    if (GetTypeSize(val->GetType()) == 8) emitter_.EmitLd(reg, it->second);
    else                                  emitter_.EmitLw(reg, it->second);
  }
}

void Back::RiscvGenerator::StoreRegToVal(const IR::IRValue* val, const std::string& reg) {
  std::string name = val->GetOperandName();
  auto ri = reg_map_.find(name);
  if (ri != reg_map_.end() && !ri->second.empty()) {
    if (reg != ri->second) {
      bool dr = (ri->second[0] == 'f'), sr = (!reg.empty() && reg[0] == 'f');
      if (dr && sr)       emitter_.EmitInst("fmv.s",   ri->second, reg);
      else if (dr && !sr) emitter_.EmitInst("fmv.w.x", ri->second, reg);
      else if (!dr && sr) emitter_.EmitInst("fmv.x.w", ri->second, reg);
      else                emitter_.EmitInst("mv",       ri->second, reg);
    }
  } else {
    auto it = val_stack_map_.find(name);
    assert(it != val_stack_map_.end());
    if (GetTypeSize(val->GetType()) == 8) emitter_.EmitSd(reg, it->second);
    else                                  emitter_.EmitSw(reg, it->second);
  }
}

void Back::RiscvGenerator::LoadValueToFReg(const IR::IRValue* val, const std::string& reg) {
  if (IR::isa<IR::IRConstant>(val)) {
    const auto* c = IR::cast<IR::IRConstant>(val);
    if (c->GetType() == IR::IRType::IR_FLOAT) {
      float fv = std::get<float>(c->GetValue());
      uint32_t bits = 0; std::memcpy(&bits, &fv, sizeof(bits));
      emitter_.EmitInst("li",      "t5", std::to_string(bits));
      emitter_.EmitInst("fmv.w.x", reg,  "t5");
    } else {
      LoadValueToReg(val, "t5");
      emitter_.EmitInst("fcvt.s.w", reg, "t5");
    }
    return;
  }
  std::string name = val->GetOperandName();
  auto ri = reg_map_.find(name);
  if (ri != reg_map_.end() && !ri->second.empty()) {
    if (reg != ri->second)
      emitter_.EmitInst(ri->second[0] == 'f' ? "fmv.s" : "fmv.w.x", reg, ri->second);
  } else {
    auto it = val_stack_map_.find(name);
    assert(it != val_stack_map_.end());
    emitter_.EmitFlw(reg, it->second);
  }
}

void Back::RiscvGenerator::StoreFRegToVal(const IR::IRValue* val, const std::string& reg) {
  std::string name = val->GetOperandName();
  auto ri = reg_map_.find(name);
  if (ri != reg_map_.end() && !ri->second.empty()) {
    if (reg != ri->second)
      emitter_.EmitInst(ri->second[0] == 'f' ? "fmv.s" : "fmv.x.w", ri->second, reg);
  } else {
    auto it = val_stack_map_.find(name);
    assert(it != val_stack_map_.end());
    emitter_.EmitFsw(reg, it->second);
  }
}

void Back::RiscvGenerator::LoadAddressToReg(const IR::IRValue* val, const std::string& reg) {
  if (IR::isa<IR::IRAlloc>(val)) {
    const auto* al = IR::cast<IR::IRAlloc>(val);
    auto it = val_stack_map_.find(al->GetOperandName());
    assert(it != val_stack_map_.end());
    int off = it->second;
    if (off >= -2048 && off <= 2047) emitter_.EmitInst("addi", reg, "sp", std::to_string(off));
    else {
      emitter_.EmitInst("li",  reg, std::to_string(off));
      emitter_.EmitInst("add", reg, "sp", reg);
    }
  } else if (IR::isa<IR::IRGlobalAlloc>(val)) {
    emitter_.EmitInst("la", reg, StripAt(val->GetOperandName()));
  } else {
    std::string name = val->GetOperandName();
    auto ri = reg_map_.find(name);
    if (ri != reg_map_.end() && !ri->second.empty()) {
      if (reg != ri->second) emitter_.EmitInst("mv", reg, ri->second);
    } else {
      auto it = val_stack_map_.find(name);
      assert(it != val_stack_map_.end());
      if (target_.Is64Bit()) emitter_.EmitLd(reg, it->second);
      else           emitter_.EmitLw(reg, it->second);
    }
  }
}

void Back::RiscvGenerator::StoreRegToValByName(const std::string& name,
                                               const std::string& reg,
                                               IR::IRType type) {
  auto it = val_stack_map_.find(name);
  assert(it != val_stack_map_.end());
  if (!reg.empty() && reg[0] == 'f')  emitter_.EmitFsw(reg, it->second);
  else if (GetTypeSize(type) == 8)    emitter_.EmitSd (reg, it->second);
  else                                emitter_.EmitSw (reg, it->second);
}

void Back::RiscvGenerator::LoadValueToRegByName(const std::string& name,
                                                const std::string& reg,
                                                IR::IRType type) {
  auto it = val_stack_map_.find(name);
  assert(it != val_stack_map_.end());
  if (!reg.empty() && reg[0] == 'f')  emitter_.EmitFlw(reg, it->second);
  else if (GetTypeSize(type) == 8)    emitter_.EmitLd (reg, it->second);
  else                                emitter_.EmitLw (reg, it->second);
}
