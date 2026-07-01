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
  context.call_stack_info.param_sp_top = 4 * (func_info.max_param_num - 8);
}

// 访问函数
void IR::IRVisitor::Visit(const koopa_raw_function_t &func) {
  if (func->bbs.len == 0) {
    return;
  }
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
    {
      // 访问 alloc 指令
      assert(value->ty->tag == KOOPA_RTT_POINTER);
      auto base_ty = value->ty->data.pointer.base;
      int size = GetTypeSize(base_ty);
      for (int i = 0; i < size / 4; i++) {
        reg_name = context.GetAddress(ObjKind::LOCAL_VAR_ADD);
      }
      SetMap(value, reg_name);
      break;
    }
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
      SetMap(value, context.GetParamRegOrAddForGet(kind.data.func_arg_ref.index));
      break;
    case KOOPA_RVT_GLOBAL_ALLOC:
      Visit(kind.data.global_alloc, value);
      break;
    case KOOPA_RVT_GET_ELEM_PTR:
      Visit(kind.data.get_elem_ptr, value);
      break;
    case KOOPA_RVT_GET_PTR:
      Visit(kind.data.get_ptr, value);
      break;
    default:
      // 其他类型暂时遇不到
      assert(false);
  }
}

/***********  访问对应类型指令  ***********/

// 对于%1 = add 3, 4 通过GetAdd找不到映射，就要对3和4进行Visit
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
    RegObj reg_name = GetTemReg(ret.value);  
    std::cout << "  # " << "ret " << reg_name.reg_name << std::endl;
    assert(reg_name.kind == ObjKind::TEMP_REG || reg_name.kind == ObjKind::FUNC_PARAM_REG);
    if (reg_name.reg_name != "a0") {
      std::cout << "  mv a0, " << reg_name.reg_name << std::endl;
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
  RegObj lhs_reg = GetTemReg(binary.lhs);
  RegObj rhs_reg = GetTemReg(binary.rhs);
  // 注意GetAdd的调用次数
  std::string lhs_reg_name = lhs_reg.reg_name;
  std::string rhs_reg_name = rhs_reg.reg_name;
  RegObj dest_address = context.GetAddress(ObjKind::LOCAL_VAR_ADD);

  std::cout << "  # " << dest_address.reg_name << " = " << binary.op << " " << lhs_reg_name << ", " << rhs_reg_name << std::endl;

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
  SetMap(value, dest_address);
  context.t_reg_builder.Reset();
}

void IR::IRVisitor::Visit(const koopa_raw_load_t &load, koopa_raw_value_t value) {
  RegObj dest_address = context.GetAddress(ObjKind::LOCAL_VAR_ADD);

  if (load.src->kind.tag == KOOPA_RVT_ALLOC) {
    // 局部变量：直接加载它的值
    RegObj src_reg = GetMap(load.src);
    assert(src_reg.kind == ObjKind::LOCAL_VAR_ADD);
    RegObj temp = AddToReg(src_reg);
    std::cout << "  # " << dest_address.reg_name << " = load " << src_reg.reg_name << std::endl;
    std::cout << "  sw " << temp.reg_name << ", " << dest_address.reg_name << std::endl;
  } else if (load.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
    // 全局变量：加载它的值
    RegObj src_reg = GetMap(load.src);
    RegObj temp = AddToReg(src_reg);
    std::cout << "  # " << dest_address.reg_name << " = load " << src_reg.reg_name << std::endl;
    std::cout << "  sw " << temp.reg_name << ", " << dest_address.reg_name << std::endl;
  } else {
    // 从一个计算出的指针指向的地址加载值
    RegObj addr_reg = GetTemReg(load.src); // 加载指针本身的(即目标物理地址)
    std::cout << "  # " << dest_address.reg_name << " = load from pointer in " << addr_reg.reg_name << std::endl;
    std::cout << "  sw " << addr_reg.reg_name << ", " << dest_address.reg_name << std::endl;
  }
  SetMap(value, dest_address);
  context.t_reg_builder.Reset();
}

