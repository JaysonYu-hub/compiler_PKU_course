#pragma once

#include <string>
#include <memory>
#include <vector>
#include <cstdint>

namespace IR {

  enum class IRType {
    IR_INT32,
    IR_VOID,
    IR_POINTER,
    IR_ARRAY
  };

  enum class IRBinaryOp {
    ADD, SUB, MUL, DIV, MOD, 
    AND, OR, XOR, 
    SHL, SHR, SAR, 
    EQ, NE, 
    LT, LE, GT, GE 
  };

  std::string OpToString(IRBinaryOp op);
  std::string TypeToString(IRType type);
  IRType StringToType(const std::string &type);

  // 基础 Value 类
  class IRValue : public std::enable_shared_from_this<IRValue> {
  public:
    virtual ~IRValue() = default;
    virtual std::string DumpText() const = 0;
    virtual std::string GetOperandName() const = 0;
    virtual IRType GetType() const = 0;
  };

  // 整型常量类
  class IRConstant : public IRValue {
  private:
    int32_t val;
  public:
    explicit IRConstant(int32_t val) : val(val) {}
    int32_t GetValue() const { return val; }

    std::string DumpText() const override;
    std::string GetOperandName() const override;
    IRType GetType() const override;
  };

  // 符号/变量引用类
  class IRSymbol : public IRValue {
  private:
    std::string name;
  public:
    explicit IRSymbol(std::string name) : name(std::move(name)) {}

    std::string DumpText() const override;
    std::string GetOperandName() const override;
    IRType GetType() const override;
  };

  // 全局变量分配类
  class IRGlobalAlloc : public IRValue {
  private:
    std::string name;
    std::string type;
    bool is_zeroinit;
    std::vector<int32_t> init_vals;
    std::vector<int> array_dims;

    static std::string FormatNestedInitializer(const std::vector<int32_t>& vals, 
                                               const std::vector<int>& dims, 
                                               size_t dim_idx, 
                                               size_t& val_idx);
  public:
    IRGlobalAlloc(std::string name, std::string type, bool is_zeroinit, std::vector<int32_t> init_vals, std::vector<int> array_dims = {});

    bool IsZeroInit() const { return is_zeroinit; }
    const std::vector<int32_t>& GetInitVals() const { return init_vals; }
    const std::vector<int>& GetArrayDims() const { return array_dims; }
    bool IsArray() const { return !array_dims.empty(); }
    // 返回分配的总字节数
    int GetAllocSize() const {
      if (array_dims.empty()) return 4;
      int size = 4;
      for (int d : array_dims) size *= d;
      return size;
    }

    std::string DumpText() const override;
    std::string GetOperandName() const override;
    IRType GetType() const override;
  };

  // 函数形参类
  class IRFuncParam : public IRValue {
  private:
    std::string name;
    IRType type;
    std::string type_str;
  public:
    IRFuncParam(std::string name, IRType type, std::string type_str);

    std::string DumpText() const override;
    std::string GetOperandName() const override;
    IRType GetType() const override;
  };

  // 基础指令类
  class IRInstruction : public IRValue {
  public:
    virtual ~IRInstruction() = default;

    std::string GetOperandName() const override;
    IRType GetType() const override;
  };

  // 具有 SSA 寄存器返回值的指令基类
  class IRNameInstruction : public IRInstruction {
  protected:
    std::string name;
  public:
    explicit IRNameInstruction(std::string name) : name(std::move(name)) {}
    std::string GetName() const { return name; }
    std::string GetOperandName() const override;
  };

  // 二元计算指令类
  class IRBinary : public IRNameInstruction {
  private:
    IRBinaryOp op;
    std::shared_ptr<IRValue> lhs;
    std::shared_ptr<IRValue> rhs;
  public:
    IRBinary(std::string name, IRBinaryOp op, std::shared_ptr<IRValue> lhs, std::shared_ptr<IRValue> rhs);

    IRBinaryOp GetOp() const { return op; }
    std::shared_ptr<IRValue> GetLhs() const { return lhs; }
    std::shared_ptr<IRValue> GetRhs() const { return rhs; }

    std::string DumpText() const override;
    IRType GetType() const override;
  };
  
  // alloc 指令类
  class IRAlloc : public IRNameInstruction {
  private:
    IR::IRType type;
    std::vector<int> array_dims;
    std::string alloc_type_str;
  public:
    IRAlloc(std::string name, std::string type_str, std::vector<int> array_dims = {});
    IRAlloc(std::string name, std::string type_str, int array_size);

    const std::vector<int>& GetArrayDims() const { return array_dims; }
    bool IsArray() const { return !array_dims.empty(); }
    // 返回分配的总字节数
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
  public:
    IRLoad(std::string name, std::shared_ptr<IRValue> ptr);

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
    IRGetElemPtr(std::string name, std::shared_ptr<IRValue> arr, std::shared_ptr<IRValue> index, int elem_size = 4);

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
    IRGetPtr(std::string name, std::shared_ptr<IRValue> ptr, std::shared_ptr<IRValue> index, int elem_size = 4);

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
    explicit IRJump(std::shared_ptr<IRValue> target);

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
    IRBranch(std::shared_ptr<IRValue> cond, std::shared_ptr<IRValue> then_target, std::shared_ptr<IRValue> else_target);

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
    explicit IRReturn(std::shared_ptr<IRValue> val = nullptr);

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
    IRCall(std::string name, std::shared_ptr<IRValue> func, std::vector<std::shared_ptr<IRValue>> args, IRType ret_type);

    bool HasReturnValue() const { return !name.empty(); }
    std::shared_ptr<IRValue> GetFunc() const { return func; }
    const std::vector<std::shared_ptr<IRValue>>& GetArgs() const { return args; }

    std::string DumpText() const override;
    std::string GetOperandName() const override;
    IRType GetType() const override;
  };

  // 基本块类
  class IRBasicBlock : public IRValue {
  public:
    std::string name;
    std::vector<std::shared_ptr<IRInstruction>> insts;

    explicit IRBasicBlock(std::string name);

    void AddInstruction(std::shared_ptr<IRInstruction> inst);
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
    IRType GetType() const override;
  };

  // 函数类
  class IRFunction : public IRValue {
  public:
    std::string name;
    IRType ret_type;
    std::vector<std::shared_ptr<IRFuncParam>> params;
    std::vector<std::shared_ptr<IRBasicBlock>> blocks;

    IRFunction(std::string name, IRType ret_type);

    std::string DumpText() const override;
    std::string GetOperandName() const override;
    IRType GetType() const override;
  };

  // 程序根类
  class IRProgram {
  public:
    std::vector<std::shared_ptr<IRFuncDecl>> decls;
    std::vector<std::shared_ptr<IRGlobalAlloc>> globals;
    std::vector<std::shared_ptr<IRFunction>> funcs;

    std::string DumpText() const;
  };
}
