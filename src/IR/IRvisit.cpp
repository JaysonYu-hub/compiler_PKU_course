#include "IRvisit.h"
#include "iostream"
#include "cassert"

void handle_raw_program(koopa_raw_program_t raw) {
  std::cout << ".text" << std::endl;
  for (size_t i = 0; i < raw.funcs.len; i++) {
    assert(raw.funcs.kind == KOOPA_RSIK_FUNCTION);
    koopa_raw_function_t func = (koopa_raw_function_t)raw.funcs.buffer[i];
    std::string func_name(func->name);
    func_name = func_name.substr(1); // 去掉前缀 '_'
    std::cout << ".global " << func_name << std::endl;
    std::cout << func_name << ":" << std::endl;

    for (size_t j = 0; j < func->bbs.len; ++j) {
      assert(func->bbs.kind == KOOPA_RSIK_BASIC_BLOCK);
      koopa_raw_basic_block_t bb = (koopa_raw_basic_block_t) func->bbs.buffer[j];

      for (size_t k = 0; k < bb->insts.len; ++k) {
        assert(bb->insts.kind == KOOPA_RSIK_VALUE);
        koopa_raw_value_t value = (koopa_raw_value_t) bb->insts.buffer[k];
        assert(value->kind.tag == KOOPA_RVT_RETURN);
        koopa_raw_value_t ret_value = value->kind.data.ret.value;
        assert(ret_value->kind.tag == KOOPA_RVT_INTEGER);
        int32_t int_val = ret_value->kind.data.integer.value;
        std::cout << "li a0, " << int_val << std::endl;
        std::cout << "ret" << std::endl;
      }
    }
  }
}


// 访问 raw program
void Visit(const koopa_raw_program_t &program) {
  // 执行一些其他的必要操作
  // 访问所有全局变量
  Visit(program.values);
  std::cout << ".text" << std::endl;
  // 访问所有函数
  Visit(program.funcs);
}

// 访问 raw slice
void Visit(const koopa_raw_slice_t &slice) {
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

// 访问函数
void Visit(const koopa_raw_function_t &func) {
  // 执行一些其他的必要操作
  std::string func_name(func->name);
  func_name = func_name.substr(1); // 去掉前缀 '_'
  std::cout << ".global " << func_name << std::endl;
  std::cout << func_name << ":" << std::endl;
  // 访问所有基本块
  Visit(func->bbs);
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb) {
  // 执行一些其他的必要操作
  // ...
  // 访问所有指令
  Visit(bb->insts);
}

// 访问指令
void Visit(const koopa_raw_value_t &value) {
  // 根据指令类型判断后续需要如何访问
  const auto &kind = value->kind;
  switch (kind.tag) {
    case KOOPA_RVT_RETURN:
      // 访问 return 指令
      Visit(kind.data.ret);
      break;
    case KOOPA_RVT_INTEGER:
      // 访问 integer 指令
      Visit(kind.data.integer);
      break;
    default:
      // 其他类型暂时遇不到
      assert(false);
  }
}

// 访问对应类型指令的函数定义略
// 视需求自行实现
// ...
void Visit(const koopa_raw_integer_t &integer) {
  // 访问 integer 指令
  std::cout << "li a0, " << integer.value << std::endl;
}

void Visit(const koopa_raw_return_t &ret) {
  // 访问 return 指令
  Visit(ret.value);
  std::cout << "ret" << std::endl;
}