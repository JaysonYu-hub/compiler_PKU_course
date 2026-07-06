#pragma once
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <variant>
#include <cassert>
#include "IR/custom_ir.hpp"

class CodeGenerator;

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

class ArrayInformation { 
  public:
  std::string address;
  std::vector<int> dims;
  std::vector<int> const_values;
  bool is_param = false;
  ArrayInformation() = default;
  ArrayInformation(std::string address, std::vector<int> dims, std::vector<int> const_values = {}, bool is_param = false) 
    : address(std::move(address)), dims(std::move(dims)), const_values(std::move(const_values)), is_param(is_param) {}

  int GetTotalSize() const {
    int total = 1;
    for (int d : dims) {
      total *= d;
    }
    return total;
  }
};

enum class SymbolKind { CONSTANT, VARIABLE, FUNCTION, ARRAY, CONST_ARRAY};

/**
 * 符号表
 * 符号表是一个字典，键为变量名，值为Symbol
 * Symbol为符号表项，包含变量名、变量类型、变量值
 * 对于变量，变量值为address
 * 对于常量，变量值为const_value
 * 对于函数，变量值为FuncInformation
 * 对于数组，变量值为ArrayInformation
*/
struct Symbol {
public:
  std::string name;
  SymbolKind kind;
  std::variant<int, std::string, FuncInformation, ArrayInformation> value;
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

  bool isGlobal() const { return parentScope == nullptr; }
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
  std::shared_ptr<std::string> decl_type_context;
  // 用于内存IR
  std::shared_ptr<IR::IRProgram> ir_program;
  IR::IRFunction* current_func = nullptr;
  IR::IRBasicBlock* current_block = nullptr;
  std::unordered_map<std::string, std::shared_ptr<IR::IRValue>> ir_values;
  std::vector<std::vector<std::shared_ptr<IR::IRValue>>> arg_stack;

  void SetIRValue(const std::string& name, std::shared_ptr<IR::IRValue> val) {
    ir_values[name] = val;
  }

  std::shared_ptr<IR::IRBasicBlock> AddNewBlock(const std::string& name) {
    if (current_func) {
      auto block = std::make_shared<IR::IRBasicBlock>(name);
      current_func->blocks.push_back(block);
      current_block = block.get();
      SetIRValue(name, block);
      return block;
    }
    return nullptr;
  }

  void AddExistingBlock(std::shared_ptr<IR::IRBasicBlock> block) {
    if (current_func && block) {
      current_func->blocks.push_back(block);
      current_block = block.get();
    }
  }

  void AddGlobal(std::shared_ptr<IR::IRGlobalAlloc> global) {
    if (global && ir_program) {
      ir_program->globals.push_back(global);
      SetIRValue(global->GetOperandName(), global);
    }
  }

  void AddDecl(std::shared_ptr<IR::IRFuncDecl> decl) {
    if (decl && ir_program) {
      ir_program->decls.push_back(decl);
      SetIRValue(decl->GetOperandName(), decl);
    }
  }

  std::shared_ptr<IR::IRValue> GetIRValue(const std::string& name) {
    if (ir_values.find(name) != ir_values.end()) {
      return ir_values[name];
    }
    // 不再利用前缀推导变量，仅自动解析纯整数字符串常量
    if (!name.empty() && name[0] != '%' && name[0] != '@') {
      try {
        int val = std::stoi(name);
        auto const_val = std::make_shared<IR::IRConstant>(val);
        ir_values[name] = const_val;
        return const_val;
      } catch (...) {
      }
    }
    return nullptr;
  }

  Context() : reg_builder(), scope(std::make_shared<Scope>(scope_ident_builder)), ir_program(std::make_shared<IR::IRProgram>()) {}
  void enterNewScope() {
    std::string new_ident = scope_ident_builder.NewNum();
    scope = std::make_shared<Scope>(scope, new_ident);
  }
  void exitScope() {
    if (scope->getParentScope() != nullptr) {
      scope = scope->getParentScope();
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
};

class ValueAST : public BaseAST {
public:
  virtual ~ValueAST() = default;
  virtual int getValue(Context& context) const = 0;
};

class ArrayDimensionsAST : public BaseAST {
public:
  std::vector<std::unique_ptr<ValueAST>> dims;
  ArrayDimensionsAST() = default;
};

// CompUnit ::= end_comp_unit
class CompUnitAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> end_comp_unit;
  CompUnitAST() = default;
};

// end_comp_unit ::= comp_unit | end_comp_unit comp_unit
class CompUnitSequenceAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> comp_unit;
  CompUnitSequenceAST() = default;
};
using CompEndCompUnitAST = CompUnitSequenceAST;

class CompUnitListAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> end_comp_unit;
  std::unique_ptr<BaseAST> comp_unit;
  CompUnitListAST() = default;
};
using TwoEndCompUnitAST = CompUnitListAST;

