#pragma once
#include "koopa.h"
#include "string"
#include "unordered_map"
using namespace std;

class RCBuilder {
private:
  int reg_counter = 0; // 临时寄存器计数器

public:
  // 生成下一个可用的寄存器名字（例如 "a0", "a1"）
  std::string NewReg() {
    if (reg_counter < 8) {
      return "a" + std::to_string(reg_counter++);
    } else {
      return "t" + std::to_string(reg_counter++ - 8);
    }
  }
  // 仅查看当前计数器值
  std::string CurrentReg() const {
    if (reg_counter < 8) {
      return "a" + std::to_string(reg_counter);
    } else {
      return "t" + std::to_string(reg_counter - 8);
    }
  }
  // 每当开始解析一个新函数时清零计数器
  void Reset() {
  reg_counter = 0;
  }
};

class IRVisitor {
private:
  RCBuilder rc_builder;
  unordered_map<koopa_raw_value_t, string> value_reg_map; // 存储指令对应的寄存器名字
public:
  void HandleRawProgram(const koopa_raw_program_t &program) {
    // 访问 raw program
    Visit(program);
  }
private:
  void Visit(const koopa_raw_program_t &program);
  void Visit(const koopa_raw_slice_t &slice);
  void Visit(const koopa_raw_function_t &func);
  void Visit(const koopa_raw_basic_block_t &bb);
  void Visit(const koopa_raw_value_t &value);
  int Visit(const koopa_raw_integer_t &integer);
  void Visit(const koopa_raw_return_t &aggregate);
  void Visit(const koopa_raw_binary_t &binary);
};