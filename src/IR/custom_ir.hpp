#pragma once
#include <string>
#include <memory>
#include <vector>
#include <cstdint>
#include <variant>
#include <cassert>
#include <type_traits>

namespace IR {

  enum class IRType {
    IR_INT32,
    IR_FLOAT,
    IR_VOID,
    IR_POINTER,
    IR_ARRAY,
    IR_DEFAULT
  };

  enum class IRValueKind {
    IR_CONSTANT,
    IR_SYMBOL,
    IR_GLOBAL_ALLOC,
    IR_FUNC_PARAM,
    IR_INSTRUCTION,
    IR_BASIC_BLOCK,
    IR_FUNC_DECL,
    IR_FUNCTION
  };

  enum class IRBinaryOp {
    ADD, SUB, MUL, DIV, MOD, 
    AND, OR, XOR, 
    SHL, SHR, SAR, 
    EQ, NE, 
    LT, LE, GT, GE 
  };

  enum class IRCastOp {
    INT_TO_FLOAT,
    FLOAT_TO_INT
  };

  enum class IRInstructionType {
    IR_INST_BINARY,
    IR_INST_CAST,
    IR_INST_ALLOC,
    IR_INST_STORE,
    IR_INST_LOAD,
    IR_INST_RET,
    IR_INST_BR,
    IR_INST_CALL,
    IR_INST_GETPTR,
    IR_INST_GETELEMPTR,
    IR_INST_JUMP,
    IR_INST_GLOBAL_ALLOC,
    IR_INST_UNKNOWN
  };

  std::string OpToString(IRBinaryOp op);
  std::string CastOpToString(IRCastOp op);
  std::string TypeToString(IRType type);
  IRType StringToType(const std::string &type);


  // 基础 Value 类
  class IRValue : public std::enable_shared_from_this<IRValue> {
  protected:
    IRValueKind kind;
    explicit IRValue(IRValueKind kind) : kind(kind) {}
  public:
    virtual ~IRValue() = default;
    IRValueKind GetKind() const { return kind; }
    virtual std::string DumpText() const = 0;
    virtual std::string GetOperandName() const = 0;
    virtual IRType GetType() const { return IRType::IR_DEFAULT; }
  };

  // 基础指令类
  class IRInstruction : public IRValue {
  public:
    IRInstructionType inst_type;
    IRInstruction(IRValueKind kind, IRInstructionType inst_type) : IRValue(kind), inst_type(inst_type) {}
    IRInstruction(IRInstructionType inst_type) : IRValue(IRValueKind::IR_INSTRUCTION), inst_type(inst_type) {}
    virtual ~IRInstruction() = default;

    std::string GetOperandName() const override;
  };

  // 具有 SSA 寄存器返回值的指令基类
  class IRNameInstruction : public IRInstruction {
  protected:
    std::string name;
  public:
    IRNameInstruction(IRValueKind kind, IRInstructionType inst_type, std::string name)
      : IRInstruction(kind, inst_type), name(std::move(name)) {}
    IRNameInstruction(IRInstructionType inst_type, std::string name)
      : IRInstruction(IRValueKind::IR_INSTRUCTION, inst_type), name(std::move(name)) {}
    std::string GetName() const { return name; }
    std::string GetOperandName() const override;
  };

  // 整型常量类
  class IRConstant : public IRValue {
  private:
    IRType const_type;
    std::variant<int32_t, float> val;
  public:
    explicit IRConstant(int32_t val) : IRValue(IRValueKind::IR_CONSTANT), const_type(IRType::IR_INT32), val(val) {}
    explicit IRConstant(float val) : IRValue(IRValueKind::IR_CONSTANT), const_type(IRType::IR_FLOAT), val(val) {}

    std::variant<int32_t, float> GetValue() const { return val; }

    std::string DumpText() const override;
    std::string GetOperandName() const override;
    IRType GetType() const override { return const_type; }
  };

  // 符号/变量引用类
  class IRSymbol : public IRValue {
  private:
    std::string name;
  public:
    explicit IRSymbol(std::string name) : IRValue(IRValueKind::IR_SYMBOL), name(std::move(name)) {}

    std::string DumpText() const override;
    std::string GetOperandName() const override;
    IRType GetType() const override { return IRType::IR_POINTER; }
  };

  // 全局变量分配类
  class IRGlobalAlloc : public IRValue {
  private:
    std::string name;
    std::string type;
    bool is_zeroinit;
    std::vector<int32_t> init_vals;
    std::vector<float> init_float_vals;
    std::vector<int> array_dims;