// comp_unit ::= Decl | FuncDef
class FunctionDefinitionUnitAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> func_def;
  FunctionDefinitionUnitAST() = default; 
};
using FuncDefCompUnitAST = FunctionDefinitionUnitAST;

class DeclarationUnitAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> decl;
    DeclarationUnitAST() = default;
};
using DeclCompUnitAST = DeclarationUnitAST;

class FuncFParamsAST;
// FuncDef ::= Type IDENT '(' [FuncFParams] ')' Block
class FuncDefAST : public BaseAST {
public:
  std::unique_ptr<std::string> type;
  std::unique_ptr<std::string> ident;
  std::unique_ptr<BaseAST> block;
  std::unique_ptr<FuncFParamsAST> func_f_params;
  FuncDefAST() = default; 
};

// FuncFParam ::= Type IDENT ';'
class FuncFParamAST : public BaseAST {
public:
  std::unique_ptr<std::string> type;
  std::unique_ptr<std::string> ident;
  bool is_array = false;
  std::vector<std::unique_ptr<ValueAST>> array_dims;
  FuncFParamAST() = default; 
};

// FuncFParams ::= FuncFParam { ',' FuncFParam }
class FuncFParamsAST : public BaseAST {
public:
  std::vector<std::unique_ptr<FuncFParamAST>> func_f_params; 
  FuncFParamsAST() = default; 
};

// BlockItem ::= Decl | Stmt
class DeclBlockAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> decl;
  DeclBlockAST() = default; 
};
using DeclBloAST = DeclBlockAST;

class StmtBlockAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> stmt;
  StmtBlockAST() = default; 
};
using StmtBloAST = StmtBlockAST;

// Block ::= '{' {BlockItem} '}'
class BlockAST : public BaseAST {
public:
  std::vector<std::unique_ptr<BaseAST>> block_items; 
  BlockAST() = default; 
};

// Stmt ::= UnmatchedStmt | MatchedStmt
class UnmatchedStmtWrapperAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> unmatched_stmt;
  UnmatchedStmtWrapperAST() = default; 
};
using UnmatchedStmtStmtAST = UnmatchedStmtWrapperAST;

class MatchedStmtWrapperAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> matched_stmt;
  MatchedStmtWrapperAST() = default; 
};
using MatchedStmtStmtAST = MatchedStmtWrapperAST;

// UnmatchedStmt ::= IF '(' Exp ')' Stmt | IF '(' Exp ')' MatchedStmt ELSE UnMatchedStmt | WHILE '(' Exp ')' UnmatchedStmt
class IfUnmatchedStmtAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> if_exp;
  std::unique_ptr<BaseAST> if_stmt;
  std::unique_ptr<BaseAST> else_stmt;
  IfUnmatchedStmtAST() = default; 
};

class IfElseUnmatchedStmtAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> if_exp;
  std::unique_ptr<BaseAST> if_stmt;
  std::unique_ptr<BaseAST> else_stmt;
  IfElseUnmatchedStmtAST() = default; 
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
};

class AssignStmtAST : public BaseAST {
public:
  std::unique_ptr<LValAST> lval;
  std::unique_ptr<ValueAST> exp;
  AssignStmtAST() = default; 
};

class ExpStmtAST : public BaseAST {
public:
  std::unique_ptr<ValueAST> exp;
  ExpStmtAST() = default; 
};

class EmptyStmtAST : public BaseAST {
public:
  EmptyStmtAST() = default; 
};

class BlockStmtAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> block;
  BlockStmtAST() = default; 
};

class IfMatchedStmtAST : public BaseAST {
public:
  std::unique_ptr<ValueAST> if_exp;
  std::unique_ptr<BaseAST> if_stmt;
  std::unique_ptr<BaseAST> else_stmt;
  IfMatchedStmtAST() = default; 
};

class WhileStmtAST : public BaseAST {
public:
  std::unique_ptr<ValueAST> while_exp;
  std::unique_ptr<BaseAST> while_stmt;
  WhileStmtAST() = default; 
};

class ContinueStmtAST : public BaseAST {
public:
  ContinueStmtAST() = default;
};

class BreakStmtAST : public BaseAST {
public:
  BreakStmtAST() = default;
};

// Exp ::= LOrExp
class ExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> lor_exp;
  ExpAST() = default;
  int getValue(Context& context) const override;
};

// UnaryExp ::= UnaryOP UnaryExp | PrimaryExp | IDENT '(' [FuncRParams] ')'
class UnaryOPExpAST : public ValueAST {
public:
  std::unique_ptr<OpAST> op;
  std::unique_ptr<ValueAST> unary_exp;
  UnaryOPExpAST() = default; 
  int getValue(Context& context) const override;
};

class PrimaryUnaryExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> primary_exp;
  PrimaryUnaryExpAST() = default; 
  int getValue(Context& context) const override;
};

class FuncCallExpAST : public ValueAST {
public:
  std::unique_ptr<std::string> ident;
  std::unique_ptr<BaseAST> func_r_params;
  FuncCallExpAST() = default; 
  int getValue(Context& context) const override;
};

// FuncRParams ::= Exp { ',' Exp }
class FuncRParamsAST : public BaseAST {
public:
  std::vector<std::unique_ptr<ValueAST>> func_r_params; 
  FuncRParamsAST() = default; 
};

// PrimaryExp ::= '(' Exp ')' | LVal | Number
class ParenthesizedExprAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> exp;
  ParenthesizedExprAST() = default; 
  int getValue(Context& context) const override;
};
using ThreePriAST = ParenthesizedExprAST;

class LValuePrimaryExprAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> lval;
  LValuePrimaryExprAST() = default; 
  int getValue(Context& context) const override;
};
using LValPriAST = LValuePrimaryExprAST;

class NumberPrimaryExprAST : public ValueAST {
public:
  int value;
  NumberPrimaryExprAST() = default; 
  int getValue(Context& context) const override;
};
using NumberPriAST = NumberPrimaryExprAST;

//'+' | '-' | '!' | '*' | '/' | '%'
class OpAST : public BaseAST {
public:
  std::string op;
  OpAST() = default; 
};

// MulExp ::= UnaryExp | MulExp ('*' | '/' | '%') UnaryExp
class UnaryMulExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> unary_exp;
  UnaryMulExpAST() = default; 
  int getValue(Context& context) const override;
};

class BinaryMulExprAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> mul_exp;
  std::unique_ptr<OpAST> op; // '*' | '/' | '%'
  std::unique_ptr<ValueAST> unary_exp;
  BinaryMulExprAST() = default; 
  int getValue(Context& context) const override;
};
using ThreeMulExpAST = BinaryMulExprAST;

// AddExp ::= MulExp | AddExp ('+' | '-') MulExp
class MulAddExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> mul_exp;
  MulAddExpAST() = default; 
  int getValue(Context& context) const override;
};

class BinaryAddExprAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> add_exp;
  std::unique_ptr<OpAST> op; // '+' or '-'
  std::unique_ptr<ValueAST> mul_exp;
  BinaryAddExprAST() = default; 
  int getValue(Context& context) const override;
};
using ThreeAddExpAST = BinaryAddExprAST;

// RelExp ::= AddExp | RelExp ('<' | '>' | '<=' | '>=') AddExp
class AddRelExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> add_exp;
  AddRelExpAST() = default; 
  int getValue(Context& context) const override;
};

class BinaryRelExprAST : public ValueAST {
public: 
  std::unique_ptr<ValueAST> rel_exp;
  std::unique_ptr<OpAST> op; // '<' | '>' | '<=' | '>='
  std::unique_ptr<ValueAST> add_exp;
  BinaryRelExprAST() = default; 
  int getValue(Context& context) const override;
};
using ThreeRelExpAST = BinaryRelExprAST;

// EqExp ::= RelExp | EqExp ('==' | '!=') RelExp
class RelEqExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> rel_exp;
  RelEqExpAST() = default; 
  int getValue(Context& context) const override;
};

class BinaryEqExprAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> eq_exp;
  std::unique_ptr<OpAST> op; // '==' | '!='
  std::unique_ptr<ValueAST> rel_exp;
  BinaryEqExprAST() = default; 
  int getValue(Context& context) const override;
};
using ThreeEqExpAST = BinaryEqExprAST;

// LAndExp ::= EqExp | LAndExp '&&' EqExp
class EqLAndExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> eq_exp;
  EqLAndExpAST() = default; 
  int getValue(Context& context) const override;
};

class BinaryLogicalAndExprAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> land_exp;
  std::unique_ptr<OpAST> op; // '&&'
  std::unique_ptr<ValueAST> eq_exp;
  BinaryLogicalAndExprAST() = default; 
  int getValue(Context& context) const override;
};
using ThreeLAndExpAST = BinaryLogicalAndExprAST;

// LOrExp ::= LAndExp | LOrExp '||' LAndExp
class LAndLOrExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> land_exp;
  LAndLOrExpAST() = default; 
  int getValue(Context& context) const override;
};

class BinaryLogicalOrExprAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> lor_exp;
  std::unique_ptr<OpAST> op; // '||'
  std::unique_ptr<ValueAST> land_exp;
  BinaryLogicalOrExprAST() = default; 
  int getValue(Context& context) const override;
};
using ThreeLOrExpAST = BinaryLogicalOrExprAST;

