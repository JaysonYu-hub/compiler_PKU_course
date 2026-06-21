#pragma once
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <variant>

class RegBuilder {
private:
  int reg_counter = 0; // 临时寄存器计数器
public:
  // 生成下一个可用的寄存器名字（例如 "%0", "%1"）
  std::string NewReg() {
    return "%" + std::to_string(reg_counter++);
  }
  std::string CurrentReg() const {
    return "%" + std::to_string(reg_counter);
  }
  void Reset() {
    reg_counter = 0;
  }

  std::string GetAddress(const std::string &var_name) const {
    return "@" + var_name;
  }
};

class NumBuilder {
private:
  inline static int num_counter = 0; // 数字计数器
public:
  void static AddNum() {
    num_counter++;
  }
  void static Reset() {
    num_counter = 0;
  }
  std::string static currentNum() {
    return std::to_string(num_counter);
  }
};

class LabelBuilder {
public:
  std::string static GetLabel(std::string type, std::string cond) {
    return "%" + type + "_" + cond.erase(0, 1) ;
  }
};

//符号表
enum class SymbolKind { CONSTANT , VARIABLE };

struct Symbol {
public:
  std::string name;
  SymbolKind kind;
  std::variant<int, std::string> value; // 对于变量为address, 对于常量为const_value
  Symbol(const std::string &name, SymbolKind kind, int const_value) : name(name), kind(kind), value(const_value) {}

  Symbol(const std::string &name, SymbolKind kind, const std::string &address) : name(name), kind(kind), value(address) {}
  Symbol(const std::string &name, SymbolKind kind) : name(name), kind(kind), value(0) {}
};

class Scope {
private:
  std::unordered_map<std::string, std::shared_ptr<Symbol>> symbols;
  std::shared_ptr<Scope> parentScope;
  std::string ident;
public:
  Scope(std::shared_ptr<Scope> parentScope) : parentScope(parentScope) {
    NumBuilder::AddNum();
    ident = NumBuilder::currentNum();
  }

  std::shared_ptr<Scope> getParentScope() const { return parentScope; }

  bool define(std::shared_ptr<Symbol> symbol) {
    if (symbols.find(symbol->name) != symbols.end()) {
      return false;
    }
    symbols[symbol->name] = symbol;
    return true;
  }
  std::shared_ptr<Symbol> lookup(const std::string &name) {
    auto current = symbols.find(name);
    if (current != symbols.end()) {
      return current->second;
    }
    if (parentScope != nullptr) {
      return parentScope->lookup(name);
    }
    return nullptr;
  }

  std::shared_ptr<Symbol> lookupLocal(const std::string &name) {
    auto it = symbols.find(name);
    if (it != symbols.end()) {
      return it->second;
    }
    return nullptr;
  }

  void print(int depth = 0) const {
    std::string indent(depth * 2, ' ');
    std::clog << indent << "=== [Scope 层级 " << depth << "] 地址: " << this << " ===" << std::endl;
    if (symbols.empty()) {
      std::clog << indent << "  (当前层没有符号)" << std::endl;
    } else {
      // 打印当前层的所有符号
      for (const auto& pair : symbols) {
        const auto& name = pair.first;
        const auto& sym = pair.second;

        std::clog << indent << "  -> 名字: " << name;

        // 打印符号类型 (假设你的 SymbolKind 是枚举)
        std::clog << " | 类型: ";
        if (sym->kind == SymbolKind::CONSTANT) {
          std::clog << "CONSTANT (值: " << std::get<int>(sym->value) << ")";
        }
        std::clog << std::endl;
      }
    }

    // 如果有父作用域，递归打印父作用域
    if (parentScope != nullptr) {
      std::clog << indent << "  --> 指向父作用域..." << std::endl;
      parentScope->print(depth + 1);
    } else {
      std::clog << indent << "=== [Scope 根节点结束] ===" << std::endl;
    }
  }
  std::string getScopeIdent() {
    return ident;
  }
};

// 上下文：用于传递RegBuilder和作用域符号表
class Context {
public:
  RegBuilder reg_builder;
  std::shared_ptr<Scope> scope;
  bool cur_block_has_terminator = false;

  Context(const RegBuilder &reg_builder) : reg_builder(reg_builder), scope(std::make_shared<Scope>(nullptr)) {}