    static std::string FormatNestedInitializer(const std::vector<int32_t>& vals, 
                                               const std::vector<int>& dims, 
                                               size_t dim_idx, 
                                               size_t& val_idx);
    static std::string FormatNestedFloatInitializer(const std::vector<float>& vals, 
                                                    const std::vector<int>& dims, 
                                                    size_t dim_idx, 
                                                    size_t& val_idx);
  public:
    IRGlobalAlloc(std::string name, std::string type, bool is_zeroinit, std::vector<int32_t> init_vals, std::vector<int> array_dims = {})
      : IRValue(IRValueKind::IR_GLOBAL_ALLOC), name(std::move(name)), type(std::move(type)), is_zeroinit(is_zeroinit), init_vals(std::move(init_vals)), array_dims(std::move(array_dims)) {}

    IRGlobalAlloc(std::string name, std::string type, bool is_zeroinit, std::vector<float> init_float_vals, std::vector<int> array_dims = {})
      : IRValue(IRValueKind::IR_GLOBAL_ALLOC), name(std::move(name)), type(std::move(type)), is_zeroinit(is_zeroinit), init_float_vals(std::move(init_float_vals)), array_dims(std::move(array_dims)) {}

    bool IsZeroInit() const { return is_zeroinit; }
    const std::vector<int32_t>& GetInitVals() const { return init_vals; }
    const std::vector<float>& GetInitFloatVals() const { return init_float_vals; }
    const std::vector<int>& GetArrayDims() const { return array_dims; }
    bool IsArray() const { return !array_dims.empty(); }
    int GetAllocSize() const {
      if (array_dims.empty()) return 4;
      int size = 4;
      for (int d : array_dims) size *= d;
      return size;
    }

    std::string DumpText() const override;
    std::string GetOperandName() const override;
    IRType GetType() const override { return IRType::IR_POINTER; }
  };

  // 函数形参类
  class IRFuncParam : public IRValue {
  private:
    std::string name;
    IRType type;
    std::string type_str;
  public:
    IRFuncParam(std::string name, IRType type, std::string type_str) 
      : IRValue(IRValueKind::IR_FUNC_PARAM), name(std::move(name)), type(type), type_str(std::move(type_str)) {}

    std::string DumpText() const override;
    std::string GetOperandName() const override;
    IRType GetType() const override { return type; }
  };


  // 二元计算指令类
  class IRBinary : public IRNameInstruction {
  private:
    IRBinaryOp op;
    std::shared_ptr<IRValue> lhs;
    std::shared_ptr<IRValue> rhs;
    IRType operand_type;
    IRType result_type;
  public:
    IRBinary(std::string name, IRBinaryOp op, std::shared_ptr<IRValue> lhs,
             std::shared_ptr<IRValue> rhs, IRType operand_type,
             IRType result_type)
      : IRNameInstruction(IRInstructionType::IR_INST_BINARY, std::move(name)),
        op(op), lhs(std::move(lhs)), rhs(std::move(rhs)),
        operand_type(operand_type), result_type(result_type) {
      assert(this->lhs != nullptr && this->rhs != nullptr);
    }

    IRBinary(std::string name, IRBinaryOp op, std::shared_ptr<IRValue> lhs,
             std::shared_ptr<IRValue> rhs);

    IRBinaryOp GetOp() const { return op; }
    std::shared_ptr<IRValue> GetLhs() const { return lhs; }
    std::shared_ptr<IRValue> GetRhs() const { return rhs; }
    IRType GetOperandType() const { return operand_type; }

    std::string DumpText() const override;
    IRType GetType() const override;
  };

  // SysY permits implicit conversion between int and float.  Keeping that
  // conversion explicit in IR prevents the backend from guessing from a use.
  class IRCast : public IRNameInstruction {
  private:
    IRCastOp op;
    std::shared_ptr<IRValue> value;
    IRType target_type;
  public:
    IRCast(std::string name, IRCastOp op, std::shared_ptr<IRValue> value,
           IRType target_type)
      : IRNameInstruction(IRInstructionType::IR_INST_CAST, std::move(name)),
        op(op), value(std::move(value)), target_type(target_type) {
      assert(this->value != nullptr);
    }

    IRCastOp GetOp() const { return op; }
    std::shared_ptr<IRValue> GetValue() const { return value; }
    std::string DumpText() const override;
    IRType GetType() const override { return target_type; }
  };
  
  // alloc 指令类
  class IRAlloc : public IRNameInstruction {
  private:
    IR::IRType type;
    std::vector<int> array_dims;
    std::string alloc_type_str;
  public:
    IRAlloc(std::string name, std::string type_str, std::vector<int> array_dims = {})
      : IRNameInstruction(IRInstructionType::IR_INST_ALLOC, std::move(name)), type(StringToType(type_str)), array_dims(std::move(array_dims)), alloc_type_str(std::move(type_str)) {}

    IRAlloc(std::string name, std::string type_str, int array_size)
      : IRAlloc(std::move(name), std::move(type_str), std::vector<int>{array_size}) {}

