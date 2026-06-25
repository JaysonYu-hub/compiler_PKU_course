#pragma once
#include "koopa.h"
#include <string>
#include <unordered_map>
#include <assert.h>
#include <iostream>

namespace IR {
  struct CallStackInfo { 
    int stack_size = 0;
    int local_var_sp_top = 0;
    int local_var_sp = 0;
    // 用于
    int param_sp_bottom = -1;

    void Reset() {
      stack_size = 0;
      local_var_sp_top = 0;
      local_var_sp = 0;
    }
    bool IfCall() {
      return local_var_sp_top != stack_size;
    }
  };

  struct FuncInformation {
    int return_ins_number = 0;
    bool if_call = false;
    int max_param_num = 8;
    void PrintFuncInfo() {
      std::cout << "return_ins_number : " << std::to_string(return_ins_number) << "\n";
      std::cout << "if_call : " << std::to_string(if_call) << "\n";
      std::cout << "max_param_num : " << std::to_string(max_param_num) << "\n";
    }
  };
  
  enum ObjKind {
    TEMP_REG,
    FUNC_PARAM_REG,
    RA_ADD,
    LOCALVAR_ADD,
    FUNC_PARAM_ADD
  };

  struct RegObj {
    std::string reg_name;
    ObjKind kind;
    RegObj(std::string reg_name, ObjKind kind) : reg_name(reg_name), kind(kind) {}
    RegObj(std::string reg_name) : reg_name(reg_name), kind(ObjKind::TEMP_REG) {}
    RegObj() : reg_name(""), kind(ObjKind::TEMP_REG) {}
  };

  class TempRegBuilder {
  private:
    int reg_counter = 0; // 临时寄存器计数器
  public:
    // 生成下一个可用的寄存器名字（例如 "t0", "t1"）
    RegObj NewReg() {
      if (reg_counter < 7) {
        return RegObj("t" + std::to_string(reg_counter++), ObjKind::TEMP_REG);
      } else {
        return RegObj("UseToManyReg", ObjKind::TEMP_REG);
      }
    }
    // 每当开始解析一个新函数时清零计数器
    void Reset() {
      reg_counter = 0;
    }
  };
  
  class Context {
    public:
    TempRegBuilder t_reg_builder;
    CallStackInfo call_stack_info;
    std::unordered_map<koopa_raw_value_t, RegObj> value_reg_map; // 存储指令对应的寄存器名字
    RegObj GetAddress(ObjKind kind) {
      switch (kind) {
        case ObjKind::RA_ADD: {
          if (call_stack_info.stack_size - 4 != call_stack_info.local_var_sp_top) {
            std::cerr << "Error: rd sp overflow" << std::endl;
            return RegObj("OverStackError", kind);
          }
          return RegObj(std::to_string(call_stack_info.stack_size - 4) + "(sp)", ObjKind::RA_ADD);
          break;
        }
        case ObjKind::LOCALVAR_ADD: {
          if (call_stack_info.local_var_sp - 4 >= 0) {
            call_stack_info.local_var_sp -= 4;
            return RegObj(std::to_string(call_stack_info.local_var_sp) + "(sp)", kind);
          } else {
            std::cerr << "Error: local var sp overflow" << std::endl;
            std::cout << "local_var_sp : " << std::to_string(call_stack_info.local_var_sp) << std::endl;
            return RegObj("OverStackError", kind);
          }
          break;
        }
        default: { 
          assert(false);
        } 
      }
    }
    // 用于调用函数时，把多余参数放到栈上, x表示参数index(从0开始)
    // 当x大于8时，从大到小调用
    RegObj GetParamRegOrAddForSet(int x) {
      if (x <= 7) {
        return RegObj("a" + std::to_string(x), ObjKind::FUNC_PARAM_REG);
      } else {
        // 此时没有call_stack_info
        // 把param_sp_bottom放到参数栈的最底部,只有第一次调用时触发
        if (call_stack_info.param_sp_bottom == -1) {
          call_stack_info.param_sp_bottom = call_stack_info.local_var_sp - 4 * (x - 7);
          assert(call_stack_info.param_sp_bottom != -1);
        }
        if (call_stack_info.param_sp_bottom + 4 <= call_stack_info.local_var_sp) {
          int temp = call_stack_info.param_sp_bottom;
          call_stack_info.param_sp_bottom += 4;
          return RegObj(std::to_string(temp) + "(sp)", ObjKind::FUNC_PARAM_ADD);
        } else {
          std::cerr << "Error: param sp overflow" << std::endl;
          std::cout << "local_var_sp : " << std::to_string(call_stack_info.local_var_sp) << std::endl;
          std::cout << "param_sp_bottom : " << std::to_string(call_stack_info.param_sp_bottom) << std::endl;
          return RegObj("OverStackError", ObjKind::FUNC_PARAM_ADD);
        }
        if (call_stack_info.param_sp_bottom == call_stack_info.local_var_sp) {

        }
      }
    }
    // 用于函数定义，可以得到所需9,10参数的栈位置
    RegObj GetParamRegOrAddForGet(int x) {
      // 此时有call_stack_info
      if (x <= 7) {
        return RegObj("a" + std::to_string(x), ObjKind::FUNC_PARAM_REG);
      } else {
        int out_sp = call_stack_info.stack_size + 4 * (x - 8);
        return RegObj(std::to_string(out_sp) + "(sp)", ObjKind::FUNC_PARAM_ADD);
      }
    }
  };

