#pragma once

#include <string>
#include <memory>
#include <vector>
#include <cstdint>
#include <cassert>

namespace IR {

  // IR 数据类型
  enum class IRType {
    IR_INT32,
    IR_VOID,
    IR_POINTER,
    IR_ARRAY
  };

  // 基础 Value 类，代表所有可以作为操作数的目标（如常量、指令的返回值等）
  class IRValue : public std::enable_shared_from_this<IRValue> {
  public:
    virtual ~IRValue() = default;
    
    // 生成对应的 Koopa IR 文本表示
    virtual std::string DumpText() const = 0;

    // 作为操作数使用时的名字（常量返回自身，指令返回寄存器名）
    virtual std::string GetOperandName() const = 0;
    
    // 获取当前 Value 的数据类型
    virtual IRType GetType() const = 0;
  };

  // 整型常量类
  class IRConstant : public IRValue {
  private:
    int32_t val;
  public:
    explicit IRConstant(int32_t val) : val(val) {}
    
    int32_t GetValue() const { return val; }

    std::string DumpText() const override {
      return std::to_string(val);
    }

    std::string GetOperandName() const override {
      return std::to_string(val);
    }

    IRType GetType() const override {
      return IRType::IR_INT32;
    }
  };

  // 符号/变量引用类
  class IRSymbol : public IRValue {
  private:
    std::string name;
  public:
    explicit IRSymbol(std::string name) : name(std::move(name)) {}

    std::string DumpText() const override {
      return name;
    }

    std::string GetOperandName() const override {
      return name;
    }

    IRType GetType() const override {
      return IRType::IR_POINTER;
    }
  };

  // 全局变量分配类
  class IRGlobalAlloc : public IRValue {
  private:
    std::string name;
    std::string type;
    bool is_zeroinit;
    std::vector<int32_t> init_vals;
  public:
    IRGlobalAlloc(std::string name, std::string type, bool is_zeroinit, std::vector<int32_t> init_vals)
      : name(std::move(name)), type(std::move(type)), is_zeroinit(is_zeroinit), init_vals(std::move(init_vals)) {}

    bool IsZeroInit() const { return is_zeroinit; }
    const std::vector<int32_t>& GetInitVals() const { return init_vals; }

    std::string DumpText() const override {
      if (is_zeroinit) {
        return "global " + name + " = alloc " + type + ", zeroinit";
      }
      if (init_vals.size() == 1) {
        return "global " + name + " = alloc " + type + ", " + std::to_string(init_vals[0]);
      }
      std::string str = "{";
      for (size_t i = 0; i < init_vals.size(); i++) {
        str += std::to_string(init_vals[i]);
        if (i != init_vals.size() - 1) {
          str += ", ";
        }
      }
      str += "}";
      return "global " + name + " = alloc " + type + ", " + str;
    }

    std::string GetOperandName() const override {
      return name;
    }

    IRType GetType() const override {
      return IRType::IR_POINTER;
    }
  };

  // 函数形参类
  class IRFuncParam : public IRValue {
  private:
    std::string name;
    IRType type;
  public:
    IRFuncParam(std::string name, IRType type) : name(std::move(name)), type(type) {}

    std::string DumpText() const override {
      return name + ": " + (type == IRType::IR_INT32 ? "i32" : "/* other */");
    }

    std::string GetOperandName() const override {
      return name;
    }

    IRType GetType() const override {
      return type;
    }
  };

  // 基础指令类，代表所有 IR 指令（包括有返回值和无返回值的）
  class IRInstruction : public IRValue {
  public:
    virtual ~IRInstruction() = default;

    std::string GetOperandName() const override {
      assert(false && "Void instruction cannot be used as an operand");
      return "";
    }

    IRType GetType() const override {
      return IRType::IR_VOID;
    };
  };

  // 具有 SSA 寄存器返回值的指令基类（如二元计算等）
  class IRNameInstruction : public IRInstruction {
  protected:
    std::string name; // SSA 寄存器名，如 "%0", "%1" 等
  public:
    explicit IRNameInstruction(std::string name) : name(std::move(name)) {}

