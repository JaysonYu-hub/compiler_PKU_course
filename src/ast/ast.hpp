#pragma once
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <variant>
#include <cassert>
#include "../IR/custom_ir.hpp"
#include "ast_visitor.hpp"



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
  std::vector<IR::IRType> param_types;
  FuncInformation() : ret_type("") {}
  FuncInformation(std::string ret_type, std::vector<IR::IRType> param_types = {})
    : ret_type(std::move(ret_type)), param_types(std::move(param_types)) {}
};

class ArrayInformation { 
  public:
  std::string address;
  std::vector<int> dims;
  std::vector<int> const_values;
  std::vector<float> const_float_values;
  bool is_param = false;
  IR::IRType element_type = IR::IRType::IR_INT32;
  ArrayInformation() = default;
  ArrayInformation(std::string address, std::vector<int> dims,
                   std::vector<int> const_values = {},
                   std::vector<float> const_float_values = {},
                   bool is_param = false,
                   IR::IRType element_type = IR::IRType::IR_INT32)
    : address(std::move(address)), dims(std::move(dims)),
      const_values(std::move(const_values)),
      const_float_values(std::move(const_float_values)), is_param(is_param),
      element_type(element_type) {}

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
  IR::IRType object_type = IR::IRType::IR_DEFAULT;
  std::variant<int, float, std::string, FuncInformation, ArrayInformation> value;
  Symbol(const std::string &name, SymbolKind kind, int const_value)
    : name(name), kind(kind), object_type(IR::IRType::IR_INT32), value(const_value) {}
  Symbol(const std::string &name, SymbolKind kind, float const_value)
    : name(name), kind(kind), object_type(IR::IRType::IR_FLOAT), value(const_value) {}
  Symbol(const std::string &name, SymbolKind kind, const std::string &address,
         IR::IRType object_type = IR::IRType::IR_INT32)
    : name(name), kind(kind), object_type(object_type), value(address) {}

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

class ConstantVal {
public:
  std::variant<int, float> val;
  ConstantVal() : val(0) {}
  ConstantVal(int val) : val(val) {}
  ConstantVal(float val) : val(val) {}

  bool isFloat() const {
    return std::holds_alternative<float>(val);
  }
  float getFloat() const {
    return isFloat() ? std::get<float>(val) : static_cast<float>(std::get<int>(val));
  }
  int32_t getInt() const {
    return isFloat() ? static_cast<int32_t>(std::get<float>(val)) : std::get<int>(val);
  }
};

class WhileLabelContext {
public:
  std::shared_ptr<IR::IRBasicBlock> while_label_begin;
  std::shared_ptr<IR::IRBasicBlock> while_label_continue;
  std::shared_ptr<IR::IRBasicBlock> while_label_end;