    const std::vector<int>& GetArrayDims() const { return array_dims; }
    bool IsArray() const { return !array_dims.empty(); }
    IRType GetAllocatedType() const { return type; }
    int GetAllocSize() const {
      if (array_dims.empty()) return 4;
      int size = 4;
      for (int d : array_dims) size *= d;
      return size;
    }

    std::string DumpText() const override;
    IRType GetType() const override;
  };

  // store 指令类
  class IRStore : public IRInstruction {
  private:
    std::shared_ptr<IRValue> val;
    std::shared_ptr<IRValue> ptr;
  public:
    IRStore(std::shared_ptr<IRValue> val, std::shared_ptr<IRValue> ptr);
    std::shared_ptr<IRValue> GetVal() const { return val; }
    std::shared_ptr<IRValue> GetPtr() const { return ptr; }
    std::string DumpText() const override;
  };

  // load 指令类
  class IRLoad : public IRNameInstruction {
  private:
    std::shared_ptr<IRValue> ptr;
    IRType loaded_type;
  public:
    IRLoad(std::string name, std::shared_ptr<IRValue> ptr, IRType type = IRType::IR_INT32)
      : IRNameInstruction(IRInstructionType::IR_INST_LOAD, std::move(name)), ptr(std::move(ptr)), loaded_type(type) {
      assert(this->ptr != nullptr);
    }
    std::shared_ptr<IRValue> GetPtr() const { return ptr; }
    std::string DumpText() const override;
    IRType GetType() const override;
  };

  // getelemptr 指令类
  class IRGetElemPtr : public IRNameInstruction {
  private:
    std::shared_ptr<IRValue> arr;
    std::shared_ptr<IRValue> index;
    int elem_size;  // 子元素大小（字节）
  public:
    IRGetElemPtr(std::string name, std::shared_ptr<IRValue> arr, std::shared_ptr<IRValue> index, int elem_size = 4)
      :  IRNameInstruction(IRInstructionType::IR_INST_GETELEMPTR, std::move(name)), arr(std::move(arr)), index(std::move(index)), elem_size(elem_size) {
      assert(this->arr != nullptr && this->index != nullptr);
    }

    std::shared_ptr<IRValue> GetArr() const { return arr; }
    std::shared_ptr<IRValue> GetIndex() const { return index; }
    int GetElemSize() const { return elem_size; }

    std::string DumpText() const override;
    IRType GetType() const override;
  };

  // getptr 指令类
  class IRGetPtr : public IRNameInstruction {
  private:
    std::shared_ptr<IRValue> ptr;
    std::shared_ptr<IRValue> index;
    int elem_size;  // 元素大小（字节）
  public:
    IRGetPtr(std::string name, std::shared_ptr<IRValue> ptr, std::shared_ptr<IRValue> index, int elem_size = 4)
      : IRNameInstruction(IRInstructionType::IR_INST_GETPTR, std::move(name)), ptr(std::move(ptr)), index(std::move(index)), elem_size(elem_size) {
      assert(this->ptr != nullptr && this->index != nullptr);
    }

    std::shared_ptr<IRValue> GetPtr() const { return ptr; }
    std::shared_ptr<IRValue> GetIndex() const { return index; }
    int GetElemSize() const { return elem_size; }

    std::string DumpText() const override;
    IRType GetType() const override;
  };

  // jump 指令类
  class IRJump : public IRInstruction {
  private:
    std::shared_ptr<IRValue> target;
  public:
    explicit IRJump(std::shared_ptr<IRValue> target) : IRInstruction(IRInstructionType::IR_INST_JUMP), target(std::move(target)) {
      assert(this->target != nullptr);
    }
    std::shared_ptr<IRValue> GetTarget() const { return target; }
    std::string DumpText() const override;
  };

  // branch 指令类
  class IRBranch : public IRInstruction {
  private:
    std::shared_ptr<IRValue> cond;
    std::shared_ptr<IRValue> then_target;
    std::shared_ptr<IRValue> else_target;
  public:
    IRBranch(std::shared_ptr<IRValue> cond, std::shared_ptr<IRValue> then_target, std::shared_ptr<IRValue> else_target)
      : IRInstruction(IRInstructionType::IR_INST_BR), cond(std::move(cond)), then_target(std::move(then_target)), else_target(std::move(else_target)) {
      assert(this->cond != nullptr && this->then_target != nullptr && this->else_target != nullptr);
    }

    std::shared_ptr<IRValue> GetCond() const { return cond; }
    std::shared_ptr<IRValue> GetThenTarget() const { return then_target; }
    std::shared_ptr<IRValue> GetElseTarget() const { return else_target; }

    std::string DumpText() const override;
  };