    std::string GetName() const { return name; }

    std::string GetOperandName() const override {
      return name;
    }
  };

  // 二元计算操作符
  enum class IRBinaryOp {
    ADD, SUB, MUL, DIV, MOD, // 算术运算符
    AND, OR, XOR,            // 位运算符
    SHL, SHR, SAR,           // 移位运算符
    EQ, NE,                  // 等于、不等于
    LT, LE, GT, GE           // 比较运算符
  };

  // 将操作符枚举转换为 Koopa IR 的文本关键字
  inline std::string OpToString(IRBinaryOp op) {
    switch (op) {
      case IRBinaryOp::ADD: return "add";
      case IRBinaryOp::SUB: return "sub";
      case IRBinaryOp::MUL: return "mul";
      case IRBinaryOp::DIV: return "div";
      case IRBinaryOp::MOD: return "mod";
      case IRBinaryOp::AND: return "and";
      case IRBinaryOp::OR:  return "or";
      case IRBinaryOp::XOR: return "xor";
      case IRBinaryOp::SHL: return "shl";
      case IRBinaryOp::SHR: return "shr";
      case IRBinaryOp::SAR: return "sar";
      case IRBinaryOp::EQ:  return "eq";
      case IRBinaryOp::NE:  return "ne";
      case IRBinaryOp::LT:  return "lt";
      case IRBinaryOp::LE:  return "le";
      case IRBinaryOp::GT:  return "gt";
      case IRBinaryOp::GE:  return "ge";
      default: assert(false && "Invalid binary operator");
    }
    return "";
  }

  // 把类型枚举转换为字符串
  inline std::string TypeToString(IRType type) {
    switch (type) {
      case IRType::IR_INT32: return "i32";
      case IRType::IR_VOID:  return "void";
      case IRType::IR_POINTER: return "ptr";
      case IRType::IR_ARRAY: return "array";
      default: assert(false && "Invalid IR type");
    }
    return "";
  }

  // 把字符串转换为类型枚举
  inline IRType StringToType(const std::string &type) {
    if (type == "i32") return IRType::IR_INT32;
    if (type == "void") return IRType::IR_VOID;
    if (type == "ptr") return IRType::IR_POINTER;
    if (type == "array") return IRType::IR_ARRAY;
    assert(false && "Invalid IR type");
    return IRType::IR_VOID;
  }

  // 二元计算指令类
  class IRBinary : public IRNameInstruction {
  private:
    IRBinaryOp op;
    std::shared_ptr<IRValue> lhs;
    std::shared_ptr<IRValue> rhs;
  public:
    IRBinary(std::string name, IRBinaryOp op, std::shared_ptr<IRValue> lhs, std::shared_ptr<IRValue> rhs)
      : IRNameInstruction(std::move(name)), op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {
      assert(this->lhs != nullptr && this->rhs != nullptr);
    }

    IRBinaryOp GetOp() const { return op; }
    std::shared_ptr<IRValue> GetLhs() const { return lhs; }
    std::shared_ptr<IRValue> GetRhs() const { return rhs; }

    std::string DumpText() const override {
      return name + " = " + OpToString(op) + " " + lhs->GetOperandName() + ", " + rhs->GetOperandName();
    }

    IRType GetType() const override {
      // 在 SysY 中，二元计算的返回值目前都是 i32 整数
      return IRType::IR_INT32;
    }
  };
  
  // alloc 指令类
  class IRAlloc : public IRNameInstruction {
  private:
    IR::IRType type;
  public:
    IRAlloc(std::string name, IR::IRType type) : IRNameInstruction(std::move(name)), type(std::move(type)) {}
    IRAlloc(std::string name, std::string type) : IRNameInstruction(std::move(name)), type(StringToType(type)) {}
    std::string DumpText() const override {
      return name + " = alloc " + TypeToString(type);
    }

