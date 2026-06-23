#pragma once
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <variant>
#include <assert.h>

// 临时寄存器生成器, 在函数开始时reset
class Counter {
private:
  int counter = 0;
public:
  std::string NewNum() {
    return std::to_string(counter++);
  }
  std::string CurrentNum() const {
    return std::to_string(counter);
  }
  void Reset() {
    counter = 0;
  }
};

/**  全局计数器
 **/
class NumBuilder {
private:
  inline static int num_counter = 0;
public:
  void static Reset() {
    num_counter = 0;
  }
  std::string static currentNum() {
    return std::to_string(num_counter);
  }

  std::string static GetNum() {
    num_counter++;
    return std::to_string(num_counter);
  }
};

/**
 * 标签生成工具
 * 支持
 * 1. type + cond + globalnum
 * 2. type + globalnum
*/
class LabelBuilder {
public:
  // 返回标签，"%" + type + "_" + cond.erase(0, 1) + "_" + NumBuilder::GetNum()
  std::string static GetLabel(std::string type, std::string cond) {
    return "%" + type + "_" + cond.erase(0, 1) + "_" + NumBuilder::GetNum();
  }
  // 返回标签，"%" + type + "_" + NumBuilder::GetNum()
  std::string static GetLabel(std::string type) {
    return "%" + type + "_" + NumBuilder::GetNum();
  }
};


class FuncInformation { 
  public:
  std::string ret_type;
  FuncInformation() : ret_type("") {}
  FuncInformation(std::string ret_type) : ret_type(ret_type) {}
};

enum class SymbolKind { CONSTANT, VARIABLE, FUNCTION };

/**
 * 符号表
 * 符号表是一个字典，键为变量名，值为Symbol
 * Symbol为符号表项，包含变量名、变量类型、变量值
 * 对于变量，变量值为address
 * 对于常量，变量值为const_value
*/
struct Symbol {
public:
  std::string name;
  SymbolKind kind;
  std::variant<int, std::string, FuncInformation> value;
  Symbol(const std::string &name, SymbolKind kind, int const_value) : name(name), kind(kind), value(const_value) {}
  Symbol(const std::string &name, SymbolKind kind, const std::string &address) : name(name), kind(kind), value(address) {}

  Symbol(const std::string &name, SymbolKind kind, FuncInformation func_information) : name(name), kind(kind), value(func_information) {}
  Symbol(const std::string &name, SymbolKind kind) : name(name), kind(kind) {
    if (kind == SymbolKind::CONSTANT) {
      value = 0;
    } else if (kind == SymbolKind::FUNCTION) {
      value = FuncInformation();
    } else if (kind == SymbolKind::VARIABLE) {
      value = "";
    } else {
      assert(false);
    }
  }
};

/**
 * 作用域
 * 作用域是一个字典，键为变量名，值为Symbol
 * 作用域的父作用域为parentScope
*/
class Scope {
private:
  std::unordered_map<std::string, std::shared_ptr<Symbol>> symbols;
  std::shared_ptr<Scope> parentScope;
  std::string ident;
public:
  Scope(Counter& scope_ident_builder) : parentScope(nullptr), ident("global") {}
  Scope(std::shared_ptr<Scope> parentScope, std::string ident) 
  : parentScope(parentScope), ident(ident) {}

  std::shared_ptr<Scope> getParentScope() const { return parentScope; }

