#include "ast/codegen.hpp"
#include "ast/ast.hpp"
#include <iostream>
#include <cassert>
#include <memory>

CodeGenerator::CodeGenerator(Context& context) : context(context) {}

std::string CodeGenerator::Generate(const BaseAST& node) {
  if (const auto* p = dynamic_cast<const CompUnitAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const CompUnitSequenceAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const CompUnitListAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const FunctionDefinitionUnitAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const DeclarationUnitAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const FuncDefAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const FuncFParamsAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const FuncFParamAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const BlockAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const DeclBlockAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const StmtBlockAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const UnmatchedStmtWrapperAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const MatchedStmtWrapperAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const IfUnmatchedStmtAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const IfElseUnmatchedStmtAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const ReturnStmtAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const AssignStmtAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const ExpStmtAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const EmptyStmtAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const BlockStmtAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const IfMatchedStmtAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const WhileStmtAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const ContinueStmtAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const BreakStmtAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const ExpAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const UnaryOPExpAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const PrimaryUnaryExpAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const FuncCallExpAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const FuncRParamsAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const ParenthesizedExprAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const LValuePrimaryExprAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const NumberPrimaryExprAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const OpAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const UnaryMulExpAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const BinaryMulExprAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const MulAddExpAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const BinaryAddExprAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const AddRelExpAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const BinaryRelExprAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const RelEqExpAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const BinaryEqExprAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const EqLAndExpAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const BinaryLogicalAndExprAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const LAndLOrExpAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const BinaryLogicalOrExprAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const LValAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const ConstExpAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const ConstDefAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const ArrayConstDefAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const ConstExpConstInitValAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const ConstInitValListAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const CompoundConstInitValueAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const ConstDeclWrapperAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const VarDeclWrapperAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const ConstDeclAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const ConstDefListAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const VarDeclAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const VarDefListAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const SimpleVarDefAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const InitializedVarDefAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const ArrayVarDefAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const ArrayInitializedVarDefAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const InitValAST*>(&node)) return Generate(*p);
  if (const auto* p = dynamic_cast<const InitValListAST*>(&node)) return Generate(*p);

  return "";
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
    Generate(*node.end_comp_unit);
  }
  return "";
}

std::string CodeGenerator::Generate(const CompUnitSequenceAST& node) {
  if (node.comp_unit) {
    Generate(*node.comp_unit);
  }
  return "";
}

std::string CodeGenerator::Generate(const CompUnitListAST& node) {
  if (node.end_comp_unit) {
    Generate(*node.end_comp_unit);
  }
  if (node.comp_unit) {
    Generate(*node.comp_unit);
  }
  return "";
}

std::string CodeGenerator::Generate(const FunctionDefinitionUnitAST& node) {
  if (node.func_def) {
    Generate(*node.func_def);
  }
  return "";
}

std::string CodeGenerator::Generate(const DeclarationUnitAST& node) {
  if (node.decl) {
    Generate(*node.decl);
  }
  return "";
}

std::string CodeGenerator::Generate(const FuncDefAST& node) {
  context.ResetRegBuilder();
  auto symbol = std::make_shared<Symbol>(*node.ident, SymbolKind::FUNCTION, FuncInformation(*node.type));
  context.scope->define(symbol);

  // 初始化内存表示的 IRFunction
  IR::IRType ret_type = (*node.type == "void") ? IR::IRType::IR_VOID : IR::IRType::IR_INT32;
  auto func = std::make_shared<IR::IRFunction>(*node.ident, ret_type);
  context.ir_program->funcs.push_back(func);
  context.current_func = func.get();

  context.enterNewScope();
  std::cout << "fun @" << *node.ident << "(";
  if (node.func_f_params) {
    Generate(*node.func_f_params);
  }
  std::cout << ")";
  if (*node.type != "void") {
    std::cout << " : " << *node.type;
  }
  std::cout << " {\n";
  std::cout << "\%entry:\n";
  context.AddNewBlock("\%entry"); //添加 %entry 内存基本块

  context.cur_block_has_terminator = false;
  if (node.func_f_params) {
    ParamLocalization(*node.func_f_params);
  }

  if (node.block) {
    Generate(*node.block);
  }
  // 函数返回值没有返回值
  if (!context.cur_block_has_terminator && *node.type == "void") {
    std::cout << "  ret\n";
    if (context.current_block) {
      auto inst = std::make_shared<IR::IRReturn>(nullptr);
      context.current_block->AddInstruction(inst);
    }
  }

  std::cout << "}\n";
  context.exitScope();
  
  // 函数生成结束，重置活动指针
  context.current_func = nullptr;
  context.current_block = nullptr;
  return "";
}