    IRType GetType() const override {
      return IRType::IR_POINTER;
    }
  };

  // store 指令类
  class IRStore : public IRInstruction {
  private:
    std::shared_ptr<IRValue> val;
    std::shared_ptr<IRValue> ptr;
  public:
    IRStore(std::shared_ptr<IRValue> val, std::shared_ptr<IRValue> ptr)
      : val(std::move(val)), ptr(std::move(ptr)) {
      assert(this->ptr != nullptr && this->val != nullptr);
      }
    std::string DumpText() const override {
      return "store " + val->GetOperandName() + ", " + ptr->GetOperandName();
    }
  };

  // load 指令类
  class IRLoad : public IRNameInstruction {
  private:
    std::shared_ptr<IRValue> ptr;
  public:
    explicit IRLoad(std::string name, std::shared_ptr<IRValue> ptr)
      : IRNameInstruction(std::move(name)), ptr(std::move(ptr)) {
      assert(this->ptr != nullptr);
    }

    std::shared_ptr<IRValue> GetPtr() const { return ptr; }
    std::string DumpText() const override {
      return name + " = load " + ptr->GetOperandName();
    }
    IRType GetType() const override {
      return IRType::IR_INT32;
    }
  };

  // jump 指令类
  class IRJump : public IRInstruction {
  private:
    std::shared_ptr<IRValue> target;
  public:
    explicit IRJump(std::shared_ptr<IRValue> target) : target(std::move(target)) {
      assert(this->target != nullptr);
    }

    std::shared_ptr<IRValue> GetTarget() const { return target; }

    std::string DumpText() const override {
      return "jump " + target->GetOperandName();
    }
  };

  // branch 指令类
  class IRBranch : public IRInstruction {
  private:
    std::shared_ptr<IRValue> cond;
    std::shared_ptr<IRValue> then_target;
    std::shared_ptr<IRValue> else_target;
  public:
    IRBranch(std::shared_ptr<IRValue> cond, std::shared_ptr<IRValue> then_target, std::shared_ptr<IRValue> else_target)
      : cond(std::move(cond)), then_target(std::move(then_target)), else_target(std::move(else_target)) {
      assert(this->cond != nullptr && this->then_target != nullptr && this->else_target != nullptr);
      }
    std::shared_ptr<IRValue> GetCond() const { return cond; }
    std::shared_ptr<IRValue> GetThenTarget() const { return then_target; }
    std::shared_ptr<IRValue> GetElseTarget() const { return else_target; }

    std::string DumpText() const override {
      return "br " + cond->GetOperandName() + ", " + then_target->GetOperandName() + ", " + else_target->GetOperandName();
    }
  };


  // 返回指令类
  class IRReturn : public IRInstruction {
  private:
    std::shared_ptr<IRValue> val; // 可以为 nullptr
  public:
    explicit IRReturn(std::shared_ptr<IRValue> val = nullptr) : val(std::move(val)) {}

    std::shared_ptr<IRValue> GetValue() const { return val; }

    std::string DumpText() const override {
      if (val) {
        return "ret " + val->GetOperandName();
      }
      return "ret";
    }
  };

  // 函数调用指令类（支持有/无返回值）
  class IRCall : public IRInstruction {
  private:
    std::string name; // 返回值寄存器名，若为 void 则为空
    std::shared_ptr<IRValue> func;
    std::vector<std::shared_ptr<IRValue>> args;
    IRType ret_type;
  public:
    IRCall(std::string name, std::shared_ptr<IRValue> func, std::vector<std::shared_ptr<IRValue>> args, IRType ret_type)
      : name(std::move(name)), func(std::move(func)), args(std::move(args)), ret_type(ret_type) {
      assert(this->func != nullptr);
    }

    bool HasReturnValue() const { return !name.empty(); }
    std::shared_ptr<IRValue> GetFunc() const { return func; }
    const std::vector<std::shared_ptr<IRValue>>& GetArgs() const { return args; }

