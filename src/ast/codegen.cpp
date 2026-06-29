#include "ast/codegen.hpp"
#include "ast/ast.hpp"
#include <iostream>
#include <memory>

CodeGenerator::CodeGenerator(Context& context) : context(context) {}

std::string CodeGenerator::Generate(const BaseAST& node) {
  if (const auto* p = dynamic_cast<const CompUnitAST*>(&node)) {
    return Generate(*p);
  }
  if (const auto* p = dynamic_cast<const CompUnitSequenceAST*>(&node)) {
    return Generate(*p);
  }
  if (const auto* p = dynamic_cast<const CompUnitListAST*>(&node)) {
    return Generate(*p);
  }
  if (const auto* p = dynamic_cast<const FunctionDefinitionUnitAST*>(&node)) {
    return Generate(*p);
  }
  if (const auto* p = dynamic_cast<const DeclarationUnitAST*>(&node)) {
    return Generate(*p);
  }
  if (const auto* p = dynamic_cast<const FuncDefAST*>(&node)) {
    return Generate(*p);
  }
  if (const auto* p = dynamic_cast<const FuncFParamsAST*>(&node)) {
    return Generate(*p);
  }
  if (const auto* p = dynamic_cast<const FuncFParamAST*>(&node)) {
    return Generate(*p);
  }
  if (const auto* p = dynamic_cast<const BlockAST*>(&node)) {
    return Generate(*p);
  }
  if (const auto* p = dynamic_cast<const DeclBlockAST*>(&node)) {
    return Generate(*p);
  }
  if (const auto* p = dynamic_cast<const StmtBlockAST*>(&node)) {
    return Generate(*p);
  }
  if (const auto* p = dynamic_cast<const UnmatchedStmtWrapperAST*>(&node)) {
    return Generate(*p);
  }
  if (const auto* p = dynamic_cast<const MatchedStmtWrapperAST*>(&node)) {
    return Generate(*p);
  }
  if (const auto* p = dynamic_cast<const IfUnmatchedStmtAST*>(&node)) {
    return Generate(*p);
  }
  if (const auto* p = dynamic_cast<const IfElseUnmatchedStmtAST*>(&node)) {
    return Generate(*p);
  }
  if (const auto* p = dynamic_cast<const ReturnStmtAST*>(&node)) {
    return Generate(*p);
  }
  if (const auto* p = dynamic_cast<const AssignStmtAST*>(&node)) {
    return Generate(*p);
  }
  if (const auto* p = dynamic_cast<const ExpStmtAST*>(&node)) {
    return Generate(*p);
  }
  if (const auto* p = dynamic_cast<const EmptyStmtAST*>(&node)) {
    return Generate(*p);
  }
  if (const auto* p = dynamic_cast<const BlockStmtAST*>(&node)) {
    return Generate(*p);
  }
  if (const auto* p = dynamic_cast<const IfMatchedStmtAST*>(&node)) {
    return Generate(*p);
  }
  if (const auto* p = dynamic_cast<const WhileStmtAST*>(&node)) {
    return Generate(*p);
  }
  if (const auto* p = dynamic_cast<const ContinueStmtAST*>(&node)) {
    return Generate(*p);
  }
  if (const auto* p = dynamic_cast<const BreakStmtAST*>(&node)) {
    return Generate(*p);
  }
  return node.Dump(context);
}