// LVal = IDENT | IDENT '[' Exp ']'
class LValAST : public ValueAST {
public:
  std::unique_ptr<std::string> ident;
  std::vector<std::unique_ptr<ValueAST>> index_exp; 
  LValAST() = default; 
  std::string DumpOnlyAddress(Context& context) const;
  int getValue(Context& context) const override;
};

// ConstExp ::= Exp
class ConstExpAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> exp;
  ConstExpAST() = default; 
  int getValue(Context& context) const override;
};

// ConstDef ::= IDENT '=' ConstInitVal | IDENT '[' ConstExp ']' '=' ConstInitVal
class ConstDefAST : public BaseAST {
public:
  std::unique_ptr<std::string> ident;
  std::unique_ptr<ValueAST> const_init_val;
  ConstDefAST() = default;
};
class CompoundConstInitValueAST;
class ArrayConstDefAST : public BaseAST { 
  public:
  std::unique_ptr<std::string> ident;
  std::vector<std::unique_ptr<ValueAST>> const_exps;
  std::unique_ptr<CompoundConstInitValueAST> const_init_val;
  ArrayConstDefAST() = default;
};
// ConstInitVal ::= ConstExp | '{' ConstInitVal { ',' ConstInitVal } '}'
class ConstExpConstInitValAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> const_exp;
  ConstExpConstInitValAST() = default; 
  int getValue(Context& context) const override;
};

class ConstInitValListAST : public BaseAST {
public:
  std::vector<std::unique_ptr<ValueAST>> const_init_vals;
  ConstInitValListAST() = default; 
};

class CompoundConstInitValueAST : public ValueAST {
public:
  std::unique_ptr<ConstInitValListAST> const_init_val_list;
  CompoundConstInitValueAST() = default; 
  int getValue(Context& context) const override;
  std::vector<int> getValues(Context& context) const;
};
using ThreeConstInitValAST = CompoundConstInitValueAST;

// Decl ::= ConstDecl | VarDecl
class ConstDeclWrapperAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> const_decl;
  ConstDeclWrapperAST() = default; 
};
using ConstDeclDecAST = ConstDeclWrapperAST;

class VarDeclWrapperAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> var_decl;
  VarDeclWrapperAST() = default; 
};
using VarDeclDecAST = VarDeclWrapperAST;

// ConstDecl ::= 'const' Type ConstDef { ',' ConstDef } ';'
class ConstDeclAST : public BaseAST {
public:
  std::unique_ptr<std::string> type;
  std::unique_ptr<BaseAST> const_def_list;
  ConstDeclAST() = default; 
};

class ConstDefListAST : public BaseAST {
public:
  std::vector<std::unique_ptr<BaseAST>> const_defs;
  ConstDefListAST() = default; 
};

// VarDecl ::= Type VarDef { ',' VarDef } ';'
class VarDeclAST : public BaseAST {
public:
  std::unique_ptr<std::string> type;
  std::unique_ptr<BaseAST> var_def_list;
  VarDeclAST() = default; 
};

class VarDefListAST : public BaseAST {
public:
  std::vector<std::unique_ptr<BaseAST>> var_defs;
  VarDefListAST() = default; 
};

// VarDef ::= IDENT | IDENT '=' InitVal | IDENT '[' ConstExp ']' | IDENT '[' ConstExp ']' '=' InitVal
class SimpleVarDefAST : public BaseAST {
public:
  std::unique_ptr<std::string> ident;
  SimpleVarDefAST() = default; 
};
using IDENTVarAST = SimpleVarDefAST;

class InitializedVarDefAST : public BaseAST {
public:
  std::unique_ptr<std::string> ident;
  std::unique_ptr<ValueAST> init_val;
  InitializedVarDefAST() = default; 
};
using ThreeVarAST = InitializedVarDefAST;

class ArrayVarDefAST : public BaseAST { 
  public:
  std::unique_ptr<std::string> ident;
  std::vector<std::unique_ptr<ValueAST>> const_exps;
  ArrayVarDefAST() = default; 
};
using FourVarDefAST = ArrayVarDefAST;

class InitValListAST;
class ArrayInitializedVarDefAST : public BaseAST { 
  public:
  std::unique_ptr<std::string> ident;
  std::vector<std::unique_ptr<ValueAST>> const_exps;
  std::unique_ptr<InitValListAST> init_val_list;
  ArrayInitializedVarDefAST() = default; 
};
using SixVarDefAST = ArrayInitializedVarDefAST;

// InitVal ::= Exp | '{' Exp { ',' Exp } '}'
class InitValAST : public ValueAST {
public:
  std::unique_ptr<ValueAST> exp;
  InitValAST() = default; 
  int getValue(Context& context) const override;
};

class InitValListAST : public ValueAST {
public:
  std::vector<std::unique_ptr<ValueAST>> init_vals;
  InitValListAST() = default; 
  int getValue(Context& context) const override { return 0; }
  std::vector<int> getValues(Context& context) const;
};