std::string CodeGenerator::Generate(const FuncFParamsAST& node) {
  if (node.func_f_params.size() > 0) {
    std::cout << Generate(*node.func_f_params[0]);
    for (size_t i = 1; i < node.func_f_params.size(); i++) {
      std::cout << ", " << Generate(*node.func_f_params[i]);
    }
  }
  return "";
}

std::string CodeGenerator::Generate(const FuncFParamAST& node) {
  return "@" + *node.ident + ": " + *node.type;
}

void CodeGenerator::ParamLocalization(const FuncFParamsAST& node) {
  for (auto& param : node.func_f_params) {
    ParamLocalization(*param);
  }
}

void CodeGenerator::ParamLocalization(const FuncFParamAST& node) {
  std::string address = context.GetAddress(*node.ident);
  auto symbol = std::make_shared<Symbol>(*node.ident, SymbolKind::VARIABLE, address);
  context.scope->define(symbol);
  std::cout << "  " << address << " = " << "alloc " << *node.type << "\n";
  std::cout << "  store " << "@" + *node.ident << ", " << address << "\n";
}

std::string CodeGenerator::Generate(const BlockAST& node) {
  context.enterNewScope();
  for (auto& item : node.block_items) {
    Generate(*item);
  }
  context.exitScope();
  return "";
}

std::string CodeGenerator::Generate(const DeclBlockAST& node) {
  if (node.decl) {
    Generate(*node.decl);
  }
  return "";
}

std::string CodeGenerator::Generate(const StmtBlockAST& node) {
  if (node.stmt) {
    Generate(*node.stmt);
  }
  return "";
}

std::string CodeGenerator::Generate(const MatchedStmtWrapperAST& node) {
  if (node.matched_stmt) {
    return Generate(*node.matched_stmt);
  }
  return "";
}

std::string CodeGenerator::Generate(const UnmatchedStmtWrapperAST& node) {
  if (node.unmatched_stmt) {
    Generate(*node.unmatched_stmt);
  }
  return "";
}

std::string CodeGenerator::Generate(const IfUnmatchedStmtAST& node) {
  std::string cond_reg = Generate(*node.if_exp);
  std::string if_label = LabelBuilder::GetLabel("then", cond_reg);
  std::string end_label = LabelBuilder::GetLabel("end", cond_reg);
  std::cout << "  br " << cond_reg << ", " << if_label << ", " << end_label << "\n";
  std::cout << if_label << ":\n";
  context.AddNewBlock(if_label);

  context.cur_block_has_terminator = false;
  Generate(*node.if_stmt);
  if (!context.cur_block_has_terminator) {
    std::cout << "  jump " << end_label << "\n";
  }
  std::cout << end_label << ":\n";
  context.AddNewBlock(end_label);

  context.cur_block_has_terminator = false;
  return "";
}

std::string CodeGenerator::Generate(const IfElseUnmatchedStmtAST& node) {
  std::string cond_reg = Generate(*node.if_exp);
  std::string if_label = LabelBuilder::GetLabel("then", cond_reg);
  std::string else_label = LabelBuilder::GetLabel("else", cond_reg);
  std::string end_label = LabelBuilder::GetLabel("end", cond_reg);
  std::cout << "  br " << cond_reg << ", " << if_label << ", " << else_label << "\n";
  std::cout << if_label << ":\n";
  context.AddNewBlock(if_label);

  context.cur_block_has_terminator = false;
  Generate(*node.if_stmt);
  bool then_has_terminator = context.cur_block_has_terminator;
  if (!then_has_terminator) {
    std::cout << "  jump " << end_label << "\n";
  }
  std::cout << else_label << ":\n";
  context.AddNewBlock(else_label);

  context.cur_block_has_terminator = false;
  Generate(*node.else_stmt);
  bool else_has_terminator = context.cur_block_has_terminator;
  if (!else_has_terminator) {
    std::cout << "  jump " << end_label << "\n";
  }
  if (!then_has_terminator || !else_has_terminator) {
    std::cout << end_label << ":\n";
    context.AddNewBlock(end_label);
  } else {
    context.cur_block_has_terminator = true;
  }
  return "";
}

std::string CodeGenerator::Generate(const ReturnStmtAST& node) {
  if (context.cur_block_has_terminator) return "";
  std::string ret_val = "";
  if (node.exp) {
    ret_val = Generate(*node.exp);
    std::cout << "  ret " << ret_val << "\n";
  } else {
    std::cout << "  ret\n";
  }

  if (context.current_block) {
    auto val = node.exp ? context.GetIRValue(ret_val) : nullptr;
    auto inst = std::make_shared<IR::IRReturn>(val);
    context.current_block->AddInstruction(inst);
  }

  context.cur_block_has_terminator = true;
  return "";
}

std::string CodeGenerator::Generate(const AssignStmtAST& node) {
  if (context.cur_block_has_terminator) return "";
  if (node.lval) {
    std::string rhs_reg = Generate(*node.exp);
    std::string lhs_reg = node.lval->DumpOnlyAddress(context);
    std::cout << "  store " << rhs_reg << ", " << lhs_reg << "\n";
  }
  return "";
}

std::string CodeGenerator::Generate(const ExpStmtAST& node) {
  if (context.cur_block_has_terminator) return "";
  if (node.exp) {
    Generate(*node.exp);
  }
  return "";
}

std::string CodeGenerator::Generate(const EmptyStmtAST& node) {
  return "";
}

std::string CodeGenerator::Generate(const BlockStmtAST& node) {
  if (node.block) {
    Generate(*node.block);
  }
  return "";
}

std::string CodeGenerator::Generate(const IfMatchedStmtAST& node) {
  if (context.cur_block_has_terminator) return "";
  std::string cond_reg = Generate(*node.if_exp);
  std::string if_label = LabelBuilder::GetLabel("then", cond_reg);
  std::string else_label = LabelBuilder::GetLabel("else", cond_reg);
  std::string end_label = LabelBuilder::GetLabel("end", cond_reg);
  std::cout << "  br " << cond_reg << ", " << if_label << ", " << else_label << "\n";
  std::cout << if_label << ":\n";
  context.AddNewBlock(if_label);

  context.cur_block_has_terminator = false;
  Generate(*node.if_stmt);
  bool then_has_terminator = context.cur_block_has_terminator;
  if (!then_has_terminator) {
    std::cout << "  jump " << end_label << "\n";
  }
  std::cout << else_label << ":\n";
  context.AddNewBlock(else_label);

  context.cur_block_has_terminator = false;
  Generate(*node.else_stmt);
  bool else_has_terminator = context.cur_block_has_terminator;
  if (!else_has_terminator) {
    std::cout << "  jump " << end_label << "\n";
  }
  if (!then_has_terminator || !else_has_terminator) {
    std::cout << end_label << ":\n";
    context.AddNewBlock(end_label);
  } else {
    context.cur_block_has_terminator = true;
  }
  return "";
}

std::string CodeGenerator::Generate(const WhileStmtAST& node) {
  if (context.cur_block_has_terminator) return "";
  std::cout << "  // while \n";
  std::string loop_label = LabelBuilder::GetLabel("while_entry");
  std::string continue_loop_label = LabelBuilder::GetLabel("while_body");
  std::string end_label = LabelBuilder::GetLabel("end");

  context.enterNewWhileScope(loop_label, continue_loop_label, end_label);
  std::cout << "  jump " << loop_label << "\n";
  std::cout << loop_label << ":\n";
  context.AddNewBlock(loop_label);

  std::string cond_reg = Generate(*node.while_exp);
  std::cout << "  br " << cond_reg << ", " << continue_loop_label << ", " << end_label << "\n";
  std::cout << continue_loop_label << ":\n";
  context.AddNewBlock(continue_loop_label);

  Generate(*node.while_stmt);
  if (!context.cur_block_has_terminator) {
    std::cout << "  jump " << loop_label << "\n";
  }
  std::cout << end_label << ":\n";
  context.AddNewBlock(end_label);

  context.cur_block_has_terminator = false;
  context.exitWhileScope();
  return "";
}

std::string CodeGenerator::Generate(const BreakStmtAST& node) {
  if (context.cur_block_has_terminator) return "";
  WhileLabelContext while_label_context = context.GetWhileLabel();
  std::cout << "  jump " << while_label_context.while_label_end << "\n";
  std::string tool_label = while_label_context.while_label_continue + NumBuilder::GetNum();
  std::cout << tool_label << ":\n";
  context.AddNewBlock(tool_label);
  return "";
}

std::string CodeGenerator::Generate(const ContinueStmtAST& node) {
  if (context.cur_block_has_terminator) return "";
  WhileLabelContext while_label_context = context.GetWhileLabel();
  std::cout << "  jump " << while_label_context.while_label_begin << "\n";
  std::string tool_label = while_label_context.while_label_continue + NumBuilder::GetNum();
  std::cout << tool_label << ":\n";
  context.AddNewBlock(tool_label);
  return "";
}

std::string CodeGenerator::Generate(const ExpAST& node) {
  if (node.lor_exp) {
    return Generate(*node.lor_exp);
  }
  return "";
}

std::string CodeGenerator::Generate(const UnaryOPExpAST& node) {
  std::string src_reg = Generate(*node.unary_exp);
  if (node.op->op == "+") {
    return src_reg;
  }
  else if (node.op->op == "-") {
    std::string dest_reg = context.NewReg();
    std::cout << "  " << dest_reg << " = sub 0, " << src_reg << "\n";
    if (context.current_block) {
      auto lhs = context.GetIRValue("0");
      auto rhs = context.GetIRValue(src_reg);
      auto inst = std::make_shared<IR::IRBinary>(dest_reg, IR::IRBinaryOp::SUB, lhs, rhs);
      context.current_block->AddInstruction(inst);
      context.SetIRValue(dest_reg, inst);
    }
    return dest_reg;
  }
  else if (node.op->op == "!") {
    std::string dest_reg = context.NewReg();
    std::cout << "  " << dest_reg << " = eq 0, " << src_reg << "\n";
    if (context.current_block) {
      auto lhs = context.GetIRValue("0");
      auto rhs = context.GetIRValue(src_reg);
      auto inst = std::make_shared<IR::IRBinary>(dest_reg, IR::IRBinaryOp::EQ, lhs, rhs);
      context.current_block->AddInstruction(inst);
      context.SetIRValue(dest_reg, inst);
    }
    return dest_reg;
  }
  return "";
}

std::string CodeGenerator::Generate(const PrimaryUnaryExpAST& node) {
  if (node.primary_exp) {
    return Generate(*node.primary_exp);
  }
  return "";
}

std::string CodeGenerator::Generate(const FuncCallExpAST& node) {
  std::string func_name = *node.ident;
  auto func_symbol = context.scope->lookup(func_name);
  if (func_symbol == nullptr) {
    std::cerr << "func " << func_name << " not found" << "\n";
  }
  auto func_inf = std::get<FuncInformation>(func_symbol->value);
  std::string param;
  if (node.func_r_params) {
    param = Generate(*node.func_r_params);
  } else {
    param = "";
  }
  if (func_inf.ret_type != "void") {
    std::string func_reg = context.NewReg();
    std::cout << "  " << func_reg << " = call @" << func_name << "(" << param << ")\n";
    return func_reg;
  } else {
    std::cout << "  call @" << func_name << "(" << param << ")" << "\n";
    return "";
  }
}

std::string CodeGenerator::Generate(const FuncRParamsAST& node) {
  std::string params;
  if (node.func_r_params.size() > 0) {
    params += Generate(*node.func_r_params[0]);
    for (size_t i = 1; i < node.func_r_params.size(); i++) {
      params = params + ", " + Generate(*node.func_r_params[i]);
    }
  }
  return params;
}

std::string CodeGenerator::Generate(const ParenthesizedExprAST& node) {
  if (node.exp) {
    return Generate(*node.exp);
  }
  return "";
}

std::string CodeGenerator::Generate(const LValuePrimaryExprAST& node) {
  return Generate(*node.lval);
}

std::string CodeGenerator::Generate(const NumberPrimaryExprAST& node) {
  return std::to_string(node.value);
}

std::string CodeGenerator::Generate(const OpAST& node) {
  return node.op;
}

std::string CodeGenerator::Generate(const UnaryMulExpAST& node) {
  if (node.unary_exp) {
    return Generate(*node.unary_exp);
  }
  return "";
}

std::string CodeGenerator::Generate(const BinaryMulExprAST& node) {
  std::string lhs_reg = Generate(*node.mul_exp);
  std::string rhs_reg = Generate(*node.unary_exp);
  std::string dest_reg = context.NewReg();
  IR::IRBinaryOp op = IR::IRBinaryOp::MUL;
  if (node.op->op == "*") {
    std::cout << "  " << dest_reg << " = mul " << lhs_reg << ", " << rhs_reg << "\n";
    op = IR::IRBinaryOp::MUL;
  } else if (node.op->op == "/") {
    std::cout << "  " << dest_reg << " = div " << lhs_reg << ", " << rhs_reg << "\n";
    op = IR::IRBinaryOp::DIV;
  } else if (node.op->op == "%") {
    std::cout << "  " << dest_reg << " = mod " << lhs_reg << ", " << rhs_reg << "\n";
    op = IR::IRBinaryOp::MOD;
  }

  if (context.current_block) {
    auto lhs = context.GetIRValue(lhs_reg);
    auto rhs = context.GetIRValue(rhs_reg);
    auto inst = std::make_shared<IR::IRBinary>(dest_reg, op, lhs, rhs);
    context.current_block->AddInstruction(inst);
    context.SetIRValue(dest_reg, inst);
  }
  return dest_reg;
}

std::string CodeGenerator::Generate(const MulAddExpAST& node) {
  if (node.mul_exp) {
    return Generate(*node.mul_exp);
  }
  return "";
}

std::string CodeGenerator::Generate(const BinaryAddExprAST& node) {
  std::string lhs_reg = Generate(*node.add_exp);
  std::string rhs_reg = Generate(*node.mul_exp);
  std::string dest_reg = context.NewReg();
  IR::IRBinaryOp op = IR::IRBinaryOp::ADD;
  if (node.op->op == "+") {
    std::cout << "  " << dest_reg << " = add " << lhs_reg << ", " << rhs_reg << "\n";
    op = IR::IRBinaryOp::ADD;
  } else if (node.op->op == "-") {
    std::cout << "  " << dest_reg << " = sub " << lhs_reg << ", " << rhs_reg << "\n";
    op = IR::IRBinaryOp::SUB;
  }

  if (context.current_block) {
    auto lhs = context.GetIRValue(lhs_reg);
    auto rhs = context.GetIRValue(rhs_reg);
    auto inst = std::make_shared<IR::IRBinary>(dest_reg, op, lhs, rhs);
    context.current_block->AddInstruction(inst);
    context.SetIRValue(dest_reg, inst);
  }
  return dest_reg;
}

std::string CodeGenerator::Generate(const AddRelExpAST& node) {
  if (node.add_exp) {
    return Generate(*node.add_exp);
  }
  return "";
}

std::string CodeGenerator::Generate(const BinaryRelExprAST& node) {
  std::string lhs_reg = Generate(*node.rel_exp);
  std::string rhs_reg = Generate(*node.add_exp);
  std::string dest_reg = context.NewReg();
  IR::IRBinaryOp op = IR::IRBinaryOp::LT;
  if (node.op->op == "<") {
    std::cout << "  " << dest_reg << " = lt " << lhs_reg << ", " << rhs_reg << "\n";
    op = IR::IRBinaryOp::LT;
  } else if (node.op->op == ">") {
    std::cout << "  " << dest_reg << " = gt " << lhs_reg << ", " << rhs_reg << "\n";
    op = IR::IRBinaryOp::GT;
  } else if (node.op->op == "<=") {
    std::cout << "  " << dest_reg << " = le " << lhs_reg << ", " << rhs_reg << "\n";
    op = IR::IRBinaryOp::LE;
  } else if (node.op->op == ">=") {
    std::cout << "  " << dest_reg << " = ge " << lhs_reg << ", " << rhs_reg << "\n";
    op = IR::IRBinaryOp::GE;
  }

  if (context.current_block) {
    auto lhs = context.GetIRValue(lhs_reg);
    auto rhs = context.GetIRValue(rhs_reg);
    auto inst = std::make_shared<IR::IRBinary>(dest_reg, op, lhs, rhs);
    context.current_block->AddInstruction(inst);
    context.SetIRValue(dest_reg, inst);
  }
  return dest_reg;
}

std::string CodeGenerator::Generate(const RelEqExpAST& node) {
  if (node.rel_exp) {
    return Generate(*node.rel_exp);
  }
  return "";
}

std::string CodeGenerator::Generate(const BinaryEqExprAST& node) {
  std::string lhs_reg = Generate(*node.eq_exp);
  std::string rhs_reg = Generate(*node.rel_exp);
  std::string dest_reg = context.NewReg();
  IR::IRBinaryOp op = IR::IRBinaryOp::EQ;
  if (node.op->op == "==") {
    std::cout << "  " << dest_reg << " = eq " << lhs_reg << ", " << rhs_reg << "\n";
    op = IR::IRBinaryOp::EQ;
  } else if (node.op->op == "!=") {
    std::cout << "  " << dest_reg << " = ne " << lhs_reg << ", " << rhs_reg << "\n";
    op = IR::IRBinaryOp::NE;
  }

  if (context.current_block) {
    auto lhs = context.GetIRValue(lhs_reg);
    auto rhs = context.GetIRValue(rhs_reg);
    auto inst = std::make_shared<IR::IRBinary>(dest_reg, op, lhs, rhs);
    context.current_block->AddInstruction(inst);
    context.SetIRValue(dest_reg, inst);
  }
  return dest_reg;
}

std::string CodeGenerator::Generate(const EqLAndExpAST& node) {
  if (node.eq_exp) {
    return Generate(*node.eq_exp);
  }
  return "";
}

std::string CodeGenerator::Generate(const BinaryLogicalAndExprAST& node) {
  if (node.op->op == "&&") {
    std::string lhs_reg = Generate(*node.land_exp);
    std::string temp_address = context.NewReg();

    std::cout << "  // " << temp_address << " : " << lhs_reg << " && " << "rhs_reg\n";
    std::string then_label = LabelBuilder::GetLabel("and_calc_rhs");
    std::string end_label = LabelBuilder::GetLabel("and_end");
    std::cout << "  " << temp_address << " = alloc i32\n";
    std::cout << "  " << "store 0, " << temp_address << "\n";
    std::cout << "  br " << lhs_reg << " , " << then_label << " , " << end_label << "\n";
    // then块
    std::cout << then_label << ":\n";
    context.cur_block_has_terminator = false;
    std::string rhs_reg = Generate(*node.eq_exp);
    std::string temp_reg = context.NewReg();
    std::cout << "  " <<  temp_reg << " = ne " << rhs_reg << ", 0\n";
    std::cout << "  " << "store " << temp_reg << ", " << temp_address << "\n";
    std::cout << "  jump " << end_label << "\n";
    std::cout << end_label << ":\n";
    context.cur_block_has_terminator = false;

    std::string return_reg = context.NewReg();
    std::cout << "  " << return_reg << " = load " << temp_address << "\n";
    return return_reg;
  }
  return "";
}

std::string CodeGenerator::Generate(const LAndLOrExpAST& node) {
  if (node.land_exp) {
    return Generate(*node.land_exp);
  }
  return "";
}

std::string CodeGenerator::Generate(const BinaryLogicalOrExprAST& node) {
  if (node.op->op == "||") {
    std::string lhs_reg = Generate(*node.lor_exp);
    std::string temp_address = context.NewReg();

    std::cout << "  // " << temp_address << " : " << lhs_reg << " || " << "rhs_reg" << "\n";
    std::string then_label = LabelBuilder::GetLabel("or_calc_rhs", temp_address);
    std::string end_label = LabelBuilder::GetLabel("or_end", temp_address);
    std::cout << "  " << temp_address << " = alloc i32\n";
    std::cout << "  " << "store 1, " << temp_address << "\n";
    std::cout << "  " << "br " << lhs_reg <<", " << end_label << ", " << then_label << "\n";
    // then块
    std::cout << then_label << ":\n";
    context.cur_block_has_terminator = false;
    std::string rhs_reg = Generate(*node.land_exp);
    std::string temp_reg = context.NewReg();
    std::cout << "  " << temp_reg << " = ne 0, " << rhs_reg << "\n";
    std::cout << "  " << "store " << temp_reg << ", " << temp_address << "\n";
    std::cout << "  jump " << end_label << "\n";
    std::cout << end_label << ":\n";
    context.cur_block_has_terminator = false;

    std::string return_reg = context.NewReg();
    std::cout << "  " << return_reg << " = load " << temp_address << "\n";
    return return_reg;
  }
  return "";
}

std::string CodeGenerator::Generate(const LValAST& node) {
  auto sym = context.scope->lookup(*node.ident);
  if (sym == nullptr) {
    std::cout << "Error: LVal " << *node.ident << " not found\n";
    return "";
  }
  if (sym->kind == SymbolKind::VARIABLE) {
    std::string address = std::get<std::string>(sym->value);
    std::string new_reg = context.NewReg();
    std::cout << "  " << new_reg << " = load " << address << "\n";
    return new_reg;
  } else if (sym->kind == SymbolKind::CONSTANT) {
    return std::to_string(std::get<int>(sym->value));
  } else if (sym->kind == SymbolKind::ARRAY) {
    auto temp = context.NewReg();
    auto index = Generate(*node.index_exp[0]);
    auto result = context.NewReg();
    std::cout << "  " << temp << " = getelemptr " << std::get<ArrayInformation>(sym->value).address << ", " << index << "\n";
    std::cout << "  " << result << " = load " << temp << "\n";
    return result;
  }
  else
  {
    std::cout << "Error: LVal " << *node.ident << " is not a variable or constant\n";
    return "";
  }
}

std::string CodeGenerator::Generate(const ConstExpAST& node) {
  if (node.exp) {
    return Generate(*node.exp);
  }
  return "";
}

std::string CodeGenerator::Generate(const ConstDefAST& node) {
  int constinit_val = node.const_init_val->getValue(context);
  auto sym = std::make_shared<Symbol>(*node.ident, SymbolKind::CONSTANT, constinit_val);
  context.scope->define(sym);
  return "";
}

std::string CodeGenerator::Generate(const ArrayConstDefAST& node) {
  auto raw_init_vals = node.const_init_val->getValues(context);
  auto arr_size = node.const_exp->getValue(context);
  auto address = context.GetAddress(*node.ident);
  ArrayInformation arr_info(address, arr_size);
  auto sym = std::make_shared<Symbol>(address, SymbolKind::CONST_ARRAY);
  sym->value = arr_info;

  auto init_vals = std::vector<std::string>(arr_size);
  for (size_t i = 0; i < raw_init_vals.size(); i++) {
    init_vals[i] = std::to_string(raw_init_vals[i]);
  }
  for (size_t i = raw_init_vals.size(); i < (size_t)arr_size; i++) {
    init_vals[i] = "0";
  }
  if (context.scope->isGlobal()) {
    std::string str;
    for (int i = 0; i < arr_size; i++) {
      str += init_vals[i];
      if (i != arr_size - 1) {
        str += ", ";
      }
    }
    std::cout << "global " << address << " = alloc [i32, " << arr_size << "], {" << str << "}\n";
  } else {
    std::cout << "  " << address  << " = alloc [i32, " << arr_size << "]\n";
    for (int i = 0; i < arr_size; i++) {
      auto temp = context.NewReg();
      std::cout << "  " << temp << " = " << "getelemptr " << address << ", " << i << "\n";
      std::cout << "  store " << init_vals[i] << ", " << temp << "\n";
    }
  }
  context.scope->define(sym);
  return "";
}

std::string CodeGenerator::Generate(const ConstExpConstInitValAST& node) {
  if (node.const_exp) {
    return Generate(*node.const_exp);
  }
  return "";
}

std::string CodeGenerator::Generate(const ConstInitValListAST& node) {
  return "";
}

std::string CodeGenerator::Generate(const CompoundConstInitValueAST& node) {
  if (node.const_init_val_list) {
    return Generate(*node.const_init_val_list);
  }
  return "";
}

std::string CodeGenerator::Generate(const ConstDeclWrapperAST& node) {
  if (node.const_decl) {
    return Generate(*node.const_decl);
  }
  return "";
}

std::string CodeGenerator::Generate(const VarDeclWrapperAST& node) {
  if (node.var_decl) {
    return Generate(*node.var_decl);
  }
  return "";
}

std::string CodeGenerator::Generate(const ConstDeclAST& node) {
  assert(*node.type == "i32");
  if (node.const_def_list) {
    Generate(*node.const_def_list);
  }
  return "";
}

std::string CodeGenerator::Generate(const ConstDefListAST& node) {
  for (auto& const_def : node.const_defs) {
    Generate(*const_def);
  }
  return "";
}

std::string CodeGenerator::Generate(const VarDeclAST& node) {
  context.decl_type_context = std::make_shared<std::string>(*node.type);
  if (node.var_def_list) {
    Generate(*node.var_def_list);
  }
  return "";
}

std::string CodeGenerator::Generate(const VarDefListAST& node) {
  for (auto& var_def : node.var_defs) {
    if (!var_def) {
      std::cerr << "Error: VarDef is nullptr\n";
      continue;
    }
    Generate(*var_def);
  }
  return "";
}

std::string CodeGenerator::Generate(const SimpleVarDefAST& node) {
  std::string address = context.GetAddress(*node.ident);
  auto sym = std::make_shared<Symbol>(*node.ident, SymbolKind::VARIABLE, address);
  std::string b_type = *context.decl_type_context;

  if (context.scope->isGlobal()) {
    std::cout << "global " << address << " = alloc " << b_type << ", " << "zeroinit\n";
  } else {
    std::cout << "  " << address << " = alloc " << b_type << "\n";
  }
  context.scope->define(sym);
  return "";
}

std::string CodeGenerator::Generate(const InitializedVarDefAST& node) {
  std::string address = context.GetAddress(*node.ident);
  std::string b_type = *context.decl_type_context;
  auto sym = std::make_shared<Symbol>(*node.ident, SymbolKind::VARIABLE, address);
  if (context.scope->isGlobal()) {
    std::cout << "global " << address << " = alloc " << b_type << ", " << node.init_val->getValue(context) << "\n";
  } else {
    std::cout << "  " << address << " = alloc " << b_type << "\n";
    std::string init_val_reg = Generate(*node.init_val);
    std::cout << "  " << "store " << init_val_reg << ", " << address << "\n";
  }
  context.scope->define(sym);
  return "";
}

std::string CodeGenerator::Generate(const ArrayVarDefAST& node) {
  std::string address = context.GetAddress(*node.ident);
  std::string b_type = *context.decl_type_context;
  auto arr_size = node.const_exp->getValue(context);
  auto sym = std::make_shared<Symbol>(address, SymbolKind::ARRAY);
  sym->value = ArrayInformation(address, arr_size);

  if (context.scope->isGlobal()) {
    std::cout << "global " << address << " = alloc [" << b_type << ", " << arr_size << "]\n";
  } else {
    std::cout << "  " << address << " = alloc [" << b_type << ", " << arr_size << "]\n";
  }
  context.scope->define(sym);
  return "";
}

std::string CodeGenerator::Generate(const ArrayInitializedVarDefAST& node) {
  std::string address = context.GetAddress(*node.ident);
  std::string b_type = *context.decl_type_context;
  auto arr_size = node.const_exp->getValue(context);
  auto sym = std::make_shared<Symbol>(*node.ident, SymbolKind::ARRAY);
  sym->value = ArrayInformation(address, arr_size);
  if (context.scope->isGlobal()) {
    auto raw_init_vals = node.init_val_list->getValue(context);
    auto init_vals = std::vector<std::string>(arr_size);
    for (size_t i = 0; i < raw_init_vals.size(); i++) {
      init_vals[i] = std::to_string(raw_init_vals[i]);
    }
    for (size_t i = raw_init_vals.size(); i < (size_t)arr_size; i++) {
      init_vals[i] = "0";
    }
    std::string str;
    for (int i = 0; i < arr_size; i++) {
      str += init_vals[i];
      if (i != arr_size - 1) {
        str += ", ";
      }
    }
    std::cout << "global " << address << " = alloc [i32, " << arr_size << "], {" << str << "}\n";
  } else {
    std::vector<std::string> init_vals;
    for (auto& val : node.init_val_list->init_vals) {
      init_vals.push_back(Generate(*val));
    }
    for (size_t i = init_vals.size(); i < (size_t)arr_size; i++) {
      init_vals.push_back("0");
    }
    std::cout << "  " << address  << " = alloc [i32, " << arr_size << "]\n";
    for (int i = 0; i < arr_size; i++) {
      auto temp = context.NewReg();
      std::cout << "  " << temp << " = " << "getelemptr " << address << ", " << i << "\n";
      std::cout << "  store " << init_vals[i] << ", " << temp << "\n";
    }
  }
  context.scope->define(sym);
  return "";
}

std::string CodeGenerator::Generate(const InitValAST& node) {
  if (node.exp) {
    return Generate(*node.exp);
  }
  return "";
}

std::string CodeGenerator::Generate(const InitValListAST& node) {
  return "";
}