  // 返回指令类
  class IRReturn : public IRInstruction {
  private:
    std::shared_ptr<IRValue> val;
  public:
    explicit IRReturn(std::shared_ptr<IRValue> val = nullptr) : IRInstruction(IRInstructionType::IR_INST_RET), val(std::move(val)) {}

    std::shared_ptr<IRValue> GetValue() const { return val; }
    std::string DumpText() const override;
  };

  // 函数调用指令类
  class IRCall : public IRInstruction {
  private:
    std::string name;
    std::shared_ptr<IRValue> func;
    std::vector<std::shared_ptr<IRValue>> args;
    IRType ret_type;
  public:
    IRCall(std::string name, std::shared_ptr<IRValue> func, std::vector<std::shared_ptr<IRValue>> args, IRType ret_type)
      : IRInstruction(IRInstructionType::IR_INST_CALL), name(std::move(name)), func(std::move(func)), args(std::move(args)), ret_type(ret_type) {
      assert(this->func != nullptr);
    }

    bool HasReturnValue() const { return !name.empty(); }
    std::shared_ptr<IRValue> GetFunc() const { return func; }
    const std::vector<std::shared_ptr<IRValue>>& GetArgs() const { return args; }

    std::string DumpText() const override;
    std::string GetOperandName() const override;
    IRType GetType() const override { return ret_type; }
  };

  // 基本块类
  class IRBasicBlock : public IRValue {
  public:
    std::string name;
    std::vector<std::shared_ptr<IRInstruction>> insts;
    std::shared_ptr<IRBasicBlock> next_1;
    std::shared_ptr<IRBasicBlock> next_2;

    explicit IRBasicBlock(std::string name) : IRValue(IRValueKind::IR_BASIC_BLOCK), name(std::move(name)) {}

    void AddInstruction(std::shared_ptr<IRInstruction> inst);
    void SetNext(std::shared_ptr<IRBasicBlock> next);
    void SetNext(std::shared_ptr<IRBasicBlock> next_1, std::shared_ptr<IRBasicBlock> next_2);
    std::string DumpText() const override;
    std::string GetOperandName() const override;
    IRType GetType() const override;
  };

  // 库函数声明类
  class IRFuncDecl : public IRValue {
  public:
    std::string name;
    IRType ret_type;
    std::vector<IRType> param_types;
  public:
    IRFuncDecl(std::string name, IRType ret_type, std::vector<IRType> param_types);

    std::string DumpText() const override;
    std::string GetOperandName() const override;
    IRType GetType() const override { return ret_type; }
  };

  // 函数类
  class IRFunction : public IRValue {
  public:
    std::string name;
    IRType ret_type;
    std::vector<std::shared_ptr<IRFuncParam>> params;
    std::vector<std::shared_ptr<IRBasicBlock>> blocks;

    IRFunction(std::string name, IRType ret_type) : IRValue(IRValueKind::IR_FUNCTION), name(std::move(name)), ret_type(ret_type) {}

    std::string DumpText() const override;
    std::string GetOperandName() const override;
    IRType GetRetType() const { return ret_type; }
    IRType GetType() const override { return ret_type; }
  };

  // 程序根类
  class IRProgram {
  public:
    std::vector<std::shared_ptr<IRFuncDecl>> decls;
    std::vector<std::shared_ptr<IRGlobalAlloc>> globals;
    std::vector<std::shared_ptr<IRFunction>> funcs;

    std::string DumpText() const;
  };

  // isa implementation for raw pointers
  template <typename Target, typename Source>
  bool isa(const Source* val) {
    if (!val) return false;
    return dynamic_cast<const Target*>(val) != nullptr;
  }

  // isa implementation for shared_ptr
  template <typename Target, typename Source>
  bool isa(const std::shared_ptr<Source>& val) {
    return std::dynamic_pointer_cast<Target>(val) != nullptr;
  }

  // cast implementation for shared_ptr
  template <typename Target, typename Source>
  std::shared_ptr<Target> cast(const std::shared_ptr<Source>& val) {
    assert(isa<Target>(val) && "Invalid cast!");
    return std::static_pointer_cast<Target>(val);
  }

  // cast implementation for raw pointers
  template <typename Target, typename Source>
  Target* cast(Source* val) {
    assert(isa<Target>(val) && "Invalid cast!");
    return const_cast<Target*>(dynamic_cast<const Target*>(val));
  }

  template <typename Target, typename Source>
  const Target* cast(const Source* val) {
    assert(isa<Target>(val) && "Invalid cast!");
    return dynamic_cast<const Target*>(val);
  }

  // dynamic_cast_ptr implementation for shared_ptr
  template <typename Target, typename Source>
  std::shared_ptr<Target> dynamic_cast_ptr(const std::shared_ptr<Source>& val) {
    return std::dynamic_pointer_cast<Target>(val);
  }
}