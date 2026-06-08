#pragma once
#include <memory>
#include <string>
#include <iostream>

class BaseAST {
 public:
  virtual ~BaseAST() = default;

  virtual void Dump() const = 0;
};

class CompUnitAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> func_def;

  void Dump() const override {
    std::cout << "CompUnitAST { ";
    func_def->Dump();
    std::cout << " }";
  }
};

class FuncDefAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> func_type;
  std::string ident;
  std::unique_ptr<BaseAST> block;
  
  FuncDefAST() = default;
  void Dump() const override {
    std::cout << "FuncDefAST { ";
    func_type->Dump();
    std::cout << ", " << ident << ", ";
    block->Dump();
    std::cout << " }";
  }
};

// ...
// FuncType ::= 'int'
class FuncTypeAST : public BaseAST {
 public:
  std::string type_name; // 存 "int"
  
  FuncTypeAST(const std::string &type) : type_name(type) {}

  void Dump() const override {
    std::cout << "FuncTypeAST { " << type_name << " }";
  }
};

// Block ::= '{' Stmt '}'
class BlockAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> stmt;

  BlockAST() = default;
  void Dump() const override {
    std::cout << "BlockAST { ";
    if (stmt) stmt->Dump();
    std::cout << " }";
  }
};

class StmtAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> number;

  StmtAST() = default;
  void Dump() const override {
    std::cout << "StmtAST { ";
    if (number) number->Dump();
    std::cout << " }";
  }
};

class NumberAST : public BaseAST {
 public:
  int value;

  NumberAST(int val) : value(val) {}

  void Dump() const override {
    std::cout << value;
  }
};