  void enterNewScope() {
    scope = std::make_shared<Scope>(scope);
  }
  void exitScope() {
    if (scope->getParentScope() != nullptr) {
      scope = scope->getParentScope();
    } else {
      std::cout << "Cannot exit global scope";
      return;
    }
  }

  void Reset() {
    reg_builder.Reset();
  }

  std::string GetAddress(const std::string &var_name) {
    return reg_builder.GetAddress(var_name) + "_" + scope->getScopeIdent();
  }

  std::string NewReg() {
    return reg_builder.NewReg();
  }

  std::string CurrentReg() const {
    return reg_builder.CurrentReg();
  }

  void ResetBlockTerminator() {
    cur_block_has_terminator = false;
  }
};

class OpAST;
class LValAST;
class BaseAST {
public:
  virtual ~BaseAST() = default;
  virtual std::string Dump(Context& context) const = 0;
};

class ValueAST : public BaseAST {
public:
  virtual ~ValueAST() = default;
  virtual std::string Dump(Context& context) const = 0;
  virtual int getValue(Context& context) const = 0;
};

// CompUnit ::= FuncDef
class CompUnitAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> func_def;

  CompUnitAST() = default; 
  std::string Dump(Context& context) const override;
};

// FuncDef ::= FuncType IDENT '(' ')' Block
class FuncDefAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> func_type;
  std::unique_ptr<std::string> ident;
  std::unique_ptr<BaseAST> block;
    
  FuncDefAST() = default; 

  std::string Dump(Context& context) const override;
};

// FuncType ::= 'int'
class FuncTypeAST : public BaseAST {
public:
  std::string type_name; 
    
  FuncTypeAST() = default; 

  std::string Dump(Context& context) const override;
};

// BlockItem ::= Decl | Stmt
class DeclBloAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> decl;
  DeclBloAST() = default; 

  std::string Dump(Context& context) const override;
};

class StmtBloAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> stmt;
  StmtBloAST() = default; 

  std::string Dump(Context& context) const override;
};

// Block ::= '{' {BlockItem} '}'
class BlockAST : public BaseAST {
public:
  std::vector<std::unique_ptr<BaseAST>> block_items; 

  BlockAST() = default; 

  std::string Dump(Context& context) const override;
};

// Stmt ::= UnmatchedStmt | MatchedStmt
class UnmatchedStmtStmtAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> unmatched_stmt;
  UnmatchedStmtStmtAST() = default; 

  std::string Dump(Context& context) const override;
};

class MatchedStmtStmtAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> matched_stmt;
  MatchedStmtStmtAST() = default; 

  std::string Dump(Context& context) const override;
};

// UnmatchedStmt ::= IF '(' Exp ')' Stmt | IF '(' Exp ')' MatchedStmt ELSE UnMatchedStmt
class IfUnmatchedStmtAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> if_exp;
  std::unique_ptr<BaseAST> if_stmt;
  std::unique_ptr<BaseAST> else_stmt;

  IfUnmatchedStmtAST() = default; 

  std::string Dump(Context& context) const override;
};

class IfElseUnmatchedStmtAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> if_exp;
  std::unique_ptr<BaseAST> if_stmt;
  std::unique_ptr<BaseAST> else_stmt;

  IfElseUnmatchedStmtAST() = default; 

  std::string Dump(Context& context) const override;
};

// MatchedStmt ::= 'return' Exp ';' | LVal '=' Exp ';' | EXP ';' | ';' | Block |IF '(' Exp ')' MatchedStmt else MatchedStmt
class ReturnStmtAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> exp;

  ReturnStmtAST() = default; 

  std::string Dump(Context& context) const override;
};

class AssignStmtAST : public BaseAST {
public:
  std::unique_ptr<LValAST> lval;
  std::unique_ptr<ValueAST> exp;

  AssignStmtAST() = default; 

  std::string Dump(Context& context) const override;
};

class ExpStmtAST : public BaseAST {
public:
  std::unique_ptr<ValueAST> exp;

  ExpStmtAST() = default; 

  std::string Dump(Context& context) const override;
};

class EmptyStmtAST : public BaseAST {
public:
  EmptyStmtAST() = default; 

