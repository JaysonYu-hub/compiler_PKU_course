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
    IR_BLOCK_PARAM,
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
    IR_INST_ZERO_INIT,
    IR_INST_LOAD,
    IR_INST_RET,
    IR_INST_BR,
    IR_INST_CALL,
    IR_INST_GETPTR,
    IR_INST_GETELEMPTR,
    IR_INST_JUMP,
    IR_INST_GLOBAL_ALLOC,
    IR_INST_COMMENT,
    IR_INST_UNKNOWN
  };

  std::string OpToString(IRBinaryOp op);
  std::string CastOpToString(IRCastOp op);
  std::string TypeToString(IRType type);
  IRType StringToType(const std::string &type);
  std::string FloatToString(float value);

  // IR 对象内部统一保存不带 Koopa sigil 的名称；只有作为操作数或输出
  // 文本 IR 时才根据对象类别补上 '%' 或 '@'。
  inline std::string StripIRNameSigil(std::string name) {
    if (!name.empty() && (name.front() == '%' || name.front() == '@')) {
      name.erase(name.begin());
    }
    return name;
  }


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
    static bool classof(const IRValue* v) { return v != nullptr; }
  };

  // 基础指令类
  class IRInstruction : public IRValue {
  protected:
    std::vector<std::shared_ptr<IRValue>> operands;
  public:
    IRInstructionType inst_type;
    IRInstruction(IRValueKind kind, IRInstructionType inst_type) : IRValue(kind), inst_type(inst_type) {}
    IRInstruction(IRInstructionType inst_type) : IRValue(IRValueKind::IR_INSTRUCTION), inst_type(inst_type) {}
    virtual ~IRInstruction() = default;

    std::string GetOperandName() const override;
    virtual std::vector<std::shared_ptr<IRValue>> GetUses() const { return operands; }
    virtual bool ReplaceUse(const IRValue* old_value,
                            std::shared_ptr<IRValue> new_value);
    virtual bool HasSideEffect() const { return false; }
    static bool classof(const IRValue* v) {
      return v && v->GetKind() == IRValueKind::IR_INSTRUCTION;
    }
  };

  // 具有 SSA 寄存器返回值的指令基类
  class IRNameInstruction : public IRInstruction {
  protected:
    std::string name;
  public:
    IRNameInstruction(IRValueKind kind, IRInstructionType inst_type, std::string name)
      : IRInstruction(kind, inst_type), name(StripIRNameSigil(std::move(name))) {}
    IRNameInstruction(IRInstructionType inst_type, std::string name)
      : IRInstruction(IRValueKind::IR_INSTRUCTION, inst_type), name(StripIRNameSigil(std::move(name))) {}
    const std::string& GetName() const { return name; }
    std::string GetOperandName() const override;
    static bool classof(const IRValue* v) {
      if (!v || v->GetKind() != IRValueKind::IR_INSTRUCTION) return false;
      auto t = static_cast<const IRInstruction*>(v)->inst_type;
      return t == IRInstructionType::IR_INST_BINARY ||
             t == IRInstructionType::IR_INST_CAST ||
             t == IRInstructionType::IR_INST_ALLOC ||
             t == IRInstructionType::IR_INST_LOAD ||
             t == IRInstructionType::IR_INST_GETELEMPTR ||
             t == IRInstructionType::IR_INST_GETPTR;
    }
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
    static bool classof(const IRValue* v) {
      return v && v->GetKind() == IRValueKind::IR_CONSTANT;
    }
  };

  // 符号/变量引用类
  class IRSymbol : public IRValue {
  private:
    std::string name;
    char sigil;
  public:
    explicit IRSymbol(std::string raw_name)
      : IRValue(IRValueKind::IR_SYMBOL),
        name(StripIRNameSigil(raw_name)),
        sigil(!raw_name.empty() && raw_name.front() == '%' ? '%' : '@') {}

    const std::string& GetName() const { return name; }
    std::string DumpText() const override;
    std::string GetOperandName() const override;
    IRType GetType() const override { return IRType::IR_POINTER; }
    static bool classof(const IRValue* v) {
      return v && v->GetKind() == IRValueKind::IR_SYMBOL;
    }
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
      : IRValue(IRValueKind::IR_GLOBAL_ALLOC), name(StripIRNameSigil(std::move(name))), type(std::move(type)), is_zeroinit(is_zeroinit), init_vals(std::move(init_vals)), array_dims(std::move(array_dims)) {}

    IRGlobalAlloc(std::string name, std::string type, bool is_zeroinit, std::vector<float> init_float_vals, std::vector<int> array_dims = {})
      : IRValue(IRValueKind::IR_GLOBAL_ALLOC), name(StripIRNameSigil(std::move(name))), type(std::move(type)), is_zeroinit(is_zeroinit), init_float_vals(std::move(init_float_vals)), array_dims(std::move(array_dims)) {}

    const std::string& GetName() const { return name; }

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
    static bool classof(const IRValue* v) {
      return v && v->GetKind() == IRValueKind::IR_GLOBAL_ALLOC;
    }
  };

  // 函数形参类
  class IRFuncParam : public IRValue {
  private:
    std::string name;
    IRType type;
    std::string type_str;
  public:
    IRFuncParam(std::string name, IRType type, std::string type_str)
      : IRValue(IRValueKind::IR_FUNC_PARAM), name(StripIRNameSigil(std::move(name))), type(type), type_str(std::move(type_str)) {}

    const std::string& GetName() const { return name; }
    std::string DumpText() const override;
    std::string GetOperandName() const override;
    IRType GetType() const override { return type; }
    static bool classof(const IRValue* v) {
      return v && v->GetKind() == IRValueKind::IR_FUNC_PARAM;
    }
  };
 
  // 基本块参数
  class IRBlockParam : public IRValue {
  private:
    std::string name;
    IRType type;
  public:
    IRBlockParam(std::string name, IRType type)
      : IRValue(IRValueKind::IR_BLOCK_PARAM), name(StripIRNameSigil(std::move(name))), type(type) {}

    const std::string& GetName() const { return name; }
    std::string DumpText() const override;
    std::string GetOperandName() const override { return "%" + name; }
    IRType GetType() const override { return type; }
    static bool classof(const IRValue* v) {
      return v && v->GetKind() == IRValueKind::IR_BLOCK_PARAM;
    }
  };


  // 二元计算指令类
  class IRBinary : public IRNameInstruction {
  private:
    IRBinaryOp op;
    IRType operand_type;
    IRType result_type;
  public:
    IRBinary(std::string name, IRBinaryOp op, std::shared_ptr<IRValue> lhs,
             std::shared_ptr<IRValue> rhs, IRType operand_type,
             IRType result_type);

    IRBinary(std::string name, IRBinaryOp op, std::shared_ptr<IRValue> lhs,
             std::shared_ptr<IRValue> rhs);

    IRBinaryOp GetOp() const { return op; }
    std::shared_ptr<IRValue> GetLhs() const { return operands[0]; }
    std::shared_ptr<IRValue> GetRhs() const { return operands[1]; }
    IRType GetOperandType() const { return operand_type; }

    std::string DumpText() const override;
    IRType GetType() const override;
    static bool classof(const IRValue* v) {
      return v && v->GetKind() == IRValueKind::IR_INSTRUCTION &&
             static_cast<const IRInstruction*>(v)->inst_type == IRInstructionType::IR_INST_BINARY;
    }
  };

  // SysY permits implicit conversion between int and float.  Keeping that
  // conversion explicit in IR prevents the backend from guessing from a use.
  class IRCast : public IRNameInstruction {
  private:
    IRCastOp op;
    IRType target_type;
  public:
    IRCast(std::string name, IRCastOp op, std::shared_ptr<IRValue> value,
           IRType target_type);

    IRCastOp GetOp() const { return op; }
    std::shared_ptr<IRValue> GetValue() const { return operands[0]; }
    std::string DumpText() const override;
    IRType GetType() const override { return target_type; }
    static bool classof(const IRValue* v) {
      return v && v->GetKind() == IRValueKind::IR_INSTRUCTION &&
             static_cast<const IRInstruction*>(v)->inst_type == IRInstructionType::IR_INST_CAST;
    }
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
    static bool classof(const IRValue* v) {
      return v && v->GetKind() == IRValueKind::IR_INSTRUCTION &&
             static_cast<const IRInstruction*>(v)->inst_type == IRInstructionType::IR_INST_ALLOC;
    }
  };

  // store 指令类
  class IRStore : public IRInstruction {
  public:
    IRStore(std::shared_ptr<IRValue> val, std::shared_ptr<IRValue> ptr);
    std::shared_ptr<IRValue> GetVal() const { return operands[0]; }
    std::shared_ptr<IRValue> GetPtr() const { return operands[1]; }
    bool HasSideEffect() const override { return true; }
    std::string DumpText() const override;
    static bool classof(const IRValue* v) {
      return v && v->GetKind() == IRValueKind::IR_INSTRUCTION &&
             static_cast<const IRInstruction*>(v)->inst_type == IRInstructionType::IR_INST_STORE;
    }
  };

  // Compactly clear a contiguous local allocation before explicit stores.
  class IRZeroInit : public IRInstruction {
  private:
    int byte_size;
  public:
    IRZeroInit(std::shared_ptr<IRValue> ptr, int byte_size);

    std::shared_ptr<IRValue> GetPtr() const { return operands[0]; }
    int GetByteSize() const { return byte_size; }
    bool HasSideEffect() const override { return true; }
    std::string DumpText() const override;
    static bool classof(const IRValue* v) {
      return v && v->GetKind() == IRValueKind::IR_INSTRUCTION &&
             static_cast<const IRInstruction*>(v)->inst_type == IRInstructionType::IR_INST_ZERO_INIT;
    }
  };

  // load 指令类
  class IRLoad : public IRNameInstruction {
  private:
    IRType loaded_type;
  public:
    IRLoad(std::string name, std::shared_ptr<IRValue> ptr, IRType type = IRType::IR_INT32);
    std::shared_ptr<IRValue> GetPtr() const { return operands[0]; }
    std::string DumpText() const override;
    IRType GetType() const override;
    static bool classof(const IRValue* v) {
      return v && v->GetKind() == IRValueKind::IR_INSTRUCTION &&
             static_cast<const IRInstruction*>(v)->inst_type == IRInstructionType::IR_INST_LOAD;
    }
  };

  // getelemptr 指令类
  class IRGetElemPtr : public IRNameInstruction {
  private:
    int elem_size;  // 子元素大小（字节）
  public:
    IRGetElemPtr(std::string name, std::shared_ptr<IRValue> arr, std::shared_ptr<IRValue> index, int elem_size = 4);

    std::shared_ptr<IRValue> GetArr() const { return operands[0]; }
    std::shared_ptr<IRValue> GetIndex() const { return operands[1]; }
    int GetElemSize() const { return elem_size; }

    std::string DumpText() const override;
    IRType GetType() const override;
    static bool classof(const IRValue* v) {
      return v && v->GetKind() == IRValueKind::IR_INSTRUCTION &&
             static_cast<const IRInstruction*>(v)->inst_type == IRInstructionType::IR_INST_GETELEMPTR;
    }
  };

  // getptr 指令类
  class IRGetPtr : public IRNameInstruction {
  private:
    int elem_size;  // 元素大小（字节）
  public:
    IRGetPtr(std::string name, std::shared_ptr<IRValue> ptr, std::shared_ptr<IRValue> index, int elem_size = 4);

    std::shared_ptr<IRValue> GetPtr() const { return operands[0]; }
    std::shared_ptr<IRValue> GetIndex() const { return operands[1]; }
    int GetElemSize() const { return elem_size; }

    std::string DumpText() const override;
    IRType GetType() const override;
    static bool classof(const IRValue* v) {
      return v && v->GetKind() == IRValueKind::IR_INSTRUCTION &&
             static_cast<const IRInstruction*>(v)->inst_type == IRInstructionType::IR_INST_GETPTR;
    }
  };

  // jump 指令类
  class IRJump : public IRInstruction {
  private:
    std::shared_ptr<IRValue> target;
  public:
    explicit IRJump(std::shared_ptr<IRValue> target,
                    std::vector<std::shared_ptr<IRValue>> args = {});
    std::shared_ptr<IRValue> GetTarget() const { return target; }
    const std::vector<std::shared_ptr<IRValue>>& GetArgs() const { return operands; }
    void SetArgs(std::vector<std::shared_ptr<IRValue>> args);
    bool HasSideEffect() const override { return true; }
    std::string DumpText() const override;
    static bool classof(const IRValue* v) {
      return v && v->GetKind() == IRValueKind::IR_INSTRUCTION &&
             static_cast<const IRInstruction*>(v)->inst_type == IRInstructionType::IR_INST_JUMP;
    }
  };

  // branch 指令类
  class IRBranch : public IRInstruction {
  private:
    std::shared_ptr<IRValue> then_target;
    std::shared_ptr<IRValue> else_target;
    size_t then_arg_count = 0;
  public:
    IRBranch(std::shared_ptr<IRValue> cond,
             std::shared_ptr<IRValue> then_target,
             std::shared_ptr<IRValue> else_target,
             std::vector<std::shared_ptr<IRValue>> then_args = {},
             std::vector<std::shared_ptr<IRValue>> else_args = {});

    std::shared_ptr<IRValue> GetCond() const { return operands[0]; }
    std::shared_ptr<IRValue> GetThenTarget() const { return then_target; }
    std::shared_ptr<IRValue> GetElseTarget() const { return else_target; }
    std::vector<std::shared_ptr<IRValue>> GetThenArgs() const;
    std::vector<std::shared_ptr<IRValue>> GetElseArgs() const;
    void SetThenArgs(std::vector<std::shared_ptr<IRValue>> args);
    void SetElseArgs(std::vector<std::shared_ptr<IRValue>> args);

    bool HasSideEffect() const override { return true; }
    std::string DumpText() const override;
    static bool classof(const IRValue* v) {
      return v && v->GetKind() == IRValueKind::IR_INSTRUCTION &&
             static_cast<const IRInstruction*>(v)->inst_type == IRInstructionType::IR_INST_BR;
    }
  };

  // 返回指令类
  class IRReturn : public IRInstruction {
  public:
    explicit IRReturn(std::shared_ptr<IRValue> val = nullptr);

    std::shared_ptr<IRValue> GetValue() const { return operands.empty() ? nullptr : operands[0]; }
    bool HasSideEffect() const override { return true; }
    std::string DumpText() const override;
    static bool classof(const IRValue* v) {
      return v && v->GetKind() == IRValueKind::IR_INSTRUCTION &&
             static_cast<const IRInstruction*>(v)->inst_type == IRInstructionType::IR_INST_RET;
    }
  };

  // 函数调用指令类
  class IRCall : public IRInstruction {
  private:
    std::string name;
    std::shared_ptr<IRValue> func;
    IRType ret_type;
  public:
    IRCall(std::string name, std::shared_ptr<IRValue> func, std::vector<std::shared_ptr<IRValue>> args, IRType ret_type);

    bool HasReturnValue() const { return !name.empty(); }
    const std::string& GetName() const { return name; }
    std::shared_ptr<IRValue> GetFunc() const { return func; }
    const std::vector<std::shared_ptr<IRValue>>& GetArgs() const { return operands; }

    bool HasSideEffect() const override { return true; }
    std::string DumpText() const override;
    std::string GetOperandName() const override;
    IRType GetType() const override { return ret_type; }
    static bool classof(const IRValue* v) {
      return v && v->GetKind() == IRValueKind::IR_INSTRUCTION &&
             static_cast<const IRInstruction*>(v)->inst_type == IRInstructionType::IR_INST_CALL;
    }
  };

  // 调试注释指令类
  class IRComment : public IRInstruction {
  public:
    std::string comment;
    explicit IRComment(std::string comment)
      : IRInstruction(IRInstructionType::IR_INST_COMMENT), comment(std::move(comment)) {}

    bool HasSideEffect() const override { return true; }
    std::string DumpText() const override { return "// " + comment; }

    static bool classof(const IRValue* v) {
      return v && v->GetKind() == IRValueKind::IR_INSTRUCTION &&
             static_cast<const IRInstruction*>(v)->inst_type == IRInstructionType::IR_INST_COMMENT;
    }
  };

  // 基本块类
  class IRBasicBlock : public IRValue {
  public:
    std::string name;
    std::vector<std::shared_ptr<IRBlockParam>> params;
    std::vector<std::shared_ptr<IRInstruction>> insts;

    explicit IRBasicBlock(std::string name) : IRValue(IRValueKind::IR_BASIC_BLOCK), name(StripIRNameSigil(std::move(name))) {}

    const std::string& GetName() const { return name; }
    void AddInstruction(std::shared_ptr<IRInstruction> inst);
    void AddComment(std::string comment);
    std::shared_ptr<IRBlockParam> AddParam(std::string name, IRType type);
    const std::vector<std::shared_ptr<IRBlockParam>>& GetParams() const {
      return params;
    }
    bool RemoveInstruction(const IRInstruction* inst);
    std::vector<IRBasicBlock*> GetSuccessors() const;
    std::string DumpText() const override;
    std::string GetOperandName() const override;
    IRType GetType() const override;
    static bool classof(const IRValue* v) {
      return v && v->GetKind() == IRValueKind::IR_BASIC_BLOCK;
    }
  };

  // 库函数声明类
  class IRFuncDecl : public IRValue {
  public:
    std::string name;
    IRType ret_type;
    std::vector<IRType> param_types;
  public:
    IRFuncDecl(std::string name, IRType ret_type, std::vector<IRType> param_types);

    const std::string& GetName() const { return name; }
    std::string DumpText() const override;
    std::string GetOperandName() const override;
    IRType GetType() const override { return ret_type; }
    static bool classof(const IRValue* v) {
      return v && v->GetKind() == IRValueKind::IR_FUNC_DECL;
    }
  };

  // 函数类
  class IRFunction : public IRValue {
  public:
    std::string name;
    IRType ret_type;
    std::vector<std::shared_ptr<IRFuncParam>> params;
    std::vector<std::shared_ptr<IRBasicBlock>> blocks;

    IRFunction(std::string name, IRType ret_type) : IRValue(IRValueKind::IR_FUNCTION), name(StripIRNameSigil(std::move(name))), ret_type(ret_type) {}

    const std::string& GetName() const { return name; }
    std::string DumpText() const override;
    std::string GetOperandName() const override;
    IRType GetRetType() const { return ret_type; }
    std::vector<IRBasicBlock*> GetPredecessors(const IRBasicBlock* block) const;
    bool ReplaceAllUses(const IRValue* old_value,
                        std::shared_ptr<IRValue> new_value);
    IRType GetType() const override { return ret_type; }
    static bool classof(const IRValue* v) {
      return v && v->GetKind() == IRValueKind::IR_FUNCTION;
    }
  };

  // 程序根类
  class IRProgram {
  public:
    std::vector<std::shared_ptr<IRFuncDecl>> decls;
    std::vector<std::shared_ptr<IRGlobalAlloc>> globals;
    std::vector<std::shared_ptr<IRFunction>> funcs;

    std::string DumpText() const;
  };

  // isa implementation for raw pointers (using LLVM classof pattern)
  template <typename Target, typename Source>
  bool isa(const Source* val) {
    if (!val) return false;
    return Target::classof(static_cast<const IRValue*>(val));
  }

  // isa implementation for shared_ptr
  template <typename Target, typename Source>
  bool isa(const std::shared_ptr<Source>& val) {
    return isa<Target>(val.get());
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
    return const_cast<Target*>(static_cast<const Target*>(static_cast<const IRValue*>(val)));
  }

  template <typename Target, typename Source>
  const Target* cast(const Source* val) {
    assert(isa<Target>(val) && "Invalid cast!");
    return static_cast<const Target*>(static_cast<const IRValue*>(val));
  }

  // dynamic_cast_ptr implementation for shared_ptr
  template <typename Target, typename Source>
  std::shared_ptr<Target> dynamic_cast_ptr(const std::shared_ptr<Source>& val) {
    if (isa<Target>(val)) {
      return std::static_pointer_cast<Target>(val);
    }
    return nullptr;
  }

  // dyn_cast implementation for raw pointers (returns nullptr on failure)
  template <typename Target, typename Source>
  const Target* dyn_cast(const Source* val) {
    if (isa<Target>(val)) {
      return static_cast<const Target*>(static_cast<const IRValue*>(val));
    }
    return nullptr;
  }

  template <typename Target, typename Source>
  Target* dyn_cast(Source* val) {
    if (isa<Target>(val)) {
      return const_cast<Target*>(static_cast<const Target*>(static_cast<const IRValue*>(val)));
    }
    return nullptr;
  }
}

