#pragma once
#include "koopa.h"
#include <string>
#include <unordered_map>
#include <assert.h>
#include <iostream>

enum class RegKind {
  NORMAL,
  SPDIFFADD
};

struct RegObj {
  std::string reg_name;
  RegKind kind;

  RegObj(std::string reg_name, RegKind kind) : reg_name(reg_name), kind(kind) {}
  RegObj(std::string reg_name) : reg_name(reg_name), kind(RegKind::NORMAL) {}
  RegObj() : reg_name(""), kind(RegKind::NORMAL) {}
};

class RCBuilder {
private:
  int reg_counter = 0; // 临时寄存器计数器
  int stack_size = 0;
  int sp_diff = 0;
public:
  // 生成下一个可用的寄存器名字（例如 "a0", "a1"）
  std::string NewReg() {
    if (reg_counter < 7) {
      return "t" + std::to_string(reg_counter++);
    } else if (reg_counter < 15) {
      return "a" + std::to_string(reg_counter++ - 7);
    } else {
      reg_counter = 0;
      return "s0";
    }
  }
  // 重置寄存器计数器，一般讲用不到t3
  void ResetReg() {
    reg_counter = 0;
  }
  // 仅查看当前计数器值
  std::string CurrentReg() const {
    if (reg_counter < 8) {
      return "a" + std::to_string(reg_counter);
    } else if (reg_counter < 15) {
      return "t" + std::to_string(reg_counter - 8);
    } else {
      return "s0";
    }
  }
  // 每当开始解析一个新函数时清零计数器
  void Reset() {
    reg_counter = 0;
    stack_size = 0;
    sp_diff = 0;
  }
  void SetStackSize(int size) {
    stack_size = size;
  }
  int GetStackSize() const {
    return stack_size;
  }
  std::string NewAddress() {
    if (sp_diff + 4 <= stack_size) {
      int sp_diff_tmp = sp_diff;
      sp_diff += 4;
      return std::to_string(sp_diff_tmp) + "(sp)";
    } else {
      std::cerr << "Stack Overflow" << std::endl;
      return "OverStackError";
    }
  }
  // 根据kind返回RegName
  RegObj GetRegObj(RegKind kind) {
    if (kind == RegKind::SPDIFFADD) {
      return RegObj(NewAddress(), kind);
    } else if (kind == RegKind::NORMAL) {
      return RegObj(NewReg(), kind);
    } else {
      std::cerr << "Invalid RegKind" << std::endl;
      return RegObj("RegNameError");
    }
  }
};

class IRVisitor {
private:
  RCBuilder rc_builder;
  std::unordered_map<koopa_raw_value_t, RegObj> value_reg_map; // 存储指令对应的寄存器名字

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
  std::string Visit(const koopa_raw_integer_t &integer);
  void Visit(const koopa_raw_return_t &aggregate);
  void Visit(const koopa_raw_binary_t &binary, koopa_raw_value_t value);
  void Visit(const koopa_raw_store_t &store);
  void Visit(const koopa_raw_load_t &load, koopa_raw_value_t value);
  // 遍历函数的指令以得到分配的空间
  int VisitFuncIns(const koopa_raw_slice_t &slice);
  int VisitFuncIns(const koopa_raw_value_t &value);
  RegObj GetReg(koopa_raw_value_t value) {
    /** 
     * 这里要根据value对应的RegName的类型来决定如何获取寄存器
     * 如果是普通寄存器就正常返回
     * 如果是地址寄存器就先加载
     * */      
    if (value_reg_map.find(value) != value_reg_map.end()) {
      return value_reg_map[value];
    }
    if (value->kind.tag == KOOPA_RVT_INTEGER) {
      int val = value->kind.data.integer.value;
      if (val == 0) {
        return RegObj("x0");
      }
      assert(val >= -2147483648 && val <= 2147483647);
      // std::cout << "GetRegCall" << std::endl;
      return RegObj(Visit(value->kind.data.integer));
    }
    return RegObj("RegNameError");
  }
  // 目前仅用于建立value到栈空间的映射
  void SetReg(koopa_raw_value_t value, RegObj reg) {
    assert(reg.kind == RegKind::SPDIFFADD);
    value_reg_map[value] = reg;
  }
};