std::string CodeGenerator::Generate(const CompUnitAST& node) {
  if (node.end_comp_unit) {
    auto library_decl = R"(decl @getint(): i32
decl @getch(): i32
decl @getarray(*i32): i32
decl @putint(i32)
decl @putch(i32)
decl @putarray(i32, *i32)
decl @starttime()
decl @stoptime()

)";
    std::cout << library_decl;
    auto symbol = std::make_shared<Symbol>("getint", SymbolKind::FUNCTION, FuncInformation("i32"));
    context.scope->define(symbol);
    symbol = std::make_shared<Symbol>("getch", SymbolKind::FUNCTION, FuncInformation("i32"));
    context.scope->define(symbol);
    symbol = std::make_shared<Symbol>("getarray", SymbolKind::FUNCTION, FuncInformation("i32"));
    context.scope->define(symbol);
    symbol = std::make_shared<Symbol>("putint", SymbolKind::FUNCTION, FuncInformation("void"));
    context.scope->define(symbol);
    symbol = std::make_shared<Symbol>("putch", SymbolKind::FUNCTION, FuncInformation("void"));
    context.scope->define(symbol);
    symbol = std::make_shared<Symbol>("putarray", SymbolKind::FUNCTION, FuncInformation("void"));
    context.scope->define(symbol);
    symbol = std::make_shared<Symbol>("starttime", SymbolKind::FUNCTION, FuncInformation("void"));
    context.scope->define(symbol);
    symbol = std::make_shared<Symbol>("stoptime", SymbolKind::FUNCTION, FuncInformation("void"));
    context.scope->define(symbol);
    node.end_comp_unit->Dump(context);
  }
  return "";
}

std::string CodeGenerator::Generate(const CompUnitSequenceAST& node) {
  if (node.comp_unit) {
    node.comp_unit->Dump(context);
  }
  return "";
}

std::string CodeGenerator::Generate(const CompUnitListAST& node) {
  if (node.end_comp_unit) {
    node.end_comp_unit->Dump(context);
  }
  if (node.comp_unit) {
    node.comp_unit->Dump(context);
  }
  return "";
}

std::string CodeGenerator::Generate(const FunctionDefinitionUnitAST& node) {
  if (node.func_def) {
    node.func_def->Dump(context);
  }
  return "";
}

std::string CodeGenerator::Generate(const DeclarationUnitAST& node) {
  if (node.decl) {
    node.decl->Dump(context);
  }
  return "";
}

std::string CodeGenerator::Generate(const FuncDefAST& node) {
  return node.Dump(context);
}

std::string CodeGenerator::Generate(const FuncFParamsAST& node) {
  if (node.func_f_params.size() > 0) {
    std::cout << node.func_f_params[0]->Dump(context);
    for (int i = 1; i < node.func_f_params.size(); i++) {
      std::cout << ", " << node.func_f_params[i]->Dump(context);
    }
  }
  return "";
}

std::string CodeGenerator::Generate(const FuncFParamAST& node) {
  return node.Dump(context);
}

std::string CodeGenerator::Generate(const BlockAST& node) {
  return node.Dump(context);
}

std::string CodeGenerator::Generate(const DeclBlockAST& node) {
  if (node.decl) {
    node.decl->Dump(context);
  }
  return "";
}

std::string CodeGenerator::Generate(const StmtBlockAST& node) {
  return node.stmt ? node.stmt->Dump(context) : "";
}

std::string CodeGenerator::Generate(const UnmatchedStmtWrapperAST& node) {
  if (node.unmatched_stmt) {
    node.unmatched_stmt->Dump(context);
  }
  return "";
}

std::string CodeGenerator::Generate(const MatchedStmtWrapperAST& node) {
  return node.matched_stmt ? node.matched_stmt->Dump(context) : "";
}

std::string CodeGenerator::Generate(const IfUnmatchedStmtAST& node) {
  return node.Dump(context);
}

std::string CodeGenerator::Generate(const IfElseUnmatchedStmtAST& node) {
  return node.Dump(context);
}

std::string CodeGenerator::Generate(const ReturnStmtAST& node) {
  return node.Dump(context);
}

std::string CodeGenerator::Generate(const AssignStmtAST& node) {
  return node.Dump(context);
}

std::string CodeGenerator::Generate(const ExpStmtAST& node) {
  return node.Dump(context);
}

std::string CodeGenerator::Generate(const EmptyStmtAST& node) {
  return node.Dump(context);
}

std::string CodeGenerator::Generate(const BlockStmtAST& node) {
  return node.Dump(context);
}

std::string CodeGenerator::Generate(const IfMatchedStmtAST& node) {
  return node.Dump(context);
}

std::string CodeGenerator::Generate(const WhileStmtAST& node) {
  return node.Dump(context);
}

std::string CodeGenerator::Generate(const ContinueStmtAST& node) {
  return node.Dump(context);
}

std::string CodeGenerator::Generate(const BreakStmtAST& node) {
  return node.Dump(context);
}