void IR::IRVisitor::Visit(const koopa_raw_store_t &store) {
  RegObj src_reg = GetTemReg(store.value);
  std::string src_reg_name = src_reg.reg_name;

  if (store.dest->kind.tag == KOOPA_RVT_ALLOC) {
    RegObj dest_reg = GetMap(store.dest);
    assert(dest_reg.kind == ObjKind::LOCAL_VAR_ADD);
    std::cout << "  # store " << src_reg_name << ", " << dest_reg.reg_name << std::endl;
    std::cout << "  sw " << src_reg_name << ", " << dest_reg.reg_name << std::endl;
  } else if (store.dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
    RegObj dest_reg = GetMap(store.dest);
    RegObj addr_reg = GlobalVarToAdd(dest_reg);
    std::cout << "  # store " << src_reg_name << ", " << addr_reg.reg_name << std::endl;
    std::cout << "  sw " << src_reg_name << ", " << addr_reg.reg_name << std::endl;
  } else {
    // 写入一个计算出的内存地址：%dest 保存的是目标地址本身，需要解引用写入
    RegObj addr_reg = GetTemReg(store.dest);
    std::cout << "  # store " << src_reg_name << ", 0(" << addr_reg.reg_name << ")" << std::endl;
    std::cout << "  sw " << src_reg_name << ", 0(" << addr_reg.reg_name << ")" << std::endl;
  }
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
  context.t_reg_builder.Reset();
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
  context.call_stack_info.param_sp = 0;
  assert(call.args.kind == KOOPA_RSIK_VALUE);
  for (size_t i = 0; i < call.args.len; i++) {
    auto ptr = call.args.buffer[i];
    koopa_raw_value_t val = reinterpret_cast<koopa_raw_value_t>(ptr);
    auto tem_reg = GetTemReg(val);
    auto param_reg = context.GetParamRegOrAddForSet(i);
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
    RegObj reg_obj = context.GetAddress(ObjKind::LOCAL_VAR_ADD);
    SetMap(value, reg_obj);
    std::cout << "  sw a0, " << reg_obj.reg_name << "\n";
  }
}

void IR::IRVisitor::Visit(const koopa_raw_global_alloc_t &alloc, koopa_raw_value_t value) {
  std::string var_name = value->name;
  var_name = var_name.substr(1);
  if (alloc.init->kind.tag == KOOPA_RVT_ZERO_INIT) {
    std::cout << "  .bss\n";
    std::cout << "  .globl " << var_name << "\n";
    std::cout << var_name << ": \n";
    std::cout << "  .zero 4" << "\n";
  } else {
    int init_value = alloc.init->kind.data.integer.value;
    std::cout << "  .data" << std::endl;
    std::cout << "  .globl " << var_name << std::endl;
    std::cout << var_name << ":\n";
    std::cout << "  .word " << init_value << std::endl;
  }
  SetMap(value, RegObj(var_name, ObjKind::GLOBAL_VAR_ADD));
}

void IR::IRVisitor::Visit(const koopa_raw_get_elem_ptr_t &get_elem_ptr, koopa_raw_value_t value) {
  assert(get_elem_ptr.src->ty->tag == KOOPA_RTT_POINTER);
  auto base_ty = get_elem_ptr.src->ty->data.pointer.base;
  assert(base_ty->tag == KOOPA_RTT_ARRAY);
  
  int elem_size = GetTypeSize(base_ty->data.array.base); // 获取子元素大小
  RegObj raw_add_reg = LoadPointerToReg(get_elem_ptr.src);
  RegObj index_reg = GetTemReg(get_elem_ptr.index);

  RegObj size_reg = context.t_reg_builder.NewReg();
  RegObj offset_reg = context.t_reg_builder.NewReg();
  std::cout << "  li " << size_reg.reg_name << ", " << elem_size << "\n";
  std::cout << "  mul " << offset_reg.reg_name << ", " << index_reg.reg_name << ", " << size_reg.reg_name << "\n";
  context.t_reg_builder.Reset();

  RegObj temp_reg = context.t_reg_builder.NewReg();
  std::cout << "  add " << temp_reg.reg_name << ", " << raw_add_reg.reg_name << ", " << offset_reg.reg_name << "\n";
  
  RegObj dest_reg = context.GetAddress(ObjKind::LOCAL_VAR_ADD);
  std::cout << "  sw " << temp_reg.reg_name << ", " << dest_reg.reg_name << "\n";
  SetMap(value, dest_reg); 
  context.t_reg_builder.Reset();
}

