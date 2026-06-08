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
    func_def->Dump();
  }
};

class FuncDefAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> func_type;
  std::string ident;
  std::unique_ptr<BaseAST> block;
  
  FuncDefAST() = default;
  void Dump() const override {
    std::cout << "fun @";
    std::cout << ident << "(): ";
    func_type->Dump();
    std::cout << "{ ";
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
    std::cout << type_name;
  }
};

// Block ::= '{' Stmt '}'
class BlockAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> stmt;

  BlockAST() = default;
  void Dump() const override {
    std::cout << "\%entry: ";
    if (stmt) stmt->Dump();
  }
};

class StmtAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> number;

  StmtAST() = default;
  void Dump() const override {
    std::cout << "ret ";
    if (number) number->Dump();
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