  void define(std::shared_ptr<Symbol> symbol) {
    if (symbols.find(symbol->name) != symbols.end()) {
      std::cerr << "error: " << symbol->name << " had been defined" << std::endl;   
    }
    symbols[symbol->name] = symbol;
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

class WhileLabelContext {
public:
  std::string while_label_begin;
  std::string while_label_continue;
  std::string while_label_end;
  WhileLabelContext(std::string while_label_begin, 
    std::string while_label_continue, 
    std::string while_label_end)
    : while_label_begin(while_label_begin), 
    while_label_continue(while_label_continue), 
    while_label_end(while_label_end) {}
  WhileLabelContext() : while_label_begin(""), while_label_continue(""), while_label_end("") {}
};

// 上下文：用于提供RegBuilder接口和作用域符号表
class Context {
public:
  Counter reg_builder;
  Counter scope_ident_builder;
  std::shared_ptr<Scope> scope;
  bool cur_block_has_terminator = false;
  std::vector<WhileLabelContext> while_label_contexts;

  Context() : reg_builder(), scope(std::make_shared<Scope>(scope_ident_builder)) {}
  void enterNewScope() {
    //std::cout << "// 进入新的作用域" << std::endl;
    std::string new_ident = scope_ident_builder.NewNum();
    scope = std::make_shared<Scope>(scope, new_ident);
  }
  void exitScope() {
    if (scope->getParentScope() != nullptr) {
      scope = scope->getParentScope();
      //std::cout << "// 退出作用域" << std::endl;
    } else {
      std::cout << "Cannot exit global scope";
      return;
    }
  }

  void enterNewWhileScope(std::string while_label_begin, 
    std::string while_label_continue, 
    std::string while_label_end) { 
    while_label_contexts.push_back(WhileLabelContext(while_label_begin, while_label_continue, while_label_end));
  }

  void exitWhileScope() { 
    if (while_label_contexts.empty()) {
      std::cerr << "Cannot exit while scope" << std::endl;
      return;
    }
    while_label_contexts.pop_back();
  }

  WhileLabelContext GetWhileLabel() {
    if (while_label_contexts.empty()) {
      std::cerr << "while_label_context is empty" << std::endl;
      return WhileLabelContext();
    }
    return while_label_contexts.back();
  }
  void ResetRegBuilder() {
    reg_builder.Reset();
  }
  // return "@" + var_name + "_" + scope_ident
  std::string GetAddress(const std::string &var_name) {
    return "@" + var_name + "_" + scope->getScopeIdent();
  }
  std::string NewReg() {
    return "%" + reg_builder.NewNum();
  }
  std::string CurrentReg() const {
    return "%" + reg_builder.CurrentNum();
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

// CompUnit ::= FuncDef | CompUnit FuncDef
class FuncDefCompUnitAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> func_def;

  FuncDefCompUnitAST() = default; 
  std::string Dump(Context& context) const override;
};

class TwoCompUnitAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> comp_unit;
  std::unique_ptr<BaseAST> func_def;

  TwoCompUnitAST() = default; 
  std::string Dump(Context& context) const override;
};

class FuncTypeAST;
class FuncFParamsAST;
// FuncDef ::= FuncType IDENT '(' [FuncFParams] ')' Block
class FuncDefAST : public BaseAST {
public:
  // 这里dump需要访问func_type
  std::unique_ptr<FuncTypeAST> func_type;
  std::unique_ptr<std::string> ident;
  std::unique_ptr<BaseAST> block;
  std::unique_ptr<FuncFParamsAST> func_f_params;
  FuncDefAST() = default; 

  std::string Dump(Context& context) const override;
};

// FuncFParam ::= BType IDENT ';'
class FuncFParamAST : public BaseAST {
public:
  std::unique_ptr<std::string> btype;
  std::unique_ptr<std::string> ident;

  FuncFParamAST() = default; 

  std::string Dump(Context& context) const override;
  void ParamLocalization(Context& context);
};

// FuncFParams ::= FuncFParam { ',' FuncFParam }
class FuncFParamsAST : public BaseAST {
public:
  //为空就初始化
  std::vector<std::unique_ptr<FuncFParamAST>> func_f_params; 
  FuncFParamsAST() = default; 
  std::string Dump(Context& context) const override;
  void ParamLocalization(Context& context);
};


// FuncType ::= INT | VOID
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

// UnmatchedStmt ::= IF '(' Exp ')' Stmt | IF '(' Exp ')' MatchedStmt ELSE UnMatchedStmt | WHILE '(' Exp ')' UnmatchedStmt
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

/**  MatchedStmt ::= RETURN [Exp] ';' 
 * | LVal '=' Exp ';' 
 * | EXP ';' 
 * | ';' 
 * | Block 
 * | IF '(' Exp ')' MatchedStmt else MatchedStmt
 * | WHILE '(' Exp ')' Stmt
 * | CONTINUE ';'
 * | BREAK ';'
 *  */
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

class WhileStmtAST : public BaseAST {
public:
  std::unique_ptr<ValueAST> while_exp;
  std::unique_ptr<BaseAST> while_stmt;

  WhileStmtAST() = default; 

  std::string Dump(Context& context) const override;
};

class ContinueStmtAST : public BaseAST {
public:
  ContinueStmtAST() = default;
  std::string Dump(Context& context) const override;
};

class BreakStmtAST : public BaseAST {
public:
  BreakStmtAST() = default;
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

// UnaryExp ::= UnaryOP UnaryExp | PrimaryExp | IDENT '(' [FuncRParams] ')'
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

class FuncCallExpAST : public ValueAST {
public:
  std::unique_ptr<std::string> ident;
  std::unique_ptr<BaseAST> func_r_params;

  FuncCallExpAST() = default; 

  std::string Dump(Context& context) const override;
  // 这里的getValue要扔出报错
  int getValue(Context& context) const override;
};

// FuncRParams ::= Exp { ',' Exp }
class FuncRParamsAST : public BaseAST {
public:
  std::vector<std::unique_ptr<ValueAST>> func_r_params; 
  FuncRParamsAST() = default; 

  std::string Dump(Context& context) const override;
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

