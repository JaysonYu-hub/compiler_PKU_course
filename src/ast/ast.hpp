#pragma once
#include <memory>
#include <string>
#include <vector>

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

//符号表
enum class SymbolKind { CONSTANT };

class Symbol { 
public:
  string name;
  SymbolKind kind;
  int const_value;
  Symbol(string name, SymbolKind kind, int const_value) : name(name), kind(kind), const_value(const_value) {}
};

class Scope {
private:
  unordered_map<string, shared_ptr<Symbol>> symbols;
  shared_ptr<Scope> parentScope;
public:
  Scope(shared_ptr<Scope> parentScope) : parentScope(parentScope) {}

  shared_ptr<Scope> getParentScope() const { return parentScope; }

  bool define(shared_ptr<Symbol> symbol) {
    if (symbols.find(symbol->name) != symbols.end()) {
      return false;
    }
    symbols[symbol->name] = symbol;
    return true;
  }
  shared_ptr<Symbol> lookup(string name) {
    auto current = symbols.find(name);
    if (current != symbols.end()) {
      return current->second;
    }
    if (parentScope != nullptr) {
      return parentScope->lookup(name);
    }
    return nullptr;
  }

  shared_ptr<Symbol> lookupLocal(string name) {
    auto it = symbols.find(name);
    if (it != symbols.end()) {
      return it->second;
    }
    return nullptr;
  }
};

// 用于处理局部变量 
class CompilerState {
private: 
  CompilerState() = default;
public:
  static CompilerState& GetInstance() { 
    static CompilerState instance;
    return instance;
  }
  unordered_map<string, int> const_var_map;
  int get_const_var(string const_ident) {
    return const_var_map[const_ident];
  }
  void set_const_var(string const_ident, int const_value) {
    const_var_map[const_ident] = const_value;
  }
};

class UnaryExpAST;
class OpAST;

class BaseAST {
public:
  virtual ~BaseAST() = default;
  virtual std::string Dump(IRBuilder& builder) const = 0;
};

class ValueAST : public BaseAST {
public:
  virtual ~ValueAST() = default;
  virtual std::string Dump(IRBuilder& builder) const = 0;
  virtual int getValue() const = 0;
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

// BlockItem ::= Decl | Stmt
// 中间抽象层：BlockItem 基类
class BlockItemAST : public BaseAST {
public:
  virtual ~BlockItemAST() = default;
  std::string Dump(IRBuilder& builder) const override = 0;
};

class DeclBloAST : public BlockItemAST {
public:
  std::unique_ptr<BaseAST> decl;
  DeclBloAST() = default; 

  std::string Dump(IRBuilder& builder) const override;
};

class StmtBloAST : public BlockItemAST {
public:
  std::unique_ptr<StmtAST> stmt;
  StmtBloAST() = default; 

  std::string Dump(IRBuilder& builder) const override;
};

// Block ::= '{' {BlockItem} '}'
class BlockAST : public BaseAST {
public:
  std::vector<unique_ptr<BaseAST>> block_items;

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

// Exp ::= LOrExp
class ExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> lor_exp;

  ExpAST() = default;
  std::string Dump(IRBuilder& builder) const override;
  int getValue() const override;
};

// UnaryExp ::= UnaryOP UnaryExp | PrimaryExp
class UnaryOPExpAST : public ValueAST {
public:
  std::unique_ptr<OpAST> op;
  std::unique_ptr<ValueAST> unary_exp;

  UnaryOPExpAST() = default; 
  std::string Dump(IRBuilder& builder) const override;
  int getValue() const override;
};

// PrimaryExp ::= '(' Exp ')' | LVal | Number
class ThreePriAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> exp;
  ThreePriAST() = default; 

  std::string Dump(IRBuilder& builder) const override;
  int getValue() const override;
};

class LValPriAST : public ValueAST {
public:
  std::unique_ptr<std::string> ident;
  LValPriAST() = default; 

  std::string Dump(IRBuilder& builder) const override;
  int getValue() const override;
};

class NumberPriAST : public ValueAST {
public:
  int value;
  NumberPriAST() = default; 

  std::string Dump(IRBuilder& builder) const override;
  int getValue() const override;
};

//'+' | '-' | '!' | '*' | '/' | '%'
class OpAST : public BaseAST {
public:
  std::string op;

  OpAST() = default; 
  std::string Dump(IRBuilder& builder) const override;
};

// MulExp ::= UnaryExp | MulExp ('*' | '/' | '%') UnaryExp
class UnaryMulExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> unary_exp;

  UnaryMulExpAST() = default; 
  std::string Dump(IRBuilder& builder) const override;
  int getValue() const override;
};

class ThreeMulExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> mul_exp;
  std::unique_ptr<OpAST> op; // '*' | '/' | '%'
  std::unique_ptr<ValueAST> unary_exp;

  ThreeMulExpAST() = default; 
  std::string Dump(IRBuilder& builder) const override;
  int getValue() const override;
};

// AddExp ::= MulExp | AddExp ('+' | '-') MulExp
class MulAddExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> mul_exp;

  MulAddExpAST() = default; 
  std::string Dump(IRBuilder& builder) const override;
  int getValue() const override;
};

class ThreeAddExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> add_exp;
  std::unique_ptr<OpAST> op; // '+' or '-'
  std::unique_ptr<ValueAST> mul_exp;

  ThreeAddExpAST() = default; 
  std::string Dump(IRBuilder& builder) const override; 
  int getValue() const override;
};

// RelExp ::= AddExp | RelExp ('<' | '>' | '<=' | '>=') AddExp
class AddRelExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> add_exp;

  AddRelExpAST() = default; 
  std::string Dump(IRBuilder& builder) const override;
  int getValue() const override;
};

class ThreeRelExpAST : public ValueAST {
public: 
  std::unique_ptr<ValueAST> rel_exp;
  std::unique_ptr<OpAST> op; // '<' | '>' | '<=' | '>='
  std::unique_ptr<ValueAST> add_exp;

  ThreeRelExpAST() = default; 
  std::string Dump(IRBuilder& builder) const override; 
  int getValue() const override;
};

// EqExp ::= RelExp | EqExp ('==' | '!=') RelExp
class RelEqExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> rel_exp;

  RelEqExpAST() = default; 
  std::string Dump(IRBuilder& builder) const override;
  int getValue() const override;
};

class ThreeEqExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> eq_exp;
  std::unique_ptr<OpAST> op; // '==' | '!='
  std::unique_ptr<ValueAST> rel_exp;

  ThreeEqExpAST() = default; 
  std::string Dump(IRBuilder& builder) const override; 
  int getValue() const override;
};

// LAndExp ::= EqExp | LAndExp '&&' EqExp
class EqLAndExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> eq_exp;

  EqLAndExpAST() = default; 
  std::string Dump(IRBuilder& builder) const override;
  int getValue() const override;
};

class ThreeLAndExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> land_exp;
  std::unique_ptr<OpAST> op; // '&&'
  std::unique_ptr<ValueAST> eq_exp;

  ThreeLAndExpAST() = default; 
  std::string Dump(IRBuilder& builder) const override; 
  int getValue() const override;
};

// LOrExp ::= LAndExp | LOrExp '||' LAndExp
class LAndLOrExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> land_exp;

  LAndLOrExpAST() = default; 
  std::string Dump(IRBuilder& builder) const override;
  int getValue() const override;
};

class ThreeLOrExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> lor_exp;
  std::unique_ptr<OpAST> op; // '||'
  std::unique_ptr<ValueAST> land_exp;

  ThreeLOrExpAST() = default; 
  std::string Dump(IRBuilder& builder) const override;  
  int getValue() const override;
};  

// ConstExp ::= Exp
class ConstExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> exp;
  ConstExpAST() = default; 
  std::string Dump(IRBuilder& builder) const override;
  int getValue() const override;
};


// ConstInitVal ::= ConstExp
class ConstInitValAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> const_exp;
  ConstInitValAST() = default; 

  std::string Dump(IRBuilder& builder) const override;
  int getValue() const override;
};

// ConstDef ::= IDENT '=' ConstInitVal
class ConstDefAST : public BaseAST {
public:
  std::unique_ptr<std::string> ident;
  std::unique_ptr<BaseAST> const_init_val;
  ConstDefAST() = default;

  std::string Dump(IRBuilder& builder) const override;
};

// Decl ::= ConstDecl
class DeclAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> const_decl;
  DeclAST() = default; 

  std::string Dump(IRBuilder& builder) const override;
};

// ConstDecl ::= 'const' BType ConstDef { ',' ConstDef } ';'
class ConstDeclAST : public BaseAST {
public:
  std::unique_ptr<string> b_type;
  std::unique_ptr<BaseAST> const_def_list;
  ConstDeclAST() = default; 

  std::string Dump(IRBuilder& builder) const override;
};

class ConstDefListAST : public BaseAST {
public:
  std::vector<std::unique_ptr<BaseAST>> const_defs;
  ConstDefListAST() = default; 

  std::string Dump(IRBuilder& builder) const override;
};