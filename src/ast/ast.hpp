#pragma once
#include <memory>
#include <string>

class IRBuilder {
private:
  int reg_counter = 0; // 临时寄存器计数器
public:
  // 生成下一个可用的寄存器名字（例如 "%0", "%1"）
  std::string NewReg() {
    return "%" + std::to_string(reg_counter++);
  }
  // 仅查看当前计数器值
  std::string CurrentReg() const {
    return "%" + std::to_string(reg_counter);
  }
  // 每当开始解析一个新函数时清零计数器
  void Reset() {
    reg_counter = 0;
  }
};

class UnaryExpAST;
class OpAST;

class BaseAST {
public:
  virtual ~BaseAST() = default;
  virtual std::string Dump(IRBuilder& builder) const = 0;
};

// CompUnit ::= FuncDef
class CompUnitAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> func_def;

  CompUnitAST() = default; 
  std::string Dump(IRBuilder& builder) const override;
};

// FuncDef ::= FuncType IDENT '(' ')' Block
class FuncDefAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> func_type;
  std::unique_ptr<std::string> ident;
  std::unique_ptr<BaseAST> block;
    
  FuncDefAST() = default; 

  std::string Dump(IRBuilder& builder) const override;
};

// FuncType ::= 'int'
class FuncTypeAST : public BaseAST {
public:
  std::string type_name; 
    
  FuncTypeAST() = default; 

  std::string Dump(IRBuilder& builder) const override;
};

// Block ::= '{' Stmt '}'
class BlockAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> stmt;

  BlockAST() = default;

  std::string Dump(IRBuilder& builder) const override;
};

// Stmt ::= 'return' Exp ';'
class StmtAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> exp;

  StmtAST() = default; 

  std::string Dump(IRBuilder& builder) const override;
};

// Exp ::= AddExp
class ExpAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> add_exp;

  ExpAST() = default;

  std::string Dump(IRBuilder& builder) const override;
};

// 中间抽象层：UnaryExp 基类
class UnaryExpAST : public BaseAST {
public:
  virtual ~UnaryExpAST() = default;
  std::string Dump(IRBuilder& builder) const override = 0; 
};

// UnaryExp ::= UnaryOP UnaryExp
class UnaryOPExpAST : public UnaryExpAST {
public:
  std::unique_ptr<OpAST> op;
  std::unique_ptr<UnaryExpAST> unary_exp;

  UnaryOPExpAST() = default; 
  std::string Dump(IRBuilder& builder) const override;
};

// 中间抽象层：PrimaryExp 基类
class PrimaryExpAST : public UnaryExpAST {
public:
  virtual ~PrimaryExpAST() = default;
  std::string Dump(IRBuilder& builder) const override = 0;
};

// PrimaryExp ::= '(' Exp ')'
class ParenExprAST : public PrimaryExpAST {
public:
  std::unique_ptr<BaseAST> exp;
  
  ParenExprAST() = default; 

  std::string Dump(IRBuilder& builder) const override;
};

// PrimaryExp ::= Number
class NumberExpAST : public PrimaryExpAST {
public:
  int value;

  NumberExpAST() = default; 

  std::string Dump(IRBuilder& builder) const override;
};

//'+' | '-' | '!' | '*' | '/' | '%'
class OpAST : public BaseAST {
public:
  std::string op;

  OpAST() = default; 

  std::string Dump(IRBuilder& builder) const override;
};

// MulExp ::= UnaryExp | MulExp ('*' | '/' | '%') UnaryExp
// 中间抽象层：MulExp 基类
class MulExpAST : public UnaryExpAST {
public:
  virtual ~MulExpAST() = default;
  std::string Dump(IRBuilder& builder) const override = 0;
}
;

class UnaryMulExpAST : public MulExpAST {
public:
  std::unique_ptr<UnaryExpAST> unary_exp;

  UnaryMulExpAST() = default; 

  std::string Dump(IRBuilder& builder) const override;
};

class ThreeMulExpAST : public MulExpAST {
public:
  std::unique_ptr<MulExpAST> mul_exp;
  std::unique_ptr<OpAST> op; // '*' | '/' | '%'
  std::unique_ptr<UnaryExpAST> unary_exp;

  ThreeMulExpAST() = default; 

  std::string Dump(IRBuilder& builder) const override;
};

// AddExp ::= MulExp | AddExp ('+' | '-') MulExp
// 中间抽象层：AddExp 基类
class AddExpAST : public UnaryExpAST {
public:
  virtual ~AddExpAST() = default;
  std::string Dump(IRBuilder& builder) const override = 0;
};

class MulAddExpAST : public AddExpAST {
public:
  std::unique_ptr<MulExpAST> mul_exp;

  MulAddExpAST() = default; 

  std::string Dump(IRBuilder& builder) const override;
};

class ThreeAddExpAST : public AddExpAST {
public:
  std::unique_ptr<AddExpAST> add_exp;
  std::unique_ptr<OpAST> op; // '+' or '-'
  std::unique_ptr<MulExpAST> mul_exp;

  ThreeAddExpAST() = default; 

  std::string Dump(IRBuilder& builder) const override; 
};