  std::string Dump(Context& context) const override;
};

class BlockStmtAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> block;

  BlockStmtAST() = default; 

  std::string Dump(Context& context) const override;
};

class IfMatchedStmtAST : public BaseAST {
public:
  std::unique_ptr<ValueAST> if_exp;
  std::unique_ptr<BaseAST> if_stmt;
  std::unique_ptr<BaseAST> else_stmt;

  IfMatchedStmtAST() = default; 

  std::string Dump(Context& context) const override;
};

// Exp ::= LOrExp
class ExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> lor_exp;

  ExpAST() = default;
  std::string Dump(Context& context) const override;
  int getValue(Context& context) const override;
};

// UnaryExp ::= UnaryOP UnaryExp | PrimaryExp
class UnaryOPExpAST : public ValueAST {
public:
  std::unique_ptr<OpAST> op;
  std::unique_ptr<ValueAST> unary_exp;

  UnaryOPExpAST() = default; 
  std::string Dump(Context& context) const override;
  int getValue(Context& context) const override;
};

class PrimaryUnaryExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> primary_exp;

  PrimaryUnaryExpAST() = default; 

  std::string Dump(Context& context) const override;
  int getValue(Context& context) const override;
};

// PrimaryExp ::= '(' Exp ')' | LVal | Number
class ThreePriAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> exp;
  ThreePriAST() = default; 

  std::string Dump(Context& context) const override;
  int getValue(Context& context) const override;
};

class LValPriAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> lval;
  LValPriAST() = default; 

  std::string Dump(Context& context) const override;
  int getValue(Context& context) const override;
};

class NumberPriAST : public ValueAST {
public:
  int value;
  NumberPriAST() = default; 

  std::string Dump(Context& context) const override;
  int getValue(Context& context) const override;
};

//'+' | '-' | '!' | '*' | '/' | '%'
class OpAST : public BaseAST {
public:
  std::string op;

  OpAST() = default; 
  std::string Dump(Context& context) const override;
};

// MulExp ::= UnaryExp | MulExp ('*' | '/' | '%') UnaryExp
class UnaryMulExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> unary_exp;

  UnaryMulExpAST() = default; 
  std::string Dump(Context& context) const override;
  int getValue(Context& context) const override;
};

class ThreeMulExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> mul_exp;
  std::unique_ptr<OpAST> op; // '*' | '/' | '%'
  std::unique_ptr<ValueAST> unary_exp;

  ThreeMulExpAST() = default; 
  std::string Dump(Context& context) const override;
  int getValue(Context& context) const override;
};

// AddExp ::= MulExp | AddExp ('+' | '-') MulExp
class MulAddExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> mul_exp;

  MulAddExpAST() = default; 
  std::string Dump(Context& context) const override;
  int getValue(Context& context) const override;
};

class ThreeAddExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> add_exp;
  std::unique_ptr<OpAST> op; // '+' or '-'
  std::unique_ptr<ValueAST> mul_exp;

  ThreeAddExpAST() = default; 
  std::string Dump(Context& context) const override; 
  int getValue(Context& context) const override;
};

// RelExp ::= AddExp | RelExp ('<' | '>' | '<=' | '>=') AddExp
class AddRelExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> add_exp;

  AddRelExpAST() = default; 
  std::string Dump(Context& context) const override;
  int getValue(Context& context) const override;
};

class ThreeRelExpAST : public ValueAST {
public: 
  std::unique_ptr<ValueAST> rel_exp;
  std::unique_ptr<OpAST> op; // '<' | '>' | '<=' | '>='
  std::unique_ptr<ValueAST> add_exp;

  ThreeRelExpAST() = default; 
  std::string Dump(Context& context) const override; 
  int getValue(Context& context) const override;
};

// EqExp ::= RelExp | EqExp ('==' | '!=') RelExp
class RelEqExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> rel_exp;

  RelEqExpAST() = default; 
  std::string Dump(Context& context) const override;
  int getValue(Context& context) const override;
};

class ThreeEqExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> eq_exp;
  std::unique_ptr<OpAST> op; // '==' | '!='
  std::unique_ptr<ValueAST> rel_exp;

  ThreeEqExpAST() = default; 
  std::string Dump(Context& context) const override; 
  int getValue(Context& context) const override;
};