void IR::IRVisitor::Visit(const koopa_raw_get_ptr_t &get_ptr, koopa_raw_value_t value) {
  assert(get_ptr.src->ty->tag == KOOPA_RTT_POINTER);
  auto base_ty = get_ptr.src->ty->data.pointer.base;
  
  int elem_size = GetTypeSize(base_ty); // 基类型即为所指元素的大小
  RegObj raw_add_reg = LoadPointerToReg(get_ptr.src);
  RegObj index_reg = GetTemReg(get_ptr.index);

  RegObj size_reg = context.t_reg_builder.NewReg();
  RegObj offset_reg = context.t_reg_builder.NewReg();
  std::cout << "  li " << size_reg.reg_name << ", " << elem_size << "\n";
  std::cout << "  mul " << offset_reg.reg_name << ", " << index_reg.reg_name << ", " << size_reg.reg_name << "\n";
  context.t_reg_builder.Reset();

  RegObj temp_reg = context.t_reg_builder.NewReg();
  std::cout << "  add " << temp_reg.reg_name << ", " << raw_add_reg.reg_name << ", " << offset_reg.reg_name << "\n";
  
  RegObj dest_reg = context.GetAddress(ObjKind::LOCAL_VAR_ADD);
  std::cout << "  sw " << temp_reg.reg_name << ", " << dest_reg.reg_name << "\n";
  SetMap(value, dest_reg); 
  context.t_reg_builder.Reset();
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
  if (ty->tag == KOOPA_RTT_UNIT) {
  } else if (value->kind.tag == KOOPA_RVT_ALLOC) {
    // 局部变量分配：计算基类型的大小，累加对应的 4 字节槽位数
    int slots = GetTypeSize(ty->data.pointer.base) / 4;
    func_info.return_ins_number += slots;
  } else {
    func_info.return_ins_number += 1;
  }

  auto tag = value->kind.tag;
  if (tag == KOOPA_RVT_CALL) {
    func_info.if_call = true;
    int len = value->kind.data.call.args.len;
    if (len > func_info.max_param_num) {
      func_info.max_param_num = len;
    }
  }
}

int IR::IRVisitor::GetTypeSize(koopa_raw_type_t ty) {
  if (ty->tag == KOOPA_RTT_INT32) {
    return 4;
  } else if (ty->tag == KOOPA_RTT_POINTER) {
    return 4;
  } else if (ty->tag == KOOPA_RTT_ARRAY) {
    return ty->data.array.len * GetTypeSize(ty->data.array.base);
  }
  return 0;
}

IR::RegObj IR::IRVisitor::LoadPointerToReg(koopa_raw_value_t value) {
  RegObj reg = GetMap(value);
  RegObj load_reg = context.t_reg_builder.NewReg();
  if (value->kind.tag == KOOPA_RVT_ALLOC) {
    // 栈上分配的局部数组：计算其首地址
    assert(reg.kind == ObjKind::LOCAL_VAR_ADD);
    std::string offset_str = reg.reg_name.substr(0, reg.reg_name.find('('));
    int offset = std::stoi(offset_str);
    if (offset >= -2048 && offset <= 2047) {
      std::cout << "  addi " << load_reg.reg_name << ", sp, " << offset << "\n";
    } else {
      std::cout << "  li " << load_reg.reg_name << ", " << offset << "\n";
      std::cout << "  add " << load_reg.reg_name << ", sp, " << load_reg.reg_name << "\n";
    }
  } else if (value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
    // 全局变量：使用 la 加载首地址
    assert(reg.kind == ObjKind::GLOBAL_VAR_ADD);
    std::cout << "  la " << load_reg.reg_name << ", " << reg.reg_name << "\n";
  } else {
    // 已经是存储在寄存器或栈上的指针（例如 getelemptr 的结果或指针参数）
    if (reg.kind == ObjKind::TEMP_REG || reg.kind == ObjKind::FUNC_PARAM_REG) {
      std::cout << "  mv " << load_reg.reg_name << ", " << reg.reg_name << "\n";
    } else {
      // 存储在栈上，通过 lw 读取指针的值
      std::cout << "  lw " << load_reg.reg_name << ", " << reg.reg_name << "\n";
    }
  }
  return load_reg;
}
