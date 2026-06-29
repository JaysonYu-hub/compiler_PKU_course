#pragma once

#include <string>

class Context;
class BaseAST;
class CompUnitAST;
class CompUnitSequenceAST;
class CompUnitListAST;
class FunctionDefinitionUnitAST;
class DeclarationUnitAST;
class FuncDefAST;
class FuncFParamsAST;
class FuncFParamAST;
class BlockAST;
class DeclBlockAST;
class StmtBlockAST;
class UnmatchedStmtWrapperAST;
class MatchedStmtWrapperAST;
class IfUnmatchedStmtAST;
class IfElseUnmatchedStmtAST;
class ReturnStmtAST;
class AssignStmtAST;
class ExpStmtAST;
class EmptyStmtAST;
class BlockStmtAST;
class IfMatchedStmtAST;
class WhileStmtAST;
class ContinueStmtAST;
class BreakStmtAST;

class CodeGenerator {
public:
  explicit CodeGenerator(Context& context);

  std::string Generate(const BaseAST& node);
  std::string Generate(const CompUnitAST& node);
  std::string Generate(const CompUnitSequenceAST& node);
  std::string Generate(const CompUnitListAST& node);
  std::string Generate(const FunctionDefinitionUnitAST& node);
  std::string Generate(const DeclarationUnitAST& node);
  std::string Generate(const FuncDefAST& node);
  std::string Generate(const FuncFParamsAST& node);
  std::string Generate(const FuncFParamAST& node);
  std::string Generate(const BlockAST& node);
  std::string Generate(const DeclBlockAST& node);
  std::string Generate(const StmtBlockAST& node);
  std::string Generate(const UnmatchedStmtWrapperAST& node);
  std::string Generate(const MatchedStmtWrapperAST& node);
  std::string Generate(const IfUnmatchedStmtAST& node);
  std::string Generate(const IfElseUnmatchedStmtAST& node);
  std::string Generate(const ReturnStmtAST& node);
  std::string Generate(const AssignStmtAST& node);
  std::string Generate(const ExpStmtAST& node);
  std::string Generate(const EmptyStmtAST& node);
  std::string Generate(const BlockStmtAST& node);
  std::string Generate(const IfMatchedStmtAST& node);
  std::string Generate(const WhileStmtAST& node);
  std::string Generate(const ContinueStmtAST& node);
  std::string Generate(const BreakStmtAST& node);

private:
  Context& context;
};