// LAndExp ::= EqExp | LAndExp '&&' EqExp
class EqLAndExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> eq_exp;

  EqLAndExpAST() = default; 
  std::string Dump(Context& context) const override;
  int getValue(Context& context) const override;
};

class ThreeLAndExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> land_exp;
  std::unique_ptr<OpAST> op; // '&&'
  std::unique_ptr<ValueAST> eq_exp;

  ThreeLAndExpAST() = default; 
  std::string Dump(Context& context) const override; 
  int getValue(Context& context) const override;
};

// LOrExp ::= LAndExp | LOrExp '||' LAndExp
class LAndLOrExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> land_exp;

  LAndLOrExpAST() = default; 
  std::string Dump(Context& context) const override;
  int getValue(Context& context) const override;
};

class ThreeLOrExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> lor_exp;
  std::unique_ptr<OpAST> op; // '||'
  std::unique_ptr<ValueAST> land_exp;

  ThreeLOrExpAST() = default; 
  std::string Dump(Context& context) const override;  
  int getValue(Context& context) const override;
};  

// LVal = IDENT
class LValAST : public ValueAST {
public:
  std::unique_ptr<std::string> ident;
  LValAST() = default; 

  std::string Dump(Context& context) const override;
  // 用于仅返回变量地址而不加载
  std::string DumpOnlyAddress(Context& context) const;
  int getValue(Context& context) const override;
};

// ConstExp ::= Exp
class ConstExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> exp;
  ConstExpAST() = default; 
  std::string Dump(Context& context) const override;
  int getValue(Context& context) const override;
};


// ConstInitVal ::= ConstExp
class ConstInitValAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> const_exp;
  ConstInitValAST() = default; 

  std::string Dump(Context& context) const override;
  int getValue(Context& context) const override;
};

// ConstDef ::= IDENT '=' ConstInitVal
class ConstDefAST : public BaseAST {
public:
  std::unique_ptr<std::string> ident;
  std::unique_ptr<ValueAST> const_init_val;
  ConstDefAST() = default;

  std::string Dump(Context& context) const override;
};

// Decl ::= ConstDecl | VarDecl
class ConstDeclDecAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> const_decl;
  ConstDeclDecAST() = default; 

  std::string Dump(Context& context) const override;
};

class VarDeclDecAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> var_decl;
  VarDeclDecAST() = default; 

  std::string Dump(Context& context) const override;
};

// ConstDecl ::= 'const' BType ConstDef { ',' ConstDef } ';'
class ConstDeclAST : public BaseAST {
public:
  std::unique_ptr<std::string> b_type;
  std::unique_ptr<BaseAST> const_def_list;
  ConstDeclAST() = default; 

  std::string Dump(Context& context) const override;
};

class ConstDefListAST : public BaseAST {
public:
  std::vector<std::unique_ptr<BaseAST>> const_defs;
  ConstDefListAST() = default; 

  std::string Dump(Context& context) const override;
};

// VarDecl ::= BType VarDef { ',' VarDef } ';'
class VarDeclAST : public BaseAST {
public:
  std::unique_ptr<std::string> b_type;
  std::unique_ptr<BaseAST> var_def_list;
  VarDeclAST() = default; 

  std::string Dump(Context& context) const override;
};

class VarDefListAST : public BaseAST {
public:
  std::vector<std::unique_ptr<BaseAST>> var_defs;
  VarDefListAST() = default; 

  std::string Dump(Context& context) const override;
};

// VarDef ::= IDENT | IDENT '=`' InitVal;
// 这里单独的IDENT也被称为变量声明
class IDENTVarAST : public BaseAST {
public:
  std::unique_ptr<std::string> ident;
  IDENTVarAST() = default; 

  std::string Dump(Context& context) const override;
};

class ThreeVarAST : public BaseAST {
public:
  std::unique_ptr<std::string> ident;
  std::unique_ptr<ValueAST> init_val;
  ThreeVarAST() = default; 

  std::string Dump(Context& context) const override;
};

// InitVal ::= Exp
class InitValAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> exp;
  InitValAST() = default; 

  std::string Dump(Context& context) const override;
  int getValue(Context& context) const override;
};