    std::string DumpText() const override {
      std::string str;
      if (HasReturnValue()) {
        str += name + " = ";
      }
      str += "call " + func->GetOperandName() + "(";
      for (size_t i = 0; i < args.size(); i++) {
        str += args[i]->GetOperandName();
        if (i != args.size() - 1) {
          str += ", ";
        }
      }
      str += ")";
      return str;
    }

    std::string GetOperandName() const override {
      if (HasReturnValue()) {
        return name;
      }
      assert(false && "Void call instruction cannot be used as an operand");
      return "";
    }

    IRType GetType() const override {
      return ret_type;
    }
  };

  // 基本块类
  class IRBasicBlock : public IRValue {
  public:
    std::string name;
    std::vector<std::shared_ptr<IRInstruction>> insts;

    explicit IRBasicBlock(std::string name) : name(std::move(name)) {}

    void AddInstruction(std::shared_ptr<IRInstruction> inst) {
      insts.push_back(std::move(inst));
    }

    std::string DumpText() const override {
      std::string text = name + ":\n";
      for (const auto& inst : insts) {
        text += "  " + inst->DumpText() + "\n";
      }
      return text;
    }

    std::string GetOperandName() const override {
      return name;
    }
    IRType GetType() const override {
      return IRType::IR_VOID;
    }
  };

  // 库函数声明类
  class IRFuncDecl : public IRValue {
  public:
    std::string name;
    IRType ret_type;
    std::vector<IRType> param_types;
  public:
    IRFuncDecl(std::string name, IRType ret_type, std::vector<IRType> param_types)
      : name(std::move(name)), ret_type(ret_type), param_types(std::move(param_types)) {}

    std::string DumpText() const override {
      std::string text = "decl @" + name + "(";
      for (size_t i = 0; i < param_types.size(); i++) {
        if (param_types[i] == IRType::IR_INT32) {
          text += "i32";
        } else if (param_types[i] == IRType::IR_POINTER) {
          text += "*i32";
        }
        if (i != param_types.size() - 1) {
          text += ", ";
        }
      }
      text += ")";
      if (ret_type != IRType::IR_VOID) {
        text += " : i32";
      }
      return text;
    }

    std::string GetOperandName() const override {
      return "@" + name;
    }

    IRType GetType() const override {
      return ret_type;
    }
  };

  // 函数类
  class IRFunction : public IRValue {
  public:
    std::string name;
    IRType ret_type;
    std::vector<std::shared_ptr<IRFuncParam>> params;
    std::vector<std::shared_ptr<IRBasicBlock>> blocks;

    IRFunction(std::string name, IRType ret_type) : name(std::move(name)), ret_type(ret_type) {}

    std::string DumpText() const override {
      std::string text = "fun @" + name + "(";
      for (size_t i = 0; i < params.size(); i++) {
        text += params[i]->DumpText();
        if (i != params.size() - 1) {
          text += ", ";
        }
      }
      text += ")";
      if (ret_type != IRType::IR_VOID) {
        text += " : i32";
      }
      text += " {\n";
      for (const auto& block : blocks) {
        text += block->DumpText();
      }
      text += "}\n";
      return text;
    }

    std::string GetOperandName() const override {
      return "@" + name;
    }

    IRType GetType() const override {
      return ret_type;
    }
  };

  // 程序根类
  class IRProgram {
  public:
    std::vector<std::shared_ptr<IRFuncDecl>> decls;
    std::vector<std::shared_ptr<IRGlobalAlloc>> globals;
    std::vector<std::shared_ptr<IRFunction>> funcs;

    std::string DumpText() const {
      std::string text;
      for (const auto& decl : decls) {
        text += decl->DumpText() + "\n";
      }
      if (!decls.empty()) {
        text += "\n";
      }
      for (const auto& global : globals) {
        text += global->DumpText() + "\n";
      }
      if (!globals.empty()) {
        text += "\n";
      }
      for (const auto& func : funcs) {
        text += func->DumpText() + "\n";
      }
      return text;
    }
  };

}