  class IRVisitor {
  private:
    IR::Context context;
    std::unordered_map<koopa_raw_value_t, RegObj> value_reg_map;
  public:
    void HandleRawProgram(const koopa_raw_program_t &program) {
      // 访问 raw program
      Visit(program);
    } 
    RegObj GetReg(koopa_raw_value_t value) {
      /** 
       * 根据value对应的tag的类型来决定如何获取寄存器
       * 如果能从表里找到正常返回
       * 如果是数字就先Visit一下
       * */      
      if (value_reg_map.find(value) != value_reg_map.end()) {
        return value_reg_map[value];
      }
      if (value->kind.tag == KOOPA_RVT_INTEGER) {
        int val = value->kind.data.integer.value;
        if (val == 0) {
          return RegObj("x0", ObjKind::TEMP_REG);
        }
        assert(val >= -2147483648 && val <= 2147483647);
        // std::cout << "GetRegCall" << std::endl;
        return RegObj(Visit(value->kind.data.integer), ObjKind::TEMP_REG);
      }
      return RegObj("RegNameError");
    }
    // 目前仅用于建立value到栈空间的映射
    void SetReg(koopa_raw_value_t value, RegObj reg) {
      value_reg_map[value] = reg;
    }
    /** 
     * 把value的返回值加载到临时寄存器中并返回
     * 如果是地址类型会先加载, 
     * 注意使用此函数结束后需要resst()临时寄存器 
     * */
    RegObj GetTemReg(koopa_raw_value_t value) {
      if (value_reg_map.find(value) != value_reg_map.end()) {
        RegObj reg = value_reg_map[value];
        RegObj load_reg = context.t_reg_builder.NewReg();
        std::cout << "  lw " << load_reg.reg_name << ", " << reg.reg_name << "\n";
        return load_reg;
      } else {
        return GetReg(value);
      }
    }
    // 把地址寄存器加载到临时寄存器中
    RegObj AddToReg(RegObj reg) {
      assert(reg.kind == ObjKind::FUNC_PARAM_ADD ||
      reg.kind == ObjKind::LOCALVAR_ADD ||
      reg.kind == ObjKind::RA_ADD
      );
      RegObj load_reg = context.t_reg_builder.NewReg();
      std::cout << "  lw " << load_reg.reg_name << ", " << reg.reg_name << "\n";
      return load_reg;
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
    void Visit(const koopa_raw_branch_t &branch);
    void Visit(const koopa_raw_jump_t &jump);
    void Visit(const koopa_raw_call_t &call, koopa_raw_value_t value);
    // 遍历函数的指令以得到分配的空间
    void VisitFuncIns(const koopa_raw_slice_t &slice, IR::FuncInformation &func_information);
    void VisitFuncIns(const koopa_raw_value_t &value, IR::FuncInformation &func_information);

    void SetStackSpace(IR::FuncInformation &func_information);
  };
};