#include "IRvisit.hpp"
#include <iostream>
#include <cassert>

// 访问 raw program
void IR::IRVisitor::Visit(const koopa_raw_program_t &program) {
  // 执行一些其他的必要操作
  // 访问所有全局变量
  Visit(program.values);
  // 访问所有函数
  Visit(program.funcs);
}

// 访问 raw slice
void IR::IRVisitor::Visit(const koopa_raw_slice_t &slice) {
  for (size_t i = 0; i < slice.len; ++i) {
    auto ptr = slice.buffer[i];
    // 根据 slice 的 kind 决定将 ptr 视作何种元素
    switch (slice.kind) {
      case KOOPA_RSIK_FUNCTION:
        // 访问函数
        Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
        break;
      case KOOPA_RSIK_BASIC_BLOCK:
        // 访问基本块
        Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
        break;
      case KOOPA_RSIK_VALUE:
        // 访问指令
        Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
        break;
      default:
        // 我们暂时不会遇到其他内容, 于是不对其做任何处理
        assert(false);
    }
  }
}

// 计算函数调用需要分配的栈空间
void IR::IRVisitor::SetStackSpace(IR::FuncInformation &func_info) {
  int inst_num = func_info.return_ins_number;
  int spark_len = 0;
  spark_len += 4 * inst_num;
  spark_len += 4 * (func_info.max_param_num - 8);
  if (func_info.if_call) {
    spark_len += 4;
    context.call_stack_info.local_var_sp_top = -4;
  } else {
    context.call_stack_info.local_var_sp_top = 0;
  }
  if (spark_len != 0) {
    spark_len = spark_len - spark_len % 16 + 16;
  }
  context.call_stack_info.stack_size = spark_len;
  context.call_stack_info.local_var_sp_top += spark_len;
  context.call_stack_info.local_var_sp = context.call_stack_info.local_var_sp_top;
}

// 访问函数
void IR::IRVisitor::Visit(const koopa_raw_function_t &func) {
  // 计算需要分配的栈空间
  IR::FuncInformation func_info;
  VisitFuncIns(func->bbs, func_info);
  //func_info.PrintFuncInfo();
  SetStackSpace(func_info);
  int spark_len = context.call_stack_info.stack_size;
  
  // 执行一些其他的必要操作
  std::string func_name(func->name);
  func_name = func_name.substr(1); // 去掉前缀
  std::cout << "  .text" << std::endl;
  std::cout << "  .global " << func_name << std::endl;
  std::cout << func_name << ":" << std::endl;
  if (spark_len != 0) {
    // 分配栈空间
    std::cout << "  # 函数的prologue" << std::endl;
    std::cout << "  addi sp, sp, -" << spark_len << std::endl;
  }
  if (func_info.if_call) {
    std::cout << "  sw ra, " << context.GetAddress(ObjKind::RA_ADD).reg_name << std::endl;
  }
  // 把函数参数加载一下
  Visit(func->params);

  // 访问所有基本块
  Visit(func->bbs);
}

// 访问基本块
void IR::IRVisitor::Visit(const koopa_raw_basic_block_t &bb) {
  // 执行一些其他的必要操作
  // 输出基本块名称
  std::string bb_name = bb->name ? bb->name : "";
  if (bb_name != "\%entry") {
    bb_name = bb_name.substr(1) + ":";
    std::cout << bb_name << std::endl;
  }
  // 访问所有指令
  Visit(bb->insts);
}

// 访问指令
void IR::IRVisitor::Visit(const koopa_raw_value_t &value) {
  // 根据指令类型判断后续需要如何访问
  RegObj reg_name;
  const auto &kind = value->kind;
  switch (kind.tag) {
    case KOOPA_RVT_RETURN:
      // 访问 return 指令
      Visit(kind.data.ret);
      break;
    case KOOPA_RVT_INTEGER:
      // 访问 integer 指令
      // 只有有返回值，需要修改映射表的函数需要value
      Visit(kind.data.integer);
      break;
    case KOOPA_RVT_BINARY:
      // 访问 binary 指令
      Visit(kind.data.binary, value);
      break;
    case KOOPA_RVT_LOAD:
      // 访问 load 指令
      Visit(kind.data.load, value);
      break;
    case KOOPA_RVT_STORE:
      // 访问 store 指令
      Visit(kind.data.store);
      break;
    case KOOPA_RVT_ALLOC:
      // 访问 alloc 指令
      reg_name = context.GetAddress(ObjKind::LOCALVAR_ADD);
      SetReg(value, reg_name);
      break;
    case KOOPA_RVT_BRANCH:
      Visit(kind.data.branch);
      break;
    case KOOPA_RVT_JUMP:
      Visit(kind.data.jump);
      break;
    case KOOPA_RVT_CALL:
      Visit(kind.data.call, value);
      break;
    case KOOPA_RVT_FUNC_ARG_REF:
      // 函数定义时，建立函数参数value到对应寄存器或地址的映射
      SetReg(value, context.GetParamRegOrAddForGet(kind.data.func_arg_ref.index));
      break;
    default:
      // 其他类型暂时遇不到
      std::cout << (kind.tag == KOOPA_RVT_CALL);
      assert(false);
  }
}

/***********  访问对应类型指令  ***********/

// 对于%1 = add 3, 4 通过GetReg找不到映射，就要对3和4进行Visit
std::string IR::IRVisitor::Visit(const koopa_raw_integer_t &integer) {
  // 访问 integer 指令
  if (integer.value != 0) {
    std::string reg = context.t_reg_builder.NewReg().reg_name;
    std::cout << "  li "<< reg <<", " << integer.value << std::endl;
    // 这里使用返回值而不是SetReg()是因为寄存器可能被覆盖
    return reg;
  }
  return "x0";
}

void IR::IRVisitor::Visit(const koopa_raw_return_t &ret) {
  // 访问 return 指令
  // ret %2 | ret 3
  if (ret.value != nullptr) {
    RegObj reg_name = GetReg(ret.value);  
    std::cout << "  # " << "ret " << reg_name.reg_name << std::endl;
    if (reg_name.kind == ObjKind::LOCALVAR_ADD) {
      std::cout << "  lw a0, " << reg_name.reg_name << std::endl;
    } else if (reg_name.kind == ObjKind::TEMP_REG) {
      std::cout << "  mv a0, " << reg_name.reg_name << std::endl;
    } else {
      assert(false);
    }
  }
  int spark_len = context.call_stack_info.stack_size;
  if (spark_len != 0) {
    std::cout << "  # 函数的 epilogue\n"; 
    if (context.call_stack_info.IfCall()) {
      std::cout << "  lw ra, " << context.GetAddress(ObjKind::RA_ADD).reg_name << "\n";
    }
    std::cout << "  addi sp, sp, " << std::to_string(spark_len) << std::endl;
  }
  std::cout << "  ret" << std::endl;
}

void IR::IRVisitor::Visit(const koopa_raw_binary_t &binary, koopa_raw_value_t value) {
  // 访问 binary 指令
  // %1 = add %0, %1 | %1 = add %0, 3
  // %1 = add %0, %1 这里的%0只是一个value指针，需要判断是否为address
  RegObj lhs_reg = GetReg(binary.lhs);
  RegObj rhs_reg = GetReg(binary.rhs);
  // 注意GetReg的调用次数
  ObjKind lhs_reg_kind = lhs_reg.kind;
  ObjKind rhs_reg_kind = rhs_reg.kind;

  std::string lhs_reg_name = lhs_reg.reg_name;
  std::string rhs_reg_name = rhs_reg.reg_name;
  RegObj dest_address = context.GetAddress(ObjKind::LOCALVAR_ADD);

  std::cout << "  # " << dest_address.reg_name << " = " << binary.op << " " << lhs_reg_name << ", " << rhs_reg_name << std::endl;

  if (lhs_reg_kind == ObjKind::LOCALVAR_ADD) {
    std::string temp = context.t_reg_builder.NewReg().reg_name;
    std::cout << "  lw " << temp << ", " << lhs_reg_name << std::endl;
    lhs_reg_name = temp;
  }
  if (rhs_reg_kind == ObjKind::LOCALVAR_ADD) {
    std::string temp = context.t_reg_builder.NewReg().reg_name;
    std::cout << "  lw " << temp << ", " << rhs_reg_name << std::endl;
    rhs_reg_name = temp;
  }
  std::string dest_reg = context.t_reg_builder.NewReg().reg_name;
  switch (binary.op) {
    case KOOPA_RBO_EQ:
      std::cout << "  xor " << dest_reg << ", " << lhs_reg_name << ", " << rhs_reg_name << std::endl;
      std::cout << "  seqz " << dest_reg << ", " << dest_reg << std::endl;
      break;
    case KOOPA_RBO_NOT_EQ:
      std::cout << "  xor " << dest_reg << ", " << lhs_reg_name << ", " << rhs_reg_name << std::endl;
      std::cout << "  snez " << dest_reg << ", " << dest_reg << std::endl;
      break;
    case KOOPA_RBO_ADD:
      std::cout << "  add " << dest_reg << ", " << lhs_reg_name << ", " << rhs_reg_name << std::endl;
      break;
    case KOOPA_RBO_SUB:
      std::cout << "  sub " << dest_reg << ", " << lhs_reg_name << ", " << rhs_reg_name << std::endl;
      break;
    case KOOPA_RBO_MUL:
      std::cout << "  mul " << dest_reg << ", " << lhs_reg_name << ", " << rhs_reg_name << std::endl;
      break;
    case KOOPA_RBO_DIV:
      std::cout << "  div " << dest_reg << ", " << lhs_reg_name << ", " << rhs_reg_name << std::endl;
      break;
    case KOOPA_RBO_MOD:
      std::cout << "  rem " << dest_reg << ", " << lhs_reg_name << ", " << rhs_reg_name << std::endl;
      break;
    case KOOPA_RBO_AND:
      std::cout << "  and " << dest_reg << ", " << lhs_reg_name << ", " << rhs_reg_name << std::endl;
      break;
    case KOOPA_RBO_OR:
      std::cout << "  or " << dest_reg << ", " << lhs_reg_name << ", " << rhs_reg_name << std::endl;
      break;
    case KOOPA_RBO_LT:
      std::cout << "  slt " << dest_reg << ", " << lhs_reg_name << ", " << rhs_reg_name << std::endl;
      break;
    case KOOPA_RBO_LE:
      std::cout << "  sgt " << dest_reg << ", " << rhs_reg_name << ", " << lhs_reg_name << std::endl;
      break;
    case KOOPA_RBO_GT:
      std::cout << "  sgt " << dest_reg << ", " << lhs_reg_name << ", " << rhs_reg_name << std::endl;
      break;
    case KOOPA_RBO_GE:
      std::cout << "  slt " << dest_reg << ", " << rhs_reg_name << ", " << lhs_reg_name << std::endl;
      break;
    default:
      // 其他操作符暂时遇不到
      assert(false);
  }
  std::cout << "  sw " << dest_reg << ", " << dest_address.reg_name << std::endl;
  SetReg(value, dest_address);
  context.t_reg_builder.Reset();
}

void IR::IRVisitor::Visit(const koopa_raw_load_t &load, koopa_raw_value_t value) {
  // 访问 load 指令
  // %0 = load @x
  // assert(GetReg(load.src).kind == ObjKind::LOCALVAR_ADD);
  RegObj dest_address = context.GetAddress(ObjKind::LOCALVAR_ADD);
  std::string src_reg = GetReg(load.src).reg_name;

  std::cout << "  # " << dest_address.reg_name << " = load " << src_reg << std::endl;
  std::cout << "  lw " << "t0" << ", " << src_reg << std::endl;
  std::cout << "  sw " << "t0" << ", " << dest_address.reg_name << std::endl;
  SetReg(value, dest_address);
}

void IR::IRVisitor::Visit(const koopa_raw_store_t &store) {
  // 访问 store 指令
  // store %0, @x | store 10, @x | store @x(函数参数/栈地址), @y
  RegObj src_reg = GetReg(store.value);
  RegObj dest_reg = GetReg(store.dest);

  if (src_reg.kind == ObjKind::FUNC_PARAM_ADD 
    || src_reg.kind == ObjKind::LOCALVAR_ADD
    || src_reg.kind == ObjKind::RA_ADD ) {
    src_reg = AddToReg(src_reg);
  }
  
  assert(dest_reg.kind == ObjKind::LOCALVAR_ADD);
  
  std::string src_reg_name = src_reg.reg_name;
  std::string dest_address = dest_reg.reg_name;
  std::cout << "  # " << "store " << src_reg_name << ", " << dest_address << std::endl;
  std::cout << "  sw " << src_reg_name << ", " << dest_address << std::endl;
  context.t_reg_builder.Reset();
}

void IR::IRVisitor::Visit(const koopa_raw_branch_t &branch) {
  // 访问 branch 指令
  // br %0, %1, %2
  RegObj cond_reg = GetTemReg(branch.cond);
  std::string cond_reg_name = cond_reg.reg_name;
  std::string true_bb_name = branch.true_bb->name;
  true_bb_name = true_bb_name.substr(1);
  std::string false_bb_name = branch.false_bb->name;
  false_bb_name = false_bb_name.substr(1);
  std::cout << "  # " << "br " << cond_reg_name << ", " << true_bb_name << ", " << false_bb_name << std::endl;
  std::cout << "  bnez " << cond_reg_name << ", " << true_bb_name << std::endl;
  std::cout << "  j " << false_bb_name << std::endl;
}
void IR::IRVisitor::Visit(const koopa_raw_jump_t &jump) {
  // 访问 jump 指令
  // jmp %0
  std::cout << "  # " << "jump " << jump.target->name << std::endl;
  std::string jump_target = jump.target->name;
  jump_target = jump_target.substr(1);
  std::cout << "  j " << jump_target << std::endl;
  context.t_reg_builder.Reset();
}

void IR::IRVisitor::Visit(const koopa_raw_call_t &call, koopa_raw_value_t value) {
  /**处理call命令 
   * 把所需参数放到寄存器和栈上
   */
  assert(call.args.kind == KOOPA_RSIK_VALUE);
  context.call_stack_info.param_sp_bottom = -1;
  for (size_t i = 0; i < call.args.len; i++) {
    auto ptr = call.args.buffer[i];
    koopa_raw_value_t val = reinterpret_cast<koopa_raw_value_t>(ptr);
    auto tem_reg = GetTemReg(val);
    int t = i;
    if (t >= 8) {
      t = call.args.len - (t - 8);
    }
    // 这里希望t是从大到小
    auto param_reg = context.GetParamRegOrAddForSet(t);
    context.t_reg_builder.Reset();
    if (param_reg.kind == ObjKind::FUNC_PARAM_ADD) {
      std::cout << "  sw " << tem_reg.reg_name << ", " << param_reg.reg_name << "\n";
    } else if (param_reg.kind == ObjKind::FUNC_PARAM_REG) {
      std::cout << "  mv " << param_reg.reg_name << ", " << tem_reg.reg_name << "\n";
    } else {
      assert(false);
    }
  }
  std::string temp = call.callee->name;
  temp = temp.substr(1);
  std::cout << "  call " << temp << std::endl;

  // 调用之后要把返回值放到local_var_reg上, 如果有的话
  if (value->ty->tag != KOOPA_RTT_UNIT) {
    RegObj reg_obj = context.GetAddress(ObjKind::LOCALVAR_ADD);
    SetReg(value, reg_obj);
    std::cout << "  sw a0, " << reg_obj.reg_name << "\n";
  }
}

// 用于统计函数内有返回值的指令数
void IR::IRVisitor::VisitFuncIns(const koopa_raw_slice_t &slice, IR::FuncInformation &func_info) {
  for (size_t i = 0; i < slice.len; ++i) {
    auto ptr = slice.buffer[i];
    switch (slice.kind) {
      case KOOPA_RSIK_BASIC_BLOCK:
        // 访问基本块
        VisitFuncIns(reinterpret_cast<koopa_raw_basic_block_t>(ptr)->insts, func_info);
        break;
      case KOOPA_RSIK_VALUE:
        VisitFuncIns(reinterpret_cast<koopa_raw_value_t>(ptr), func_info);
        break;
      default:
        // 其他类型暂时遇不到
        assert(false);
    }
  }
}

void IR::IRVisitor::VisitFuncIns(const koopa_raw_value_t &value, IR::FuncInformation &func_info) {
  // 根据指令类型判断后续需要如何访问
  const auto &ty = value->ty;
  if (ty->tag != KOOPA_RTT_UNIT) {
    func_info.return_ins_number += 1;
  }

  auto tag = value->kind.tag;
  if (tag == KOOPA_RVT_CALL) {
    func_info.if_call = true;
  }

  if (value->kind.tag == KOOPA_RVT_CALL) {
    int len = value->kind.data.call.args.len;
    if (len > func_info.max_param_num) {
      func_info.max_param_num = len;
    }
  }
}

