#include "codegen.hpp"
#include "ast.hpp"
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
    context.AddDecl(std::make_shared<IR::IRFuncDecl>("getint", IR::IRType::IR_INT32, std::vector<IR::IRType>{}));
    context.AddDecl(std::make_shared<IR::IRFuncDecl>("getch", IR::IRType::IR_INT32, std::vector<IR::IRType>{}));
    context.AddDecl(std::make_shared<IR::IRFuncDecl>("getarray", IR::IRType::IR_INT32, std::vector<IR::IRType>{IR::IRType::IR_POINTER}));
    context.AddDecl(std::make_shared<IR::IRFuncDecl>("putint", IR::IRType::IR_VOID, std::vector<IR::IRType>{IR::IRType::IR_INT32}));
    context.AddDecl(std::make_shared<IR::IRFuncDecl>("putch", IR::IRType::IR_VOID, std::vector<IR::IRType>{IR::IRType::IR_INT32}));
    context.AddDecl(std::make_shared<IR::IRFuncDecl>("putarray", IR::IRType::IR_VOID, std::vector<IR::IRType>{IR::IRType::IR_INT32, IR::IRType::IR_POINTER}));
    context.AddDecl(std::make_shared<IR::IRFuncDecl>("starttime", IR::IRType::IR_VOID, std::vector<IR::IRType>{}));
    context.AddDecl(std::make_shared<IR::IRFuncDecl>("stoptime", IR::IRType::IR_VOID, std::vector<IR::IRType>{}));

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
  context.SetIRValue("@" + *node.ident, func);

  context.enterNewScope();
  if (node.func_f_params) {
    Generate(*node.func_f_params);
  }
  if (*node.type != "void") {
  }
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
    if (context.current_block) {
      auto inst = std::make_shared<IR::IRReturn>(nullptr);
      context.current_block->AddInstruction(inst);
    }
  }

  // 处理不可到达的情况
  if (!context.cur_block_has_terminator && *node.type == "i32") {
    if (context.current_block) {
      auto val = std::make_shared<IR::IRConstant>(0);
      auto inst = std::make_shared<IR::IRReturn>(val);
      context.current_block->AddInstruction(inst);
    }
  }

  context.exitScope();
  
  // 函数生成结束，重置活动指针
  context.current_func = nullptr;
  context.current_block = nullptr;
  return "";
}

std::string CodeGenerator::Generate(const FuncFParamsAST& node) {
  if (node.func_f_params.size() > 0) {
    for (size_t i = 1; i < node.func_f_params.size(); i++) {
    }
  }
  return "";
}

static std::string GetParamType(const FuncFParamAST& node, Context& context) {
  if (!node.is_array) {
    return *node.type;
  }
  std::string base_type = "i32";
  std::vector<int> dims;
  for (auto& expr : node.array_dims) {
    dims.push_back(expr->getValue(context));
  }
  std::string arr_type = base_type;
  for (auto it = dims.rbegin(); it != dims.rend(); ++it) {
    arr_type = "[" + arr_type + ", " + std::to_string(*it) + "]";
  }
  return "*" + arr_type;
}

std::string CodeGenerator::Generate(const FuncFParamAST& node) {
  return "@" + *node.ident + ": " + GetParamType(node, context);
}

void CodeGenerator::ParamLocalization(const FuncFParamsAST& node) {
  for (auto& param : node.func_f_params) {
    ParamLocalization(*param);
  }
}

void CodeGenerator::ParamLocalization(const FuncFParamAST& node) {
  std::string address = context.GetAddress(*node.ident);
  std::string param_type_str = GetParamType(node, context);

  if (node.is_array) {
    std::vector<int> dims;
    for (auto& expr : node.array_dims) {
      dims.push_back(expr->getValue(context));
    }
    auto symbol = std::make_shared<Symbol>(*node.ident, SymbolKind::ARRAY);
    symbol->value = ArrayInformation(address, dims, {}, true); // is_param = true
    context.scope->define(symbol);
  } else {
    auto symbol = std::make_shared<Symbol>(*node.ident, SymbolKind::VARIABLE, address);
    context.scope->define(symbol);
  }


  if (context.current_block) {
    std::string param_name = "@" + *node.ident;
    IR::IRType param_type = (!node.is_array && *node.type == "i32") ? IR::IRType::IR_INT32 : IR::IRType::IR_POINTER;
    auto param_sym = std::make_shared<IR::IRFuncParam>(param_name, param_type, param_type_str);
    context.SetIRValue(param_name, param_sym);

    if (context.current_func) {
      context.current_func->params.push_back(param_sym);
    }

    auto alloc_inst = std::make_shared<IR::IRAlloc>(address, param_type_str);
    context.current_block->AddInstruction(alloc_inst);
    context.SetIRValue(address, alloc_inst);

    auto store_inst = std::make_shared<IR::IRStore>(param_sym, alloc_inst);
    context.current_block->AddInstruction(store_inst);
  }
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

  auto then_bb = std::make_shared<IR::IRBasicBlock>(if_label);
  auto end_bb = std::make_shared<IR::IRBasicBlock>(end_label);
  context.SetIRValue(if_label, then_bb);
  context.SetIRValue(end_label, end_bb);

  if (context.current_block) {
    auto cond_val = context.GetIRValue(cond_reg);
    auto br_inst = std::make_shared<IR::IRBranch>(cond_val, then_bb, end_bb);
    context.current_block->AddInstruction(br_inst);
  }

  context.AddExistingBlock(then_bb);

  context.cur_block_has_terminator = false;
  Generate(*node.if_stmt);
  if (!context.cur_block_has_terminator) {
    if (context.current_block) {
      auto jump_inst = std::make_shared<IR::IRJump>(end_bb);
      context.current_block->AddInstruction(jump_inst);
    }
  }
  context.AddExistingBlock(end_bb);

  context.cur_block_has_terminator = false;
  return "";
}

std::string CodeGenerator::Generate(const IfElseUnmatchedStmtAST& node) {
  std::string cond_reg = Generate(*node.if_exp);
  std::string if_label = LabelBuilder::GetLabel("then", cond_reg);
  std::string else_label = LabelBuilder::GetLabel("else", cond_reg);
  std::string end_label = LabelBuilder::GetLabel("end", cond_reg);

  auto then_bb = std::make_shared<IR::IRBasicBlock>(if_label);
  auto else_bb = std::make_shared<IR::IRBasicBlock>(else_label);
  auto end_bb = std::make_shared<IR::IRBasicBlock>(end_label);
  context.SetIRValue(if_label, then_bb);
  context.SetIRValue(else_label, else_bb);
  context.SetIRValue(end_label, end_bb);

  if (context.current_block) {
    auto cond_val = context.GetIRValue(cond_reg);
    auto br_inst = std::make_shared<IR::IRBranch>(cond_val, then_bb, else_bb);
    context.current_block->AddInstruction(br_inst);
  }

  context.AddExistingBlock(then_bb);

  context.cur_block_has_terminator = false;
  Generate(*node.if_stmt);
  bool then_has_terminator = context.cur_block_has_terminator;
  if (!then_has_terminator) {
    if (context.current_block) {
      auto jump_inst = std::make_shared<IR::IRJump>(end_bb);
      context.current_block->AddInstruction(jump_inst);
    }
  }
  context.AddExistingBlock(else_bb);

  context.cur_block_has_terminator = false;
  Generate(*node.else_stmt);
  bool else_has_terminator = context.cur_block_has_terminator;
  if (!else_has_terminator) {
    if (context.current_block) {
      auto jump_inst = std::make_shared<IR::IRJump>(end_bb);
      context.current_block->AddInstruction(jump_inst);
    }
  }
  if (!then_has_terminator || !else_has_terminator) {
    context.AddExistingBlock(end_bb);
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
  } else {
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
    auto sym = context.scope->lookup(*node.lval->ident);
    if (sym == nullptr) {
      std::cerr << "Error: LVal " << *node.lval->ident << " not found\n";
      return "";
    }

    std::string lhs_reg;
    std::shared_ptr<IR::IRValue> ptr_val = nullptr;

    if (sym->kind == SymbolKind::VARIABLE) {
      std::string ptr = std::get<std::string>(sym->value);
      lhs_reg = ptr;
      ptr_val = context.GetIRValue(ptr);
    } else if (sym->kind == SymbolKind::ARRAY || sym->kind == SymbolKind::CONST_ARRAY) {
      auto arr_info = std::get<ArrayInformation>(sym->value);
      
      std::string prev_reg = arr_info.address;
      std::shared_ptr<IR::IRValue> prev_val = context.GetIRValue(arr_info.address);

      if (arr_info.is_param) {
        std::string loaded_ptr = context.NewReg();
        if (context.current_block) {
          auto load_ptr_inst = std::make_shared<IR::IRLoad>(loaded_ptr, prev_val);
          context.current_block->AddInstruction(load_ptr_inst);
          context.SetIRValue(loaded_ptr, load_ptr_inst);
          prev_val = load_ptr_inst;
        }
        prev_reg = loaded_ptr;
      }

      for (size_t i = 0; i < node.lval->index_exp.size(); i++) {
        auto index_reg = Generate(*node.lval->index_exp[i]);
        std::string addr_reg = context.NewReg();
        if (arr_info.is_param && i == 0) {
          if (context.current_block) {
            auto index_val = context.GetIRValue(index_reg);
            int elem_size = 4;
            for (size_t j = i; j < arr_info.dims.size(); j++) {
              elem_size *= arr_info.dims[j];
            }
            auto getptr_inst = std::make_shared<IR::IRGetPtr>(addr_reg, prev_val, index_val, elem_size);
            context.current_block->AddInstruction(getptr_inst);
            context.SetIRValue(addr_reg, getptr_inst);
            prev_val = getptr_inst;
          }
        } else {
          if (context.current_block) {
            auto index_val = context.GetIRValue(index_reg);
            int elem_size = 4;
            size_t start_idx = arr_info.is_param ? i : i + 1;
            for (size_t j = start_idx; j < arr_info.dims.size(); j++) {
              elem_size *= arr_info.dims[j];
            }
            auto getelemptr_inst = std::make_shared<IR::IRGetElemPtr>(addr_reg, prev_val, index_val, elem_size);
            context.current_block->AddInstruction(getelemptr_inst);
            context.SetIRValue(addr_reg, getelemptr_inst);
            prev_val = getelemptr_inst;
          }
        }
        prev_reg = addr_reg;
      }

      int total_dims = arr_info.dims.size() + (arr_info.is_param ? 1 : 0);
      if (node.lval->index_exp.size() < (size_t)total_dims) {
        std::string addr_reg = context.NewReg();
        if (context.current_block) {
          auto index_val = context.GetIRValue("0");
          size_t i = node.lval->index_exp.size();
          int elem_size = 4;
          size_t start_idx = arr_info.is_param ? i : i + 1;
          for (size_t j = start_idx; j < arr_info.dims.size(); j++) {
            elem_size *= arr_info.dims[j];
          }
          auto getelemptr_inst = std::make_shared<IR::IRGetElemPtr>(addr_reg, prev_val, index_val, elem_size);
          context.current_block->AddInstruction(getelemptr_inst);
          context.SetIRValue(addr_reg, getelemptr_inst);
          prev_val = getelemptr_inst;
        }
        lhs_reg = addr_reg;
        ptr_val = prev_val;
      } else {
        lhs_reg = prev_reg;
        ptr_val = prev_val;
      }
    }

    if (context.current_block) {
      auto val = context.GetIRValue(rhs_reg);
      auto store_inst = std::make_shared<IR::IRStore>(val, ptr_val);
      context.current_block->AddInstruction(store_inst);
    }
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

  auto then_bb = std::make_shared<IR::IRBasicBlock>(if_label);
  auto else_bb = std::make_shared<IR::IRBasicBlock>(else_label);
  auto end_bb = std::make_shared<IR::IRBasicBlock>(end_label);
  context.SetIRValue(if_label, then_bb);
  context.SetIRValue(else_label, else_bb);
  context.SetIRValue(end_label, end_bb);

  if (context.current_block) {
    auto cond_val = context.GetIRValue(cond_reg);
    auto br_inst = std::make_shared<IR::IRBranch>(cond_val, then_bb, else_bb);
    context.current_block->AddInstruction(br_inst);
  }

  context.AddExistingBlock(then_bb);

  context.cur_block_has_terminator = false;
  Generate(*node.if_stmt);
  bool then_has_terminator = context.cur_block_has_terminator;
  if (!then_has_terminator) {
    if (context.current_block) {
      auto jump_inst = std::make_shared<IR::IRJump>(end_bb);
      context.current_block->AddInstruction(jump_inst);
    }
  }
  context.AddExistingBlock(else_bb);

  context.cur_block_has_terminator = false;
  Generate(*node.else_stmt);
  bool else_has_terminator = context.cur_block_has_terminator;
  if (!else_has_terminator) {
    if (context.current_block) {
      auto jump_inst = std::make_shared<IR::IRJump>(end_bb);
      context.current_block->AddInstruction(jump_inst);
    }
  }
  if (!then_has_terminator || !else_has_terminator) {
    context.AddExistingBlock(end_bb);
  } else {
    context.cur_block_has_terminator = true;
  }
  return "";
}

std::string CodeGenerator::Generate(const WhileStmtAST& node) {
  if (context.cur_block_has_terminator) return "";
  std::string loop_label = LabelBuilder::GetLabel("while_entry");
  // 这里的while_body是给while开头的br或者break使用的
  std::string continue_loop_label = LabelBuilder::GetLabel("while_body");
  std::string end_label = LabelBuilder::GetLabel("end");

  auto loop_bb = std::make_shared<IR::IRBasicBlock>(loop_label);
  auto continue_loop_bb = std::make_shared<IR::IRBasicBlock>(continue_loop_label);
  auto end_bb = std::make_shared<IR::IRBasicBlock>(end_label);
  context.SetIRValue(loop_label, loop_bb);
  context.SetIRValue(continue_loop_label, continue_loop_bb);
  context.SetIRValue(end_label, end_bb);
  // j或br语句配block
  context.enterNewWhileScope(loop_label, continue_loop_label, end_label);
  if (context.current_block) {
    auto jump_inst = std::make_shared<IR::IRJump>(loop_bb);
    context.current_block->AddInstruction(jump_inst);
  }
  context.AddExistingBlock(loop_bb);

  std::string cond_reg = Generate(*node.while_exp);
  if (context.current_block) {
    auto cond_val = context.GetIRValue(cond_reg);
    auto br_inst = std::make_shared<IR::IRBranch>(cond_val, continue_loop_bb, end_bb);
    context.current_block->AddInstruction(br_inst);
  }
  context.AddExistingBlock(continue_loop_bb);

  Generate(*node.while_stmt);
  if (!context.cur_block_has_terminator) {
    if (context.current_block) {
      auto jump_inst = std::make_shared<IR::IRJump>(loop_bb);
      context.current_block->AddInstruction(jump_inst);
    }
  }
  context.AddExistingBlock(end_bb);

  context.cur_block_has_terminator = false;
  context.exitWhileScope();
  return "";
}

std::string CodeGenerator::Generate(const BreakStmtAST& node) {
  if (context.cur_block_has_terminator) return "";
  WhileLabelContext while_label_context = context.GetWhileLabel();

  auto end_bb = context.GetIRValue(while_label_context.while_label_end);
  if (context.current_block && end_bb) {
    auto jump_inst = std::make_shared<IR::IRJump>(end_bb);
    context.current_block->AddInstruction(jump_inst);
  }

  std::string tool_label = while_label_context.while_label_continue + NumBuilder::GetNum();
  context.AddNewBlock(tool_label);
  return "";
}

std::string CodeGenerator::Generate(const ContinueStmtAST& node) {
  if (context.cur_block_has_terminator) return "";
  WhileLabelContext while_label_context = context.GetWhileLabel();

  auto begin_bb = context.GetIRValue(while_label_context.while_label_begin);
  if (context.current_block && begin_bb) {
    auto jump_inst = std::make_shared<IR::IRJump>(begin_bb);
    context.current_block->AddInstruction(jump_inst);
  }

  std::string tool_label = while_label_context.while_label_continue + NumBuilder::GetNum();
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

  context.arg_stack.push_back({}); // Push new argument list to stack

  std::string param;
  if (node.func_r_params) {
    param = Generate(*node.func_r_params);
  } else {
    param = "";
  }

  auto args = std::move(context.arg_stack.back());
  context.arg_stack.pop_back(); // Pop it

  auto target_func = context.GetIRValue("@" + func_name);
  if (!target_func) {
    target_func = std::make_shared<IR::IRSymbol>("@" + func_name);
  }

  if (func_inf.ret_type != "void") {
    std::string func_reg = context.NewReg();

    if (context.current_block) {
      auto call_inst = std::make_shared<IR::IRCall>(func_reg, target_func, args, IR::IRType::IR_INT32);
      context.current_block->AddInstruction(call_inst);
      context.SetIRValue(func_reg, call_inst);
    }
    return func_reg;
  } else {

    if (context.current_block) {
      auto call_inst = std::make_shared<IR::IRCall>("", target_func, args, IR::IRType::IR_VOID);
      context.current_block->AddInstruction(call_inst);
    }
    return "";
  }
}

std::string CodeGenerator::Generate(const FuncRParamsAST& node) {
  std::string params;
  if (node.func_r_params.size() > 0) {
    std::string arg0 = Generate(*node.func_r_params[0]);
    if (!context.arg_stack.empty()) {
      context.arg_stack.back().push_back(context.GetIRValue(arg0));
    }
    params += arg0;

    for (size_t i = 1; i < node.func_r_params.size(); i++) {
      std::string argi = Generate(*node.func_r_params[i]);
      if (!context.arg_stack.empty()) {
        context.arg_stack.back().push_back(context.GetIRValue(argi));
      }
      params = params + ", " + argi;
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
    op = IR::IRBinaryOp::MUL;
  } else if (node.op->op == "/") {
    op = IR::IRBinaryOp::DIV;
  } else if (node.op->op == "%") {
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
    op = IR::IRBinaryOp::ADD;
  } else if (node.op->op == "-") {
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
    op = IR::IRBinaryOp::LT;
  } else if (node.op->op == ">") {
    op = IR::IRBinaryOp::GT;
  } else if (node.op->op == "<=") {
    op = IR::IRBinaryOp::LE;
  } else if (node.op->op == ">=") {
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
    op = IR::IRBinaryOp::EQ;
  } else if (node.op->op == "!=") {
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

    std::string then_label = LabelBuilder::GetLabel("and_calc_rhs");
    std::string end_label = LabelBuilder::GetLabel("and_end");

    auto then_bb = std::make_shared<IR::IRBasicBlock>(then_label);
    auto end_bb = std::make_shared<IR::IRBasicBlock>(end_label);
    context.SetIRValue(then_label, then_bb);
    context.SetIRValue(end_label, end_bb);


    if (context.current_block) {
      auto alloc_inst = std::make_shared<IR::IRAlloc>(temp_address, "i32");
      context.current_block->AddInstruction(alloc_inst);
      context.SetIRValue(temp_address, alloc_inst);

      auto const_zero = context.GetIRValue("0");
      auto store_zero = std::make_shared<IR::IRStore>(const_zero, alloc_inst);
      context.current_block->AddInstruction(store_zero);
    }

    if (context.current_block) {
      auto lhs_val = context.GetIRValue(lhs_reg);
      auto br_inst = std::make_shared<IR::IRBranch>(lhs_val, then_bb, end_bb);
      context.current_block->AddInstruction(br_inst);
    }

    // then块
    context.AddExistingBlock(then_bb);
    context.cur_block_has_terminator = false;

    std::string rhs_reg = Generate(*node.eq_exp);
    std::string temp_reg = context.NewReg();

    if (context.current_block) {
      auto rhs_val = context.GetIRValue(rhs_reg);
      auto const_zero = context.GetIRValue("0");
      auto ne_inst = std::make_shared<IR::IRBinary>(temp_reg, IR::IRBinaryOp::NE, rhs_val, const_zero);
      context.current_block->AddInstruction(ne_inst);
      context.SetIRValue(temp_reg, ne_inst);

      auto alloc_inst = context.GetIRValue(temp_address);
      auto store_inst = std::make_shared<IR::IRStore>(ne_inst, alloc_inst);
      context.current_block->AddInstruction(store_inst);
    }

    if (context.current_block) {
      auto jump_inst = std::make_shared<IR::IRJump>(end_bb);
      context.current_block->AddInstruction(jump_inst);
    }

    // end块
    context.AddExistingBlock(end_bb);
    context.cur_block_has_terminator = false;

    std::string return_reg = context.NewReg();

    if (context.current_block) {
      auto alloc_inst = context.GetIRValue(temp_address);
      auto load_inst = std::make_shared<IR::IRLoad>(return_reg, alloc_inst);
      context.current_block->AddInstruction(load_inst);
      context.SetIRValue(return_reg, load_inst);
    }

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
    std::string then_label = LabelBuilder::GetLabel("or_calc_rhs", temp_address);
    std::string end_label = LabelBuilder::GetLabel("or_end", temp_address);

    auto then_bb = std::make_shared<IR::IRBasicBlock>(then_label);
    auto end_bb = std::make_shared<IR::IRBasicBlock>(end_label);
    context.SetIRValue(then_label, then_bb);
    context.SetIRValue(end_label, end_bb);


    if (context.current_block) {
      auto alloc_inst = std::make_shared<IR::IRAlloc>(temp_address, "i32");
      context.current_block->AddInstruction(alloc_inst);
      context.SetIRValue(temp_address, alloc_inst);

      auto const_one = context.GetIRValue("1");
      auto store_one = std::make_shared<IR::IRStore>(const_one, alloc_inst);
      context.current_block->AddInstruction(store_one);
    }

    if (context.current_block) {
      auto lhs_val = context.GetIRValue(lhs_reg);
      auto br_inst = std::make_shared<IR::IRBranch>(lhs_val, end_bb, then_bb);
      context.current_block->AddInstruction(br_inst);
    }

    // then块
    context.AddExistingBlock(then_bb);
    context.cur_block_has_terminator = false;

    std::string rhs_reg = Generate(*node.land_exp);
    std::string temp_reg = context.NewReg();

    if (context.current_block) {
      auto rhs_val = context.GetIRValue(rhs_reg);
      auto const_zero = context.GetIRValue("0");
      auto ne_inst = std::make_shared<IR::IRBinary>(temp_reg, IR::IRBinaryOp::NE, const_zero, rhs_val);
      context.current_block->AddInstruction(ne_inst);
      context.SetIRValue(temp_reg, ne_inst);

      auto alloc_inst = context.GetIRValue(temp_address);
      auto store_inst = std::make_shared<IR::IRStore>(ne_inst, alloc_inst);
      context.current_block->AddInstruction(store_inst);
    }

    if (context.current_block) {
      auto jump_inst = std::make_shared<IR::IRJump>(end_bb);
      context.current_block->AddInstruction(jump_inst);
    }

    // end块
    context.AddExistingBlock(end_bb);
    context.cur_block_has_terminator = false;

    std::string return_reg = context.NewReg();

    if (context.current_block) {
      auto alloc_inst = context.GetIRValue(temp_address);
      auto load_inst = std::make_shared<IR::IRLoad>(return_reg, alloc_inst);
      context.current_block->AddInstruction(load_inst);
      context.SetIRValue(return_reg, load_inst);
    }

    return return_reg;
  }
  return "";
}

std::string CodeGenerator::Generate(const LValAST& node) {
  auto sym = context.scope->lookup(*node.ident);
  if (sym == nullptr) {
    return "";
  }
  if (sym->kind == SymbolKind::VARIABLE) {
    std::string ptr = std::get<std::string>(sym->value);
    std::string new_reg = context.NewReg();

    if (context.current_block) {
      auto ptr_val = context.GetIRValue(ptr);
      auto load_inst = std::make_shared<IR::IRLoad>(new_reg, ptr_val);
      context.current_block->AddInstruction(load_inst);
      context.SetIRValue(new_reg, load_inst);
    }
    return new_reg;
  } else if (sym->kind == SymbolKind::CONSTANT) {
    std::string int_value = std::to_string(std::get<int>(sym->value));
    context.GetIRValue(int_value);
    return int_value;
  } else if (sym->kind == SymbolKind::ARRAY || sym->kind == SymbolKind::CONST_ARRAY) {
    auto arr_info = std::get<ArrayInformation>(sym->value);
    
    std::string prev_reg = arr_info.address;
    std::shared_ptr<IR::IRValue> prev_val = context.GetIRValue(arr_info.address);

    if (arr_info.is_param) {
      std::string loaded_ptr = context.NewReg();
      if (context.current_block) {
        auto load_ptr_inst = std::make_shared<IR::IRLoad>(loaded_ptr, prev_val);
        context.current_block->AddInstruction(load_ptr_inst);
        context.SetIRValue(loaded_ptr, load_ptr_inst);
        prev_val = load_ptr_inst;
      }
      prev_reg = loaded_ptr;
    }

    for (size_t i = 0; i < node.index_exp.size(); i++) {
      auto index_reg = Generate(*node.index_exp[i]);
      std::string addr_reg = context.NewReg();
      if (arr_info.is_param && i == 0) {
        if (context.current_block) {
          auto index_val = context.GetIRValue(index_reg);
          int elem_size = 4;
          for (size_t j = i; j < arr_info.dims.size(); j++) {
            elem_size *= arr_info.dims[j];
          }
          auto getptr_inst = std::make_shared<IR::IRGetPtr>(addr_reg, prev_val, index_val, elem_size);
          context.current_block->AddInstruction(getptr_inst);
          context.SetIRValue(addr_reg, getptr_inst);
          prev_val = getptr_inst;
        }
      } else {
        if (context.current_block) {
          auto index_val = context.GetIRValue(index_reg);
          int elem_size = 4;
          size_t start_idx = arr_info.is_param ? i : i + 1;
          for (size_t j = start_idx; j < arr_info.dims.size(); j++) {
            elem_size *= arr_info.dims[j];
          }
          auto getelemptr_inst = std::make_shared<IR::IRGetElemPtr>(addr_reg, prev_val, index_val, elem_size);
          context.current_block->AddInstruction(getelemptr_inst);
          context.SetIRValue(addr_reg, getelemptr_inst);
          prev_val = getelemptr_inst;
        }
      }
      prev_reg = addr_reg;
    }

    bool need_decay = false;
    if (!arr_info.is_param) {
      need_decay = node.index_exp.size() < arr_info.dims.size();
    } else {
      need_decay = node.index_exp.size() > 0 && (node.index_exp.size() - 1) < arr_info.dims.size();
    }
    if (need_decay) {
      std::string addr_reg = context.NewReg();
      if (context.current_block) {
        auto index_val = context.GetIRValue("0");
        size_t i = node.index_exp.size();
        int elem_size = 4;
        size_t start_idx = arr_info.is_param ? i : i + 1;
        for (size_t j = start_idx; j < arr_info.dims.size(); j++) {
          elem_size *= arr_info.dims[j];
        }
        auto getelemptr_inst = std::make_shared<IR::IRGetElemPtr>(addr_reg, prev_val, index_val, elem_size);
        context.current_block->AddInstruction(getelemptr_inst);
        context.SetIRValue(addr_reg, getelemptr_inst);
        prev_val = getelemptr_inst;
      }
      return addr_reg;
    } else {
      int total_dims = arr_info.dims.size() + (arr_info.is_param ? 1 : 0);
      if (node.index_exp.size() == (size_t)total_dims) {
        std::string final_reg = context.NewReg();
        if (context.current_block) {
          auto load_final_inst = std::make_shared<IR::IRLoad>(final_reg, prev_val);
          context.current_block->AddInstruction(load_final_inst);
          context.SetIRValue(final_reg, load_final_inst);
        }
        return final_reg;
      } else {
        return prev_reg;
      }
    }
  } else {
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

static void FillConstInitVal(const BaseAST& node, 
                             const std::vector<int>& dims, 
                             const std::vector<int>& sub_sizes, 
                             int parent_level, 
                             int& index, 
                             std::vector<int32_t>& flat_vals, 
                             Context& context) {
  if (const auto* p = dynamic_cast<const ConstExpConstInitValAST*>(&node)) {
    int val = p->const_exp->getValue(context);
    if (index < (int)flat_vals.size()) {
      flat_vals[index] = val;
    }
    index++;
  } else if (const auto* p = dynamic_cast<const CompoundConstInitValueAST*>(&node)) {
    if (index % dims.back() != 0) {
      return;
    }
    int target_level = -1;
    for (int k = parent_level + 1; k < (int)dims.size(); k++) {
      if (index % sub_sizes[k] == 0) {
        target_level = k;
        break;
      }
    }
    if (target_level == -1) return;

    int start_index = index;
    int sub_size = sub_sizes[target_level];

    if (p->const_init_val_list) {
      for (const auto& child : p->const_init_val_list->const_init_vals) {
        FillConstInitVal(*child, dims, sub_sizes, target_level, index, flat_vals, context);
      }
    }
    if (index < start_index + sub_size) {
      index = start_index + sub_size;
    }
  }
}

static void FillConstInitValFromInitVal(const BaseAST& node, 
                                        const std::vector<int>& dims, 
                                        const std::vector<int>& sub_sizes, 
                                        int parent_level, 
                                        int& index, 
                                        std::vector<int32_t>& flat_vals, 
                                        Context& context) {
  if (const auto* p = dynamic_cast<const InitValAST*>(&node)) {
    int val = p->exp->getValue(context);
    if (index < (int)flat_vals.size()) {
      flat_vals[index] = val;
    }
    index++;
  } else if (const auto* p = dynamic_cast<const InitValListAST*>(&node)) {
    if (index % dims.back() != 0) {
      return;
    }
    int target_level = -1;
    for (int k = parent_level + 1; k < (int)dims.size(); k++) {
      if (index % sub_sizes[k] == 0) {
        target_level = k;
        break;
      }
    }
    if (target_level == -1) return;

    int start_index = index;
    int sub_size = sub_sizes[target_level];

    for (const auto& child : p->init_vals) {
      FillConstInitValFromInitVal(*child, dims, sub_sizes, target_level, index, flat_vals, context);
    }
    if (index < start_index + sub_size) {
      index = start_index + sub_size;
    }
  }
}

static void FillInitValRegs(const BaseAST& node, 
                            const std::vector<int>& dims, 
                            const std::vector<int>& sub_sizes, 
                            int parent_level, 
                            int& index, 
                            std::vector<std::string>& flat_vals, 
                            CodeGenerator& gen, 
                            Context& context) {
  if (const auto* p = dynamic_cast<const InitValAST*>(&node)) {
    std::string val = gen.Generate(*p->exp);
    if (index < (int)flat_vals.size()) {
      flat_vals[index] = val;
    }
    index++;
  } else if (const auto* p = dynamic_cast<const InitValListAST*>(&node)) {
    if (index % dims.back() != 0) {
      return;
    }
    int target_level = -1;
    for (int k = parent_level + 1; k < (int)dims.size(); k++) {
      if (index % sub_sizes[k] == 0) {
        target_level = k;
        break;
      }
    }
    if (target_level == -1) return;

    int start_index = index;
    int sub_size = sub_sizes[target_level];

    for (const auto& child : p->init_vals) {
      FillInitValRegs(*child, dims, sub_sizes, target_level, index, flat_vals, gen, context);
    }
    if (index < start_index + sub_size) {
      index = start_index + sub_size;
    }
  }
}

std::string CodeGenerator::Generate(const ArrayConstDefAST& node) {
  std::vector<int> dims;
  int arr_size = 1;
  for (auto& expr : node.const_exps) {
    int val = expr->getValue(context);
    dims.push_back(val);
    arr_size *= val;
  }

  std::vector<int> sub_sizes(dims.size());
  if (!dims.empty()) {
    sub_sizes.back() = dims.back();
    for (int k = (int)dims.size() - 2; k >= 0; --k) {
      sub_sizes[k] = dims[k] * sub_sizes[k+1];
    }
  }

  std::vector<int32_t> init_vals_int(arr_size, 0);
  int fill_index = 0;
  FillConstInitVal(*node.const_init_val, dims, sub_sizes, -1, fill_index, init_vals_int, context);

  auto address = context.GetAddress(*node.ident);
  ArrayInformation arr_info(address, dims, init_vals_int);
  auto sym = std::make_shared<Symbol>(*node.ident, SymbolKind::CONST_ARRAY);
  sym->value = arr_info;

  std::string arr_type = "i32";
  for (auto it = dims.rbegin(); it != dims.rend(); ++it) {
    arr_type = "[" + arr_type + ", " + std::to_string(*it) + "]";
  }

  if (context.scope->isGlobal()) {
    auto global_alloc = std::make_shared<IR::IRGlobalAlloc>(address, arr_type, false, init_vals_int, dims);
    context.AddGlobal(global_alloc);
  } else {
    std::shared_ptr<IR::IRAlloc> alloc_inst = nullptr;
    if (context.current_block) {
      alloc_inst = std::make_shared<IR::IRAlloc>(address, "i32", dims);
      context.current_block->AddInstruction(alloc_inst);
      context.SetIRValue(address, alloc_inst);
    }
    for (int i = 0; i < arr_size; i++) {
      // 展开扁平索引 i 到多维索引
      std::vector<int> idxs(dims.size());
      int temp_idx = i;
      for (int j = (int)dims.size() - 1; j >= 0; --j) {
        idxs[j] = temp_idx % dims[j];
        temp_idx /= dims[j];
      }

      std::string prev_reg = address;
      std::shared_ptr<IR::IRValue> prev_val = alloc_inst;

      for (size_t j = 0; j < idxs.size(); j++) {
        std::string next_reg = context.NewReg();
        if (context.current_block) {
          auto idx_val = context.GetIRValue(std::to_string(idxs[j]));
          int elem_size = 4;
          for (size_t k = j + 1; k < dims.size(); k++) {
            elem_size *= dims[k];
          }
          auto getelemptr_inst = std::make_shared<IR::IRGetElemPtr>(next_reg, prev_val, idx_val, elem_size);
          context.current_block->AddInstruction(getelemptr_inst);
          context.SetIRValue(next_reg, getelemptr_inst);
          prev_val = getelemptr_inst;
        }
        prev_reg = next_reg;
      }

      if (context.current_block) {
        auto val = context.GetIRValue(std::to_string(init_vals_int[i]));
        auto store_inst = std::make_shared<IR::IRStore>(val, prev_val);
        context.current_block->AddInstruction(store_inst);
      }
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
    auto global_alloc = std::make_shared<IR::IRGlobalAlloc>(address, b_type, true, std::vector<int32_t>{});
    context.AddGlobal(global_alloc);
  } else {
    if (context.current_block) {
      auto alloc_inst = std::make_shared<IR::IRAlloc>(address, b_type);
      context.current_block->AddInstruction(alloc_inst);
      context.SetIRValue(address, alloc_inst);
    }
  }
  context.scope->define(sym);
  return "";
}

std::string CodeGenerator::Generate(const InitializedVarDefAST& node) {
  std::string address = context.GetAddress(*node.ident);
  std::string b_type = *context.decl_type_context;
  auto sym = std::make_shared<Symbol>(*node.ident, SymbolKind::VARIABLE, address);
  if (context.scope->isGlobal()) {
    int init_val = node.init_val->getValue(context);
    auto global_alloc = std::make_shared<IR::IRGlobalAlloc>(address, b_type, false, std::vector<int32_t>{init_val});
    context.AddGlobal(global_alloc);
  } else {
    auto alloc_inst = std::make_shared<IR::IRAlloc>(address, b_type);
    context.current_block->AddInstruction(alloc_inst);
    context.SetIRValue(address, alloc_inst);

    std::string init_val_reg = Generate(*node.init_val);
    auto store_inst = std::make_shared<IR::IRStore>(context.GetIRValue(init_val_reg), context.GetIRValue(address));
    context.current_block->AddInstruction(store_inst);
  }
  context.scope->define(sym);
  return "";
}

std::string CodeGenerator::Generate(const ArrayVarDefAST& node) {
  std::string address = context.GetAddress(*node.ident);
  std::string b_type = *context.decl_type_context;
  
  std::vector<int> dims;
  for (auto& expr : node.const_exps) {
    int val = expr->getValue(context);
    dims.push_back(val);
  }

  auto sym = std::make_shared<Symbol>(*node.ident, SymbolKind::ARRAY);
  sym->value = ArrayInformation(address, dims);

  std::string arr_type = b_type;
  for (auto it = dims.rbegin(); it != dims.rend(); ++it) {
    arr_type = "[" + arr_type + ", " + std::to_string(*it) + "]";
  }

  if (context.scope->isGlobal()) {
    auto global_alloc = std::make_shared<IR::IRGlobalAlloc>(address, arr_type, true, std::vector<int32_t>{}, dims);
    context.AddGlobal(global_alloc);
  } else {
    if (context.current_block) {
      auto alloc_inst = std::make_shared<IR::IRAlloc>(address, b_type, dims);
      context.current_block->AddInstruction(alloc_inst);
      context.SetIRValue(address, alloc_inst);
    }
  }
  context.scope->define(sym);
  return "";
}

std::string CodeGenerator::Generate(const ArrayInitializedVarDefAST& node) {
  std::string address = context.GetAddress(*node.ident);
  std::string b_type = *context.decl_type_context;
  
  std::vector<int> dims;
  int arr_size = 1;
  for (auto& expr : node.const_exps) {
    int val = expr->getValue(context);
    dims.push_back(val);
    arr_size *= val;
  }

  auto sym = std::make_shared<Symbol>(*node.ident, SymbolKind::ARRAY);
  sym->value = ArrayInformation(address, dims);

  std::string arr_type = "i32";
  for (auto it = dims.rbegin(); it != dims.rend(); ++it) {
    arr_type = "[" + arr_type + ", " + std::to_string(*it) + "]";
  }

  std::vector<int> sub_sizes(dims.size());
  if (!dims.empty()) {
    sub_sizes.back() = dims.back();
    for (int k = (int)dims.size() - 2; k >= 0; --k) {
      sub_sizes[k] = dims[k] * sub_sizes[k+1];
    }
  }

  if (context.scope->isGlobal()) {
    std::vector<int32_t> init_nums(arr_size, 0);
    int fill_index = 0;
    FillConstInitValFromInitVal(*node.init_val_list, dims, sub_sizes, -1, fill_index, init_nums, context);

    auto global_alloc = std::make_shared<IR::IRGlobalAlloc>(address, arr_type, false, init_nums, dims);
    context.AddGlobal(global_alloc);
  } else {
    std::vector<std::string> init_vals(arr_size, "0");
    int fill_index = 0;
    FillInitValRegs(*node.init_val_list, dims, sub_sizes, -1, fill_index, init_vals, *this, context);

    std::shared_ptr<IR::IRAlloc> alloc_inst = nullptr;
    if (context.current_block) {
      alloc_inst = std::make_shared<IR::IRAlloc>(address, "i32", dims);
      context.current_block->AddInstruction(alloc_inst);
      context.SetIRValue(address, alloc_inst);
    }
    for (int i = 0; i < arr_size; i++) {
      std::vector<int> idxs(dims.size());
      int temp_idx = i;
      for (int j = (int)dims.size() - 1; j >= 0; --j) {
        idxs[j] = temp_idx % dims[j];
        temp_idx /= dims[j];
      }

      std::string prev_reg = address;
      std::shared_ptr<IR::IRValue> prev_val = alloc_inst;

      for (size_t j = 0; j < idxs.size(); j++) {
        std::string next_reg = context.NewReg();
        if (context.current_block) {
          auto idx_val = context.GetIRValue(std::to_string(idxs[j]));
          int elem_size = 4;
          for (size_t k = j + 1; k < dims.size(); k++) {
            elem_size *= dims[k];
          }
          auto getelemptr_inst = std::make_shared<IR::IRGetElemPtr>(next_reg, prev_val, idx_val, elem_size);
          context.current_block->AddInstruction(getelemptr_inst);
          context.SetIRValue(next_reg, getelemptr_inst);
          prev_val = getelemptr_inst;
        }
        prev_reg = next_reg;
      }

      if (context.current_block) {
        auto val = context.GetIRValue(init_vals[i]);
        auto store_inst = std::make_shared<IR::IRStore>(val, prev_val);
        context.current_block->AddInstruction(store_inst);
      }
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