  WhileLabelContext(std::shared_ptr<IR::IRBasicBlock> while_label_begin,
    std::shared_ptr<IR::IRBasicBlock> while_label_continue,
    std::shared_ptr<IR::IRBasicBlock> while_label_end)
    : while_label_begin(while_label_begin), while_label_continue(while_label_continue), while_label_end(while_label_end){};
  WhileLabelContext() = default;
};

// 上下文：用于提供RegBuilder接口和作用域符号表
class Context {
public:
  Counter reg_builder;
  Counter scope_ident_builder;
  std::shared_ptr<Scope> scope;
  bool cur_block_has_terminator = false;
  // 用于continue和break语句的generate
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
    if (!name.empty() && name[0] != '%' && name[0] != '@') {
      if (name.find('.') != std::string::npos) {
        try {
          float val = std::stof(name);
          auto const_val = std::make_shared<IR::IRConstant>(val);
          ir_values[name] = const_val;
          return const_val;
        } catch (...) {
        }
      } else {
        try {
          int val = std::stoi(name);
          auto const_val = std::make_shared<IR::IRConstant>(val);
          ir_values[name] = const_val;
          return const_val;
        } catch (...) {
        }
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

  void enterNewWhileScope(std::shared_ptr<IR::IRBasicBlock> while_label_begin, 
    std::shared_ptr<IR::IRBasicBlock> while_label_continue, 
    std::shared_ptr<IR::IRBasicBlock> while_label_end) { 
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
  virtual std::string Accept(ASTVisitor& visitor) const = 0;
};

class ValueAST : public BaseAST {
public:
  virtual ~ValueAST() = default;
  virtual ConstantVal getValue(Context& context) const = 0;
  virtual bool isAggregateInitializer() const;
  virtual const std::vector<std::unique_ptr<ValueAST>>& getInitializerElements() const;
};

// Concrete nodes inherit this helper to implement the first dispatch step.
// The visitor supplies the second dispatch step through its Visit overloads.
template <typename Derived, typename Base = BaseAST>
class VisitableAST : public Base {
public:
  std::string Accept(ASTVisitor& visitor) const final {
    return visitor.Visit(static_cast<const Derived&>(*this));
  }
};

class ArrayDimensionsAST : public VisitableAST<ArrayDimensionsAST> {
public:
  std::vector<std::unique_ptr<ValueAST>> dims;
  ArrayDimensionsAST() = default;
};

// CompUnit ::= end_comp_unit
class CompUnitAST : public VisitableAST<CompUnitAST> {
public:
  std::unique_ptr<BaseAST> end_comp_unit;
  CompUnitAST() = default;
};

// end_comp_unit ::= comp_unit | end_comp_unit comp_unit
class CompUnitSequenceAST : public VisitableAST<CompUnitSequenceAST> {
public:
  std::unique_ptr<BaseAST> comp_unit;
  CompUnitSequenceAST() = default;
};

class CompUnitListAST : public VisitableAST<CompUnitListAST> {
public:
  std::unique_ptr<BaseAST> end_comp_unit;
  std::unique_ptr<BaseAST> comp_unit;
  CompUnitListAST() = default;
};

// comp_unit ::= Decl | FuncDef
class FunctionDefinitionUnitAST : public VisitableAST<FunctionDefinitionUnitAST> {
public:
  std::unique_ptr<BaseAST> func_def;
  FunctionDefinitionUnitAST() = default; 
};

class DeclarationUnitAST : public VisitableAST<DeclarationUnitAST> {
public:
    std::unique_ptr<BaseAST> decl;
    DeclarationUnitAST() = default;
};

class FuncFParamsAST;
// FuncDef ::= Type IDENT '(' [FuncFParams] ')' Block
class FuncDefAST : public VisitableAST<FuncDefAST> {
public:
  std::unique_ptr<std::string> type;
  std::unique_ptr<std::string> ident;
  std::unique_ptr<BaseAST> block;
  std::unique_ptr<FuncFParamsAST> func_f_params;
  FuncDefAST() = default; 
};

// FuncFParam ::= Type IDENT ';'
class FuncFParamAST : public VisitableAST<FuncFParamAST> {
public:
  std::unique_ptr<std::string> type;
  std::unique_ptr<std::string> ident;
  bool is_array = false;
  std::vector<std::unique_ptr<ValueAST>> array_dims;
  FuncFParamAST() = default; 
};

// FuncFParams ::= FuncFParam { ',' FuncFParam }
class FuncFParamsAST : public VisitableAST<FuncFParamsAST> {
public:
  std::vector<std::unique_ptr<FuncFParamAST>> func_f_params; 
  FuncFParamsAST() = default; 
};

// BlockItem ::= Decl | Stmt
class DeclBlockAST : public VisitableAST<DeclBlockAST> {
public:
  std::unique_ptr<BaseAST> decl;
  DeclBlockAST() = default; 
};

class StmtBlockAST : public VisitableAST<StmtBlockAST> {
public:
  std::unique_ptr<BaseAST> stmt;
  StmtBlockAST() = default; 
};

// Block ::= '{' {BlockItem} '}'
class BlockAST : public VisitableAST<BlockAST> {
public:
  std::vector<std::unique_ptr<BaseAST>> block_items; 
  BlockAST() = default; 
};

// Stmt ::= UnmatchedStmt | MatchedStmt
class UnmatchedStmtWrapperAST : public VisitableAST<UnmatchedStmtWrapperAST> {
public:
  std::unique_ptr<BaseAST> unmatched_stmt;
  UnmatchedStmtWrapperAST() = default; 
};

class MatchedStmtWrapperAST : public VisitableAST<MatchedStmtWrapperAST> {
public:
  std::unique_ptr<BaseAST> matched_stmt;
  MatchedStmtWrapperAST() = default; 
};

// UnmatchedStmt ::= IF '(' Exp ')' Stmt | IF '(' Exp ')' MatchedStmt ELSE UnMatchedStmt | WHILE '(' Exp ')' UnmatchedStmt
class IfUnmatchedStmtAST : public VisitableAST<IfUnmatchedStmtAST> {
public:
  std::unique_ptr<BaseAST> if_exp;
  std::unique_ptr<BaseAST> if_stmt;
  std::unique_ptr<BaseAST> else_stmt;
  IfUnmatchedStmtAST() = default; 
};

class IfElseUnmatchedStmtAST : public VisitableAST<IfElseUnmatchedStmtAST> {
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
class ReturnStmtAST : public VisitableAST<ReturnStmtAST> {
public:
  std::unique_ptr<BaseAST> exp;
  ReturnStmtAST() = default; 
};

class AssignStmtAST : public VisitableAST<AssignStmtAST> {
public:
  std::unique_ptr<LValAST> lval;
  std::unique_ptr<ValueAST> exp;
  AssignStmtAST() = default; 
};

class ExpStmtAST : public VisitableAST<ExpStmtAST> {
public:
  std::unique_ptr<ValueAST> exp;
  ExpStmtAST() = default; 
};

class EmptyStmtAST : public VisitableAST<EmptyStmtAST> {
public:
  EmptyStmtAST() = default; 
};

class BlockStmtAST : public VisitableAST<BlockStmtAST> {
public:
  std::unique_ptr<BaseAST> block;
  BlockStmtAST() = default; 
};

class IfMatchedStmtAST : public VisitableAST<IfMatchedStmtAST> {
public:
  std::unique_ptr<ValueAST> if_exp;
  std::unique_ptr<BaseAST> if_stmt;
  std::unique_ptr<BaseAST> else_stmt;
  IfMatchedStmtAST() = default; 
};

class WhileStmtAST : public VisitableAST<WhileStmtAST> {
public:
  std::unique_ptr<ValueAST> while_exp;
  std::unique_ptr<BaseAST> while_stmt;
  WhileStmtAST() = default; 
};

class ContinueStmtAST : public VisitableAST<ContinueStmtAST> {
public:
  ContinueStmtAST() = default;
};

class BreakStmtAST : public VisitableAST<BreakStmtAST> {
public:
  BreakStmtAST() = default;
};

// Exp ::= LOrExp
class ExpAST : public VisitableAST<ExpAST, ValueAST> {
public:
  std::unique_ptr<ValueAST> lor_exp;
  ExpAST() = default;
  ConstantVal getValue(Context& context) const override;
};

// UnaryExp ::= UnaryOP UnaryExp | PrimaryExp | IDENT '(' [FuncRParams] ')'
class UnaryOPExpAST : public VisitableAST<UnaryOPExpAST, ValueAST> {
public:
  std::unique_ptr<OpAST> op;
  std::unique_ptr<ValueAST> unary_exp;
  UnaryOPExpAST() = default; 
  ConstantVal getValue(Context& context) const override;
};

class PrimaryUnaryExpAST : public VisitableAST<PrimaryUnaryExpAST, ValueAST> {
public:
  std::unique_ptr<ValueAST> primary_exp;
  PrimaryUnaryExpAST() = default; 
  ConstantVal getValue(Context& context) const override;
};

class FuncCallExpAST : public VisitableAST<FuncCallExpAST, ValueAST> {
public:
  std::unique_ptr<std::string> ident;
  std::unique_ptr<BaseAST> func_r_params;
  int source_line = 0;
  FuncCallExpAST() = default; 
  ConstantVal getValue(Context& context) const override;
};

// FuncRParams ::= Exp { ',' Exp }
class FuncRParamsAST : public VisitableAST<FuncRParamsAST> {
public:
  std::vector<std::unique_ptr<ValueAST>> func_r_params; 
  FuncRParamsAST() = default; 
};

// PrimaryExp ::= '(' Exp ')' | LVal | Number
class ParenthesizedExprAST : public VisitableAST<ParenthesizedExprAST, ValueAST> {
public:
  std::unique_ptr<ValueAST> exp;
  ParenthesizedExprAST() = default; 
  ConstantVal getValue(Context& context) const override;
};

class LValuePrimaryExprAST : public VisitableAST<LValuePrimaryExprAST, ValueAST> {
public:
  std::unique_ptr<ValueAST> lval;
  LValuePrimaryExprAST() = default; 
  ConstantVal getValue(Context& context) const override;
};

class NumberPrimaryExprAST : public VisitableAST<NumberPrimaryExprAST, ValueAST> {
public:
  ConstantVal value;
  NumberPrimaryExprAST() = default; 
  ConstantVal getValue(Context& context) const override;
};

//'+' | '-' | '!' | '*' | '/' | '%'
class OpAST : public VisitableAST<OpAST> {
public:
  std::string op;
  OpAST() = default; 
};

// MulExp ::= UnaryExp | MulExp ('*' | '/' | '%') UnaryExp
class UnaryMulExpAST : public VisitableAST<UnaryMulExpAST, ValueAST> {
public:
  std::unique_ptr<ValueAST> unary_exp;
  UnaryMulExpAST() = default; 
  ConstantVal getValue(Context& context) const override;
};

class BinaryMulExprAST : public VisitableAST<BinaryMulExprAST, ValueAST> {
public:
  std::unique_ptr<ValueAST> mul_exp;
  std::unique_ptr<OpAST> op; // '*' | '/' | '%'
  std::unique_ptr<ValueAST> unary_exp;
  BinaryMulExprAST() = default; 
  ConstantVal getValue(Context& context) const override;
};

// AddExp ::= MulExp | AddExp ('+' | '-') MulExp
class MulAddExpAST : public VisitableAST<MulAddExpAST, ValueAST> {
public:
  std::unique_ptr<ValueAST> mul_exp;
  MulAddExpAST() = default; 
  ConstantVal getValue(Context& context) const override;
};

class BinaryAddExprAST : public VisitableAST<BinaryAddExprAST, ValueAST> {
public:
  std::unique_ptr<ValueAST> add_exp;
  std::unique_ptr<OpAST> op; // '+' or '-'
  std::unique_ptr<ValueAST> mul_exp;
  BinaryAddExprAST() = default; 
  ConstantVal getValue(Context& context) const override;
};

// RelExp ::= AddExp | RelExp ('<' | '>' | '<=' | '>=') AddExp
class AddRelExpAST : public VisitableAST<AddRelExpAST, ValueAST> {
public:
  std::unique_ptr<ValueAST> add_exp;
  AddRelExpAST() = default; 
  ConstantVal getValue(Context& context) const override;
};

class BinaryRelExprAST : public VisitableAST<BinaryRelExprAST, ValueAST> {
public: 
  std::unique_ptr<ValueAST> rel_exp;
  std::unique_ptr<OpAST> op; // '<' | '>' | '<=' | '>='
  std::unique_ptr<ValueAST> add_exp;
  BinaryRelExprAST() = default; 
  ConstantVal getValue(Context& context) const override;
};

// EqExp ::= RelExp | EqExp ('==' | '!=') RelExp
class RelEqExpAST : public VisitableAST<RelEqExpAST, ValueAST> {
public:
  std::unique_ptr<ValueAST> rel_exp;
  RelEqExpAST() = default; 
  ConstantVal getValue(Context& context) const override;
};

class BinaryEqExprAST : public VisitableAST<BinaryEqExprAST, ValueAST> {
public:
  std::unique_ptr<ValueAST> eq_exp;
  std::unique_ptr<OpAST> op; // '==' | '!='
  std::unique_ptr<ValueAST> rel_exp;
  BinaryEqExprAST() = default; 
  ConstantVal getValue(Context& context) const override;
};

// LAndExp ::= EqExp | LAndExp '&&' EqExp
class EqLAndExpAST : public VisitableAST<EqLAndExpAST, ValueAST> {
public:
  std::unique_ptr<ValueAST> eq_exp;
  EqLAndExpAST() = default; 
  ConstantVal getValue(Context& context) const override;
};

class BinaryLogicalAndExprAST : public VisitableAST<BinaryLogicalAndExprAST, ValueAST> {
public:
  std::unique_ptr<ValueAST> land_exp;
  std::unique_ptr<OpAST> op; // '&&'
  std::unique_ptr<ValueAST> eq_exp;
  BinaryLogicalAndExprAST() = default; 
  ConstantVal getValue(Context& context) const override;
};

// LOrExp ::= LAndExp | LOrExp '||' LAndExp
class LAndLOrExpAST : public VisitableAST<LAndLOrExpAST, ValueAST> {
public:
  std::unique_ptr<ValueAST> land_exp;
  LAndLOrExpAST() = default; 
  ConstantVal getValue(Context& context) const override;
};

class BinaryLogicalOrExprAST : public VisitableAST<BinaryLogicalOrExprAST, ValueAST> {
public:
  std::unique_ptr<ValueAST> lor_exp;
  std::unique_ptr<OpAST> op; // '||'
  std::unique_ptr<ValueAST> land_exp;
  BinaryLogicalOrExprAST() = default; 
  ConstantVal getValue(Context& context) const override;
};

// LVal = IDENT | IDENT '[' Exp ']'
class LValAST : public VisitableAST<LValAST, ValueAST> {
public:
  std::unique_ptr<std::string> ident;
  std::vector<std::unique_ptr<ValueAST>> index_exp; 
  LValAST() = default; 
  std::string DumpOnlyAddress(Context& context) const;
  ConstantVal getValue(Context& context) const override;
};

// ConstExp ::= Exp
class ConstExpAST : public VisitableAST<ConstExpAST, ValueAST> {
public:
  std::unique_ptr<ValueAST> exp;
  ConstExpAST() = default; 
  ConstantVal getValue(Context& context) const override;
};

// ConstDef ::= IDENT '=' ConstInitVal | IDENT '[' ConstExp ']' '=' ConstInitVal
class ConstDefAST : public VisitableAST<ConstDefAST> {
public:
  std::unique_ptr<std::string> ident;
  std::unique_ptr<ValueAST> const_init_val;
  ConstDefAST() = default;
};
class CompoundConstInitValueAST;
class ArrayConstDefAST : public VisitableAST<ArrayConstDefAST> {
  public:
  std::unique_ptr<std::string> ident;
  std::vector<std::unique_ptr<ValueAST>> const_exps;
  std::unique_ptr<CompoundConstInitValueAST> const_init_val;
  ArrayConstDefAST() = default;
};
// ConstInitVal ::= ConstExp | '{' ConstInitVal { ',' ConstInitVal } '}'
class ConstExpConstInitValAST : public VisitableAST<ConstExpConstInitValAST, ValueAST> {
public:
  std::unique_ptr<ValueAST> const_exp;
  ConstExpConstInitValAST() = default; 
  ConstantVal getValue(Context& context) const override;
};

class ConstInitValListAST : public VisitableAST<ConstInitValListAST> {
public:
  std::vector<std::unique_ptr<ValueAST>> const_init_vals;
  ConstInitValListAST() = default; 
};

class CompoundConstInitValueAST : public VisitableAST<CompoundConstInitValueAST, ValueAST> {
public:
  std::unique_ptr<ConstInitValListAST> const_init_val_list;
  CompoundConstInitValueAST() = default; 
  ConstantVal getValue(Context& context) const override;
  bool isAggregateInitializer() const override;
  const std::vector<std::unique_ptr<ValueAST>>& getInitializerElements() const override;
  std::vector<ConstantVal> getConstantVals(Context& context) const;
};

// Decl ::= ConstDecl | VarDecl
class ConstDeclWrapperAST : public VisitableAST<ConstDeclWrapperAST> {
public:
  std::unique_ptr<BaseAST> const_decl;
  ConstDeclWrapperAST() = default; 
};

class VarDeclWrapperAST : public VisitableAST<VarDeclWrapperAST> {
public:
  std::unique_ptr<BaseAST> var_decl;
  VarDeclWrapperAST() = default; 
};

// ConstDecl ::= 'const' Type ConstDef { ',' ConstDef } ';'
class ConstDeclAST : public VisitableAST<ConstDeclAST> {
public:
  std::unique_ptr<std::string> type;
  std::unique_ptr<BaseAST> const_def_list;
  ConstDeclAST() = default; 
};

class ConstDefListAST : public VisitableAST<ConstDefListAST> {
public:
  std::vector<std::unique_ptr<BaseAST>> const_defs;
  ConstDefListAST() = default; 
};

// VarDecl ::= Type VarDef { ',' VarDef } ';'
class VarDeclAST : public VisitableAST<VarDeclAST> {
public:
  std::unique_ptr<std::string> type;
  std::unique_ptr<BaseAST> var_def_list;
  VarDeclAST() = default; 
};

class VarDefListAST : public VisitableAST<VarDefListAST> {
public:
  std::vector<std::unique_ptr<BaseAST>> var_defs;
  VarDefListAST() = default; 
};

// VarDef ::= IDENT | IDENT '=' InitVal | IDENT '[' ConstExp ']' | IDENT '[' ConstExp ']' '=' InitVal
class SimpleVarDefAST : public VisitableAST<SimpleVarDefAST> {
public:
  std::unique_ptr<std::string> ident;
  SimpleVarDefAST() = default; 
};

class InitializedVarDefAST : public VisitableAST<InitializedVarDefAST> {
public:
  std::unique_ptr<std::string> ident;
  std::unique_ptr<ValueAST> init_val;
  InitializedVarDefAST() = default; 
};

class ArrayVarDefAST : public VisitableAST<ArrayVarDefAST> {
  public:
  std::unique_ptr<std::string> ident;
  std::vector<std::unique_ptr<ValueAST>> const_exps;
  ArrayVarDefAST() = default; 
};

class InitValListAST;
class ArrayInitializedVarDefAST : public VisitableAST<ArrayInitializedVarDefAST> {
  public:
  std::unique_ptr<std::string> ident;
  std::vector<std::unique_ptr<ValueAST>> const_exps;
  std::unique_ptr<InitValListAST> init_val_list;
  ArrayInitializedVarDefAST() = default; 
};

// InitVal ::= Exp | '{' Exp { ',' Exp } '}'
class InitValAST : public VisitableAST<InitValAST, ValueAST> {
public:
  std::unique_ptr<ValueAST> exp;
  InitValAST() = default; 
  ConstantVal getValue(Context& context) const override;
};

class InitValListAST : public VisitableAST<InitValListAST, ValueAST> {
public:
  std::vector<std::unique_ptr<ValueAST>> init_vals;
  InitValListAST() = default; 
  ConstantVal getValue(Context& context) const override { return ConstantVal(0); }
  bool isAggregateInitializer() const override;
  const std::vector<std::unique_ptr<ValueAST>>& getInitializerElements() const override;
  std::vector<ConstantVal> getConstantVals(Context& context) const;
};

std::string GenerateIR(const BaseAST& node, Context& context);
