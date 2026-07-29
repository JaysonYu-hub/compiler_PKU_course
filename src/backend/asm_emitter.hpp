#pragma once
// AsmEmitter: low-level RISC-V assembly output.

#include <cassert>
#include <iostream>
#include <string>

namespace Back {

class AsmEmitter {
public:
  explicit AsmEmitter(std::ostream& out = std::cout) : out_(out) {}

  // ---------- directives / labels ----------
  void EmitDirective(const std::string& directive) {
    out_ << directive << "\n";
  }
  void EmitLabel(const std::string& label) {
    out_ << label << ":\n";
  }

  // ---------- generic instruction helpers ----------
  void EmitInst(const std::string& op) {
    out_ << "  " << op << "\n";
  }
  template <typename... Args>
  void EmitInst(const std::string& op, Args&&... args) {
    out_ << "  " << op;
    bool first = true;
    ((out_ << (first ? (first = false, " ") : ", ") << args), ...);
    (void)first;
    out_ << "\n";
  }

  // ---------- memory instructions (handle large offsets) ----------
  void EmitLw(const std::string& reg, int offset, const std::string& base = "sp") {
    assert(base != "sp" || offset % 4 == 0);
    EmitMem32("lw", reg, offset, base);
  }
  void EmitSw(const std::string& reg, int offset, const std::string& base = "sp") {
    assert(base != "sp" || offset % 4 == 0);
    EmitMemStore32("sw", reg, offset, base);
  }
  void EmitLd(const std::string& reg, int offset, const std::string& base = "sp") {
    assert(base != "sp" || offset % 8 == 0);
    EmitMem64("ld", reg, offset, base);
  }
  void EmitSd(const std::string& reg, int offset, const std::string& base = "sp") {
    assert(base != "sp" || offset % 8 == 0);
    EmitMemStore64("sd", reg, offset, base);
  }
  void EmitFlw(const std::string& reg, int offset, const std::string& base = "sp") {
    assert(base != "sp" || offset % 4 == 0);
    EmitMemFloat("flw", reg, offset, base);
  }
  void EmitFsw(const std::string& reg, int offset, const std::string& base = "sp") {
    assert(base != "sp" || offset % 4 == 0);
    EmitMemFloatStore("fsw", reg, offset, base);
  }

  // ---------- raw ostream access (for legacy callers) ----------
  std::ostream& Out() { return out_; }

private:
  std::ostream& out_;

  static bool InImm12Range(int off) { return off >= -2048 && off <= 2047; }

  void EmitMem32(const std::string& op, const std::string& reg,
                 int offset, const std::string& base) {
    if (InImm12Range(offset)) {
      out_ << "  " << op << " " << reg << ", " << offset << "(" << base << ")\n";
    } else if (reg != "t6") {
      out_ << "  li " << reg << ", " << offset << "\n";
      out_ << "  add " << reg << ", " << base << ", " << reg << "\n";
      out_ << "  " << op << " " << reg << ", 0(" << reg << ")\n";
    } else {
      // 
      out_ << "  li t5, " << offset << "\n";
      out_ << "  add t5, " << base << ", t5\n";
      out_ << "  " << op << " t6, 0(t5)\n";
    }
  }
  void EmitMemStore32(const std::string& op, const std::string& reg,
                      int offset, const std::string& base) {
    if (InImm12Range(offset)) {
      out_ << "  " << op << " " << reg << ", " << offset << "(" << base << ")\n";
    } else {
      std::string tmp = (reg == "t6") ? "t5" : "t6";
      out_ << "  li " << tmp << ", " << offset << "\n";
      out_ << "  add " << tmp << ", " << base << ", " << tmp << "\n";
      out_ << "  " << op << " " << reg << ", 0(" << tmp << ")\n";
    }
  }
  void EmitMem64(const std::string& op, const std::string& reg,
                 int offset, const std::string& base) {
    EmitMem32(op, reg, offset, base);
  }
  void EmitMemStore64(const std::string& op, const std::string& reg,
                      int offset, const std::string& base) {
    EmitMemStore32(op, reg, offset, base);
  }
  void EmitMemFloat(const std::string& op, const std::string& reg,
                    int offset, const std::string& base) {
    if (InImm12Range(offset)) {
      out_ << "  " << op << " " << reg << ", " << offset << "(" << base << ")\n";
    } else {
      out_ << "  li t6, " << offset << "\n";
      out_ << "  add t6, " << base << ", t6\n";
      out_ << "  " << op << " " << reg << ", 0(t6)\n";
    }
  }
  void EmitMemFloatStore(const std::string& op, const std::string& reg,
                         int offset, const std::string& base) {
    EmitMemFloat(op, reg, offset, base);
  }
};

}
