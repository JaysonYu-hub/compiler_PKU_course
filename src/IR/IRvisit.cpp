#include "IRvisit.hpp"
#include "iostream"
#include "cassert"
using namespace std;

// 访问 raw program
void IRVisitor::Visit(const koopa_raw_program_t &program) {
  // 执行一些其他的必要操作
  // 访问所有全局变量
  Visit(program.values);
  std::cout << ".text" << std::endl;
  // 访问所有函数
  Visit(program.funcs);
}

// 访问 raw slice
void IRVisitor::Visit(const koopa_raw_slice_t &slice) {
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
void IRVisitor::Visit(const koopa_raw_function_t &func) {
  // 执行一些其他的必要操作
  std::string func_name(func->name);
  func_name = func_name.substr(1); // 去掉前缀
  std::cout << ".global " << func_name << std::endl;
  std::cout << func_name << ":" << std::endl;
  // 访问所有基本块
  Visit(func->bbs);
}

// 访问基本块
void IRVisitor::Visit(const koopa_raw_basic_block_t &bb) {
  // 执行一些其他的必要操作
  // ...
  // 访问所有指令
  Visit(bb->insts);
}

// 访问指令
void IRVisitor::Visit(const koopa_raw_value_t &value) {
  // 根据指令类型判断后续需要如何访问
  const auto &kind = value->kind;
  switch (kind.tag) {
    case KOOPA_RVT_RETURN:
      // 访问 return 指令
      Visit(kind.data.ret);
      break;
    case KOOPA_RVT_INTEGER:
      // 访问 integer 指令
      Visit(kind.data.integer, value);
      break;
    case KOOPA_RVT_BINARY:
      // 访问 binary 指令
      Visit(kind.data.binary, value);
      break;
    default:
      // 其他类型暂时遇不到
      assert(false);
  }
}

// 访问对应类型指令的函数定义略
// 视需求自行实现
void IRVisitor::Visit(const koopa_raw_integer_t &integer, koopa_raw_value_t value) {
  // 访问 integer 指令
  if (integer.value != 0) {
    string reg = rc_builder.NewReg();
    std::cout << "li "<< reg <<", " << integer.value << std::endl;
    value_reg_map[value] = reg;
  }
}

void IRVisitor::Visit(const koopa_raw_return_t &ret) {
  // 访问 return 指令
  string reg = GetReg(ret.value);
  std::cout << "mv a0, " << reg << std::endl;
  std::cout << "ret" << std::endl;
}

void IRVisitor::Visit(const koopa_raw_binary_t &binary, koopa_raw_value_t value) {
  // 访问 binary 指令
  string lhs_reg = GetReg(binary.lhs);
  string rhs_reg = GetReg(binary.rhs);
  string dest_reg = rc_builder.NewReg();
  switch (binary.op) {
    case KOOPA_RBO_EQ:
      std::cout << "xor " << dest_reg << ", " << lhs_reg << ", " << rhs_reg << std::endl;
      std::cout << "seqz " << dest_reg << ", " << dest_reg << std::endl;
      break;
    case KOOPA_RBO_NOT_EQ:
      std::cout << "xor " << dest_reg << ", " << lhs_reg << ", " << rhs_reg << std::endl;
      std::cout << "snez " << dest_reg << ", " << dest_reg << std::endl;
      break;
    case KOOPA_RBO_ADD:
      std::cout << "add " << dest_reg << ", " << lhs_reg << ", " << rhs_reg << std::endl;
      break;
    case KOOPA_RBO_SUB:
      std::cout << "sub " << dest_reg << ", " << lhs_reg << ", " << rhs_reg << std::endl;
      break;
    case KOOPA_RBO_MUL:
      std::cout << "mul " << dest_reg << ", " << lhs_reg << ", " << rhs_reg << std::endl;
      break;
    case KOOPA_RBO_DIV:
      std::cout << "div " << dest_reg << ", " << lhs_reg << ", " << rhs_reg << std::endl;
      break;
    case KOOPA_RBO_MOD:
      std::cout << "rem " << dest_reg << ", " << lhs_reg << ", " << rhs_reg << std::endl;
      break;
    case KOOPA_RBO_AND:
      std::cout << "and " << dest_reg << ", " << lhs_reg << ", " << rhs_reg << std::endl;
      break;
    case KOOPA_RBO_OR:
      std::cout << "or " << dest_reg << ", " << lhs_reg << ", " << rhs_reg << std::endl;
      break;
    case KOOPA_RBO_LT:
      std::cout << "slt " << dest_reg << ", " << lhs_reg << ", " << rhs_reg << std::endl;
      break;
    case KOOPA_RBO_LE:
      std::cout << "sgt " << dest_reg << ", " << rhs_reg << ", " << lhs_reg << std::endl;
      break;
    case KOOPA_RBO_GT:
      std::cout << "sgt " << dest_reg << ", " << lhs_reg << ", " << rhs_reg << std::endl;
      break;
    case KOOPA_RBO_GE:
      std::cout << "slt " << dest_reg << ", " << rhs_reg << ", " << lhs_reg << std::endl;
      break;
    default:
      // 其他操作符暂时遇不到
      assert(false);
  }
  value_reg_map[value] = dest_reg;
}