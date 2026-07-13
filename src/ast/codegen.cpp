#include "ast.hpp"
#include "ast_visitor.hpp"
#include <iostream>
#include <cassert>
#include <memory>

class CodeGenerator final : public ASTVisitor {
public:
  explicit CodeGenerator(Context& context);

  std::string Generate(const BaseAST& node);
  std::string Visit(const ArrayDimensionsAST& node) override;
  std::string Visit(const CompUnitAST& node) override;
  std::string Visit(const CompUnitSequenceAST& node) override;
  std::string Visit(const CompUnitListAST& node) override;
  std::string Visit(const FunctionDefinitionUnitAST& node) override;
  std::string Visit(const DeclarationUnitAST& node) override;
  std::string Visit(const FuncDefAST& node) override;
  std::string Visit(const FuncFParamsAST& node) override;
  std::string Visit(const FuncFParamAST& node) override;
  std::string Visit(const BlockAST& node) override;
  std::string Visit(const DeclBlockAST& node) override;
  std::string Visit(const StmtBlockAST& node) override;
  std::string Visit(const UnmatchedStmtWrapperAST& node) override;
  std::string Visit(const MatchedStmtWrapperAST& node) override;
  std::string Visit(const IfUnmatchedStmtAST& node) override;
  std::string Visit(const IfElseUnmatchedStmtAST& node) override;
  std::string Visit(const ReturnStmtAST& node) override;
  std::string Visit(const AssignStmtAST& node) override;
  std::string Visit(const ExpStmtAST& node) override;
  std::string Visit(const EmptyStmtAST& node) override;
  std::string Visit(const BlockStmtAST& node) override;
  std::string Visit(const IfMatchedStmtAST& node) override;
  std::string Visit(const WhileStmtAST& node) override;
  std::string Visit(const ContinueStmtAST& node) override;
  std::string Visit(const BreakStmtAST& node) override;
  std::string Visit(const ExpAST& node) override;
  std::string Visit(const UnaryOPExpAST& node) override;
  std::string Visit(const PrimaryUnaryExpAST& node) override;
  std::string Visit(const FuncCallExpAST& node) override;
  std::string Visit(const FuncRParamsAST& node) override;
  std::string Visit(const ParenthesizedExprAST& node) override;
  std::string Visit(const LValuePrimaryExprAST& node) override;
  std::string Visit(const NumberPrimaryExprAST& node) override;
  std::string Visit(const OpAST& node) override;
  std::string Visit(const UnaryMulExpAST& node) override;
  std::string Visit(const BinaryMulExprAST& node) override;
  std::string Visit(const MulAddExpAST& node) override;
  std::string Visit(const BinaryAddExprAST& node) override;
  std::string Visit(const AddRelExpAST& node) override;
  std::string Visit(const BinaryRelExprAST& node) override;
  std::string Visit(const RelEqExpAST& node) override;
  std::string Visit(const BinaryEqExprAST& node) override;
  std::string Visit(const EqLAndExpAST& node) override;
  std::string Visit(const BinaryLogicalAndExprAST& node) override;
  std::string Visit(const LAndLOrExpAST& node) override;
  std::string Visit(const BinaryLogicalOrExprAST& node) override;
  std::string Visit(const LValAST& node) override;
  std::string Visit(const ConstExpAST& node) override;
  std::string Visit(const ConstDefAST& node) override;
  std::string Visit(const ArrayConstDefAST& node) override;
  std::string Visit(const ConstExpConstInitValAST& node) override;
  std::string Visit(const ConstInitValListAST& node) override;
  std::string Visit(const CompoundConstInitValueAST& node) override;
  std::string Visit(const ConstDeclWrapperAST& node) override;
  std::string Visit(const VarDeclWrapperAST& node) override;
  std::string Visit(const ConstDeclAST& node) override;
  std::string Visit(const ConstDefListAST& node) override;
  std::string Visit(const VarDeclAST& node) override;
  std::string Visit(const VarDefListAST& node) override;
  std::string Visit(const SimpleVarDefAST& node) override;
  std::string Visit(const InitializedVarDefAST& node) override;
  std::string Visit(const ArrayVarDefAST& node) override;
  std::string Visit(const ArrayInitializedVarDefAST& node) override;
  std::string Visit(const InitValAST& node) override;
  std::string Visit(const InitValListAST& node) override;

  void ParamLocalization(const FuncFParamsAST& node);
  void ParamLocalization(const FuncFParamAST& node);

private:
  Context& context;
};

CodeGenerator::CodeGenerator(Context& context) : context(context) {}

std::string CodeGenerator::Generate(const BaseAST& node) {
  return node.Accept(*this);
}

std::string CodeGenerator::Visit(const ArrayDimensionsAST&) {
  // This grammar helper previously fell through Generate(BaseAST) and emitted no IR.
  return "";
}

std::string CodeGenerator::Visit(const CompUnitAST& node) {
  if (node.end_comp_unit) {
    context.AddDecl(std::make_shared<IR::IRFuncDecl>("getint", IR::IRType::IR_INT32, std::vector<IR::IRType>{}));
    context.AddDecl(std::make_shared<IR::IRFuncDecl>("getch", IR::IRType::IR_INT32, std::vector<IR::IRType>{}));
    context.AddDecl(std::make_shared<IR::IRFuncDecl>("getarray", IR::IRType::IR_INT32, std::vector<IR::IRType>{IR::IRType::IR_POINTER}));
    context.AddDecl(std::make_shared<IR::IRFuncDecl>("putint", IR::IRType::IR_VOID, std::vector<IR::IRType>{IR::IRType::IR_INT32}));
    context.AddDecl(std::make_shared<IR::IRFuncDecl>("putch", IR::IRType::IR_VOID, std::vector<IR::IRType>{IR::IRType::IR_INT32}));
    context.AddDecl(std::make_shared<IR::IRFuncDecl>("putarray", IR::IRType::IR_VOID, std::vector<IR::IRType>{IR::IRType::IR_INT32, IR::IRType::IR_POINTER}));
    context.AddDecl(std::make_shared<IR::IRFuncDecl>("starttime", IR::IRType::IR_VOID, std::vector<IR::IRType>{}));
    context.AddDecl(std::make_shared<IR::IRFuncDecl>("stoptime", IR::IRType::IR_VOID, std::vector<IR::IRType>{}));
    context.AddDecl(std::make_shared<IR::IRFuncDecl>("getfloat", IR::IRType::IR_FLOAT, std::vector<IR::IRType>{}));
    context.AddDecl(std::make_shared<IR::IRFuncDecl>("getfarray", IR::IRType::IR_INT32, std::vector<IR::IRType>{IR::IRType::IR_POINTER}));
    context.AddDecl(std::make_shared<IR::IRFuncDecl>("putfloat", IR::IRType::IR_VOID, std::vector<IR::IRType>{IR::IRType::IR_FLOAT}));
    context.AddDecl(std::make_shared<IR::IRFuncDecl>("putfarray", IR::IRType::IR_VOID, std::vector<IR::IRType>{IR::IRType::IR_INT32, IR::IRType::IR_POINTER}));

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
    symbol = std::make_shared<Symbol>("getfloat", SymbolKind::FUNCTION, FuncInformation("float"));
    context.scope->define(symbol);
    symbol = std::make_shared<Symbol>("getfarray", SymbolKind::FUNCTION, FuncInformation("i32"));
    context.scope->define(symbol);
    symbol = std::make_shared<Symbol>("putfloat", SymbolKind::FUNCTION, FuncInformation("void"));
    context.scope->define(symbol);
    symbol = std::make_shared<Symbol>("putfarray", SymbolKind::FUNCTION, FuncInformation("void"));
    context.scope->define(symbol);
    Generate(*node.end_comp_unit);
  }
  return "";
}

std::string CodeGenerator::Visit(const CompUnitSequenceAST& node) {
  if (node.comp_unit) {
    Generate(*node.comp_unit);
  }
  return "";
}

std::string CodeGenerator::Visit(const CompUnitListAST& node) {
  if (node.end_comp_unit) {
    Generate(*node.end_comp_unit);
  }
  if (node.comp_unit) {
    Generate(*node.comp_unit);
  }
  return "";
}

std::string CodeGenerator::Visit(const FunctionDefinitionUnitAST& node) {
  if (node.func_def) {
    Generate(*node.func_def);
  }
  return "";
}

std::string CodeGenerator::Visit(const DeclarationUnitAST& node) {
  if (node.decl) {
    Generate(*node.decl);
  }
  return "";
}

std::string CodeGenerator::Visit(const FuncDefAST& node) {
  context.ResetRegBuilder();
  auto symbol = std::make_shared<Symbol>(*node.ident, SymbolKind::FUNCTION, FuncInformation(*node.type));
  context.scope->define(symbol);

  // 初始化内存表示的 IRFunction
  IR::IRType ret_type = (*node.type == "void") ? IR::IRType::IR_VOID : ((*node.type == "f32") ? IR::IRType::IR_FLOAT : IR::IRType::IR_INT32);
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

std::string CodeGenerator::Visit(const FuncFParamsAST& node) {
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
    dims.push_back(expr->getValue(context).getInt());
  }
  std::string arr_type = base_type;
  for (auto it = dims.rbegin(); it != dims.rend(); ++it) {
    arr_type = "[" + arr_type + ", " + std::to_string(*it) + "]";
  }
  return "*" + arr_type;
}

std::string CodeGenerator::Visit(const FuncFParamAST& node) {
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
      dims.push_back(expr->getValue(context).getInt());
    }
    auto symbol = std::make_shared<Symbol>(*node.ident, SymbolKind::ARRAY);
    symbol->value = ArrayInformation(address, dims, {}, {}, true); // is_param = true
    context.scope->define(symbol);
  } else {
    auto symbol = std::make_shared<Symbol>(*node.ident, SymbolKind::VARIABLE, address);
    context.scope->define(symbol);
  }


  if (context.current_block) {
    std::string param_name = "@" + *node.ident;
    IR::IRType param_type = node.is_array ? IR::IRType::IR_POINTER : ((*node.type == "f32") ? IR::IRType::IR_FLOAT : IR::IRType::IR_INT32);
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

std::string CodeGenerator::Visit(const BlockAST& node) {
  context.enterNewScope();
  for (auto& item : node.block_items) {
    Generate(*item);
  }
  context.exitScope();
  return "";
}

std::string CodeGenerator::Visit(const DeclBlockAST& node) {
  if (node.decl) {
    Generate(*node.decl);
  }
  return "";
}

std::string CodeGenerator::Visit(const StmtBlockAST& node) {
  if (node.stmt) {
    Generate(*node.stmt);
  }
  return "";
}

std::string CodeGenerator::Visit(const MatchedStmtWrapperAST& node) {
  if (node.matched_stmt) {
    return Generate(*node.matched_stmt);
  }
  return "";
}

std::string CodeGenerator::Visit(const UnmatchedStmtWrapperAST& node) {
  if (node.unmatched_stmt) {
    Generate(*node.unmatched_stmt);
  }
  return "";
}

std::string CodeGenerator::Visit(const IfUnmatchedStmtAST& node) {
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
    context.current_block->SetNext(then_bb, end_bb);
  }

  context.AddExistingBlock(then_bb);

  context.cur_block_has_terminator = false;
  Generate(*node.if_stmt);
  if (!context.cur_block_has_terminator) {
    if (context.current_block) {
      auto jump_inst = std::make_shared<IR::IRJump>(end_bb);
      context.current_block->AddInstruction(jump_inst);
      context.current_block->SetNext(end_bb);
    }
  }
  context.AddExistingBlock(end_bb);

  context.cur_block_has_terminator = false;
  return "";
}

std::string CodeGenerator::Visit(const IfElseUnmatchedStmtAST& node) {
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
    context.current_block->SetNext(then_bb, else_bb);
  }

  context.AddExistingBlock(then_bb);

  context.cur_block_has_terminator = false;
  Generate(*node.if_stmt);
  bool then_has_terminator = context.cur_block_has_terminator;
  if (!then_has_terminator) {
    if (context.current_block) {
      auto jump_inst = std::make_shared<IR::IRJump>(end_bb);
      context.current_block->AddInstruction(jump_inst);
      context.current_block->SetNext(end_bb);
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
      context.current_block->SetNext(end_bb);
    }
  }
  if (!then_has_terminator || !else_has_terminator) {
    context.AddExistingBlock(end_bb);
    context.cur_block_has_terminator = false;
  } else {
    context.cur_block_has_terminator = true;
  }
  return "";
}

std::string CodeGenerator::Visit(const ReturnStmtAST& node) {
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

std::string CodeGenerator::Visit(const AssignStmtAST& node) {
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
          auto load_ptr_inst = std::make_shared<IR::IRLoad>(loaded_ptr, prev_val, IR::IRType::IR_POINTER);
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

std::string CodeGenerator::Visit(const ExpStmtAST& node) {
  if (context.cur_block_has_terminator) return "";
  if (node.exp) {
    Generate(*node.exp);
  }
  return "";
}

std::string CodeGenerator::Visit(const EmptyStmtAST& node) {
  return "";
}

std::string CodeGenerator::Visit(const BlockStmtAST& node) {
  if (node.block) {
    Generate(*node.block);
  }
  return "";
}

std::string CodeGenerator::Visit(const IfMatchedStmtAST& node) {
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
    context.current_block->SetNext(then_bb, else_bb);
  }

  context.AddExistingBlock(then_bb);

  context.cur_block_has_terminator = false;
  Generate(*node.if_stmt);
  bool then_has_terminator = context.cur_block_has_terminator;
  if (!then_has_terminator) {
    if (context.current_block) {
      auto jump_inst = std::make_shared<IR::IRJump>(end_bb);
      context.current_block->AddInstruction(jump_inst);
      context.current_block->SetNext(end_bb);
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
      context.current_block->SetNext(end_bb);
    }
  }
  if (!then_has_terminator || !else_has_terminator) {
    context.AddExistingBlock(end_bb);
    context.cur_block_has_terminator = false;
  } else {
    context.cur_block_has_terminator = true;
  }
  return "";
}

std::string CodeGenerator::Visit(const WhileStmtAST& node) {
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
  context.enterNewWhileScope(loop_bb, continue_loop_bb, end_bb);
  if (context.current_block) {
    auto jump_inst = std::make_shared<IR::IRJump>(loop_bb);
    context.current_block->AddInstruction(jump_inst);
    context.current_block->SetNext(loop_bb);
  }
  context.AddExistingBlock(loop_bb);

  std::string cond_reg = Generate(*node.while_exp);
  if (context.current_block) {
    auto cond_val = context.GetIRValue(cond_reg);
    auto br_inst = std::make_shared<IR::IRBranch>(cond_val, continue_loop_bb, end_bb);
    context.current_block->AddInstruction(br_inst);
    context.current_block->SetNext(continue_loop_bb, end_bb);
  }
  context.AddExistingBlock(continue_loop_bb);

  Generate(*node.while_stmt);
  if (!context.cur_block_has_terminator) {
    if (context.current_block) {
      auto jump_inst = std::make_shared<IR::IRJump>(loop_bb);
      context.current_block->AddInstruction(jump_inst);
      context.current_block->SetNext(loop_bb);
    }
  }
  context.AddExistingBlock(end_bb);

  context.cur_block_has_terminator = false;
  context.exitWhileScope();
  return "";
}

std::string CodeGenerator::Visit(const BreakStmtAST& node) {
  if (context.cur_block_has_terminator) return "";
  WhileLabelContext while_label_context = context.GetWhileLabel();

  auto end_bb = context.GetWhileLabel().while_label_end;
  if (context.current_block && end_bb) {
    auto jump_inst = std::make_shared<IR::IRJump>(end_bb);
    context.current_block->AddInstruction(jump_inst);
    context.current_block->SetNext(end_bb);
  }

  std::string tool_label = while_label_context.while_label_continue->name + NumBuilder::GetNum();
  context.AddNewBlock(tool_label);
  return "";
}

std::string CodeGenerator::Visit(const ContinueStmtAST& node) {
  if (context.cur_block_has_terminator) return "";
  WhileLabelContext while_label_context = context.GetWhileLabel();

  auto begin_bb = context.GetWhileLabel().while_label_begin;
  if (context.current_block && begin_bb) {
    auto jump_inst = std::make_shared<IR::IRJump>(begin_bb);
    context.current_block->AddInstruction(jump_inst);
    context.current_block->SetNext(begin_bb);
  }

  std::string tool_label = while_label_context.while_label_continue->name + NumBuilder::GetNum();
  context.AddNewBlock(tool_label);
  return "";
}

std::string CodeGenerator::Visit(const ExpAST& node) {
  if (node.lor_exp) {
    return Generate(*node.lor_exp);
  }
  return "";
}

std::string CodeGenerator::Visit(const UnaryOPExpAST& node) {
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

std::string CodeGenerator::Visit(const PrimaryUnaryExpAST& node) {
  if (node.primary_exp) {
    return Generate(*node.primary_exp);
  }
  return "";
}

std::string CodeGenerator::Visit(const FuncCallExpAST& node) {
  if (context.cur_block_has_terminator) return "";
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
      IR::IRType call_type = (func_inf.ret_type == "float" || func_inf.ret_type == "f32") ? IR::IRType::IR_FLOAT : IR::IRType::IR_INT32;
      auto call_inst = std::make_shared<IR::IRCall>(func_reg, target_func, args, call_type);
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

std::string CodeGenerator::Visit(const FuncRParamsAST& node) {
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

std::string CodeGenerator::Visit(const ParenthesizedExprAST& node) {
  if (node.exp) {
    return Generate(*node.exp);
  }
  return "";
}

std::string CodeGenerator::Visit(const LValuePrimaryExprAST& node) {
  return Generate(*node.lval);
}

std::string CodeGenerator::Visit(const NumberPrimaryExprAST& node) {
  if (node.value.isFloat()) {
    return std::to_string(node.value.getFloat());
  } else {
    return std::to_string(node.value.getInt());
  }
}

std::string CodeGenerator::Visit(const OpAST& node) {
  return node.op;
}

std::string CodeGenerator::Visit(const UnaryMulExpAST& node) {
  if (node.unary_exp) {
    return Generate(*node.unary_exp);
  }
  return "";
}

std::string CodeGenerator::Visit(const BinaryMulExprAST& node) {
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

std::string CodeGenerator::Visit(const MulAddExpAST& node) {
  if (node.mul_exp) {
    return Generate(*node.mul_exp);
  }
  return "";
}

std::string CodeGenerator::Visit(const BinaryAddExprAST& node) {
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

std::string CodeGenerator::Visit(const AddRelExpAST& node) {
  if (node.add_exp) {
    return Generate(*node.add_exp);
  }
  return "";
}

std::string CodeGenerator::Visit(const BinaryRelExprAST& node) {
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

std::string CodeGenerator::Visit(const RelEqExpAST& node) {
  if (node.rel_exp) {
    return Generate(*node.rel_exp);
  }
  return "";
}

std::string CodeGenerator::Visit(const BinaryEqExprAST& node) {
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

std::string CodeGenerator::Visit(const EqLAndExpAST& node) {
  if (node.eq_exp) {
    return Generate(*node.eq_exp);
  }
  return "";
}

std::string CodeGenerator::Visit(const BinaryLogicalAndExprAST& node) {
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
      context.current_block->SetNext(then_bb, end_bb);
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
      context.current_block->SetNext(end_bb);
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

std::string CodeGenerator::Visit(const LAndLOrExpAST& node) {
  if (node.land_exp) {
    return Generate(*node.land_exp);
  }
  return "";
}

std::string CodeGenerator::Visit(const BinaryLogicalOrExprAST& node) {
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
      context.current_block->SetNext(then_bb, end_bb);
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
      context.current_block->SetNext(end_bb);
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

std::string CodeGenerator::Visit(const LValAST& node) {
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
    if (std::holds_alternative<float>(sym->value)) {
      std::string float_value = std::to_string(std::get<float>(sym->value));
      context.GetIRValue(float_value);
      return float_value;
    } else {
      std::string int_value = std::to_string(std::get<int>(sym->value));
      context.GetIRValue(int_value);
      return int_value;
    }
  } else if (sym->kind == SymbolKind::ARRAY || sym->kind == SymbolKind::CONST_ARRAY) {
    auto arr_info = std::get<ArrayInformation>(sym->value);
    
    std::string prev_reg = arr_info.address;
    std::shared_ptr<IR::IRValue> prev_val = context.GetIRValue(arr_info.address);

    if (arr_info.is_param) {
      std::string loaded_ptr = context.NewReg();
      if (context.current_block) {
        auto load_ptr_inst = std::make_shared<IR::IRLoad>(loaded_ptr, prev_val, IR::IRType::IR_POINTER);
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

std::string CodeGenerator::Visit(const ConstExpAST& node) {
  if (node.exp) {
    return Generate(*node.exp);
  }
  return "";
}

std::string CodeGenerator::Visit(const ConstDefAST& node) {
  ConstantVal constinit_val = node.const_init_val->getValue(context);
  std::string decl_type = *context.decl_type_context;
  std::shared_ptr<Symbol> sym;
  if (decl_type == "f32") {
    sym = std::make_shared<Symbol>(*node.ident, SymbolKind::CONSTANT, constinit_val.getFloat());
  } else {
    sym = std::make_shared<Symbol>(*node.ident, SymbolKind::CONSTANT, constinit_val.getInt());
  }
  context.scope->define(sym);
  return "";
}


static void FillConstInitConstantVal(const ValueAST& node,
                                     const std::vector<int>& dims, 
                                     const std::vector<int>& sub_sizes, 
                                     int parent_level, 
                                     int& index, 
                                     std::vector<ConstantVal>& flat_vals, 
                                     Context& context) {
  if (!node.isAggregateInitializer()) {
    ConstantVal val = node.getValue(context);
    if (index < (int)flat_vals.size()) {
      flat_vals[index] = val;
    }
    index++;
    return;
  }

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

  for (const auto& child : node.getInitializerElements()) {
    FillConstInitConstantVal(*child, dims, sub_sizes, target_level, index, flat_vals, context);
  }
  if (index < start_index + sub_size) {
    index = start_index + sub_size;
  }
}

static void FillConstantValInitValFromInitVal(const ValueAST& node,
                                              const std::vector<int>& dims, 
                                              const std::vector<int>& sub_sizes, 
                                              int parent_level, 
                                              int& index, 
                                              std::vector<ConstantVal>& flat_vals, 
                                              Context& context) {
  if (!node.isAggregateInitializer()) {
    ConstantVal val = node.getValue(context);
    if (index < (int)flat_vals.size()) {
      flat_vals[index] = val;
    }
    index++;
    return;
  }

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

  for (const auto& child : node.getInitializerElements()) {
    FillConstantValInitValFromInitVal(*child, dims, sub_sizes, target_level, index, flat_vals, context);
  }
  if (index < start_index + sub_size) {
    index = start_index + sub_size;
  }
}

static void FillInitValRegs(const ValueAST& node,
                            const std::vector<int>& dims, 
                            const std::vector<int>& sub_sizes, 
                            int parent_level, 
                            int& index, 
                            std::vector<std::string>& flat_vals, 
                            CodeGenerator& gen, 
                            Context& context) {
  if (!node.isAggregateInitializer()) {
    std::string val = gen.Generate(node);
    if (index < (int)flat_vals.size()) {
      flat_vals[index] = val;
    }
    index++;
    return;
  }

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

  for (const auto& child : node.getInitializerElements()) {
    FillInitValRegs(*child, dims, sub_sizes, target_level, index, flat_vals, gen, context);
  }
  if (index < start_index + sub_size) {
    index = start_index + sub_size;
  }
}
std::string CodeGenerator::Visit(const ArrayConstDefAST& node) {
  std::vector<int> dims;
  int arr_size = 1;
  for (auto& expr : node.const_exps) {
    int val = expr->getValue(context).getInt();
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

  std::vector<ConstantVal> init_vals(arr_size, ConstantVal(0));
  int fill_index = 0;
  FillConstInitConstantVal(*node.const_init_val, dims, sub_sizes, -1, fill_index, init_vals, context);

  auto address = context.GetAddress(*node.ident);
  std::string decl_type = *context.decl_type_context;
  ArrayInformation arr_info;
  std::shared_ptr<IR::IRGlobalAlloc> global_alloc;
  std::string arr_type = (decl_type == "f32") ? "float" : "i32";

  for (auto it = dims.rbegin(); it != dims.rend(); ++it) {
    arr_type = "[" + arr_type + ", " + std::to_string(*it) + "]";
  }

  if (decl_type == "f32") {
    std::vector<float> const_float_values;
    bool is_all_zero = true;
    for (auto& val : init_vals) {
      float f = val.getFloat();
      const_float_values.push_back(f);
      if (f != 0.0f) {
        is_all_zero = false;
      }
    }
    arr_info = ArrayInformation(address, dims, {}, const_float_values);
    if (context.scope->isGlobal()) {
      global_alloc = std::make_shared<IR::IRGlobalAlloc>(address, arr_type, is_all_zero, const_float_values, dims);
    }
  } else {
    std::vector<int> const_values;
    bool is_all_zero = true;
    for (auto& val : init_vals) {
      int i = val.getInt();
      const_values.push_back(i);
      if (i != 0) {
        is_all_zero = false;
      }
    }
    arr_info = ArrayInformation(address, dims, const_values, {});
    if (context.scope->isGlobal()) {
      global_alloc = std::make_shared<IR::IRGlobalAlloc>(address, arr_type, is_all_zero, const_values, dims);
    }
  }

  auto sym = std::make_shared<Symbol>(*node.ident, SymbolKind::CONST_ARRAY);
  sym->value = arr_info;

  if (context.scope->isGlobal()) {
    context.AddGlobal(global_alloc);
  } else {
    if (context.cur_block_has_terminator) {
      context.scope->define(sym);
      return "";
    }
    std::shared_ptr<IR::IRAlloc> alloc_inst = nullptr;
    if (context.current_block) {
      alloc_inst = std::make_shared<IR::IRAlloc>(address, (decl_type == "f32") ? "float" : "i32", dims);
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
        std::string val_str = (decl_type == "f32") ? std::to_string(init_vals[i].getFloat()) : std::to_string(init_vals[i].getInt());
        auto val = context.GetIRValue(val_str);
        auto store_inst = std::make_shared<IR::IRStore>(val, prev_val);
        context.current_block->AddInstruction(store_inst);
      }
    }
  }
  context.scope->define(sym);
  return "";
}
std::string CodeGenerator::Visit(const ConstExpConstInitValAST& node) {
  if (node.const_exp) {
    return Generate(*node.const_exp);
  }
  return "";
}

std::string CodeGenerator::Visit(const ConstInitValListAST& node) {
  return "";
}

std::string CodeGenerator::Visit(const CompoundConstInitValueAST& node) {
  if (node.const_init_val_list) {
    return Generate(*node.const_init_val_list);
  }
  return "";
}

std::string CodeGenerator::Visit(const ConstDeclWrapperAST& node) {
  if (node.const_decl) {
    return Generate(*node.const_decl);
  }
  return "";
}

std::string CodeGenerator::Visit(const VarDeclWrapperAST& node) {
  if (node.var_decl) {
    return Generate(*node.var_decl);
  }
  return "";
}

std::string CodeGenerator::Visit(const ConstDeclAST& node) {
  assert(*node.type == "i32" || *node.type == "f32");
  context.decl_type_context = std::make_shared<std::string>(*node.type);
  if (node.const_def_list) {
    Generate(*node.const_def_list);
  }
  return "";
}

std::string CodeGenerator::Visit(const ConstDefListAST& node) {
  for (auto& const_def : node.const_defs) {
    Generate(*const_def);
  }
  return "";
}

std::string CodeGenerator::Visit(const VarDeclAST& node) {
  context.decl_type_context = std::make_shared<std::string>(*node.type);
  if (node.var_def_list) {
    Generate(*node.var_def_list);
  }
  return "";
}

std::string CodeGenerator::Visit(const VarDefListAST& node) {
  for (auto& var_def : node.var_defs) {
    if (!var_def) {
      std::cerr << "Error: VarDef is nullptr\n";
      continue;
    }
    Generate(*var_def);
  }
  return "";
}

std::string CodeGenerator::Visit(const SimpleVarDefAST& node) {
  std::string address = context.GetAddress(*node.ident);
  auto sym = std::make_shared<Symbol>(*node.ident, SymbolKind::VARIABLE, address);
  std::string b_type = *context.decl_type_context;

  if (context.scope->isGlobal()) {
    auto global_alloc = std::make_shared<IR::IRGlobalAlloc>(address, b_type, true, std::vector<int32_t>{});
    context.AddGlobal(global_alloc);
  } else {
    if (context.cur_block_has_terminator) {
      context.scope->define(sym);
      return "";
    }
    if (context.current_block) {
      auto alloc_inst = std::make_shared<IR::IRAlloc>(address, b_type);
      context.current_block->AddInstruction(alloc_inst);
      context.SetIRValue(address, alloc_inst);
    }
  }
  context.scope->define(sym);
  return "";
}

std::string CodeGenerator::Visit(const InitializedVarDefAST& node) {
  std::string address = context.GetAddress(*node.ident);
  std::string b_type = *context.decl_type_context;
  auto sym = std::make_shared<Symbol>(*node.ident, SymbolKind::VARIABLE, address);
  if (context.scope->isGlobal()) {
    ConstantVal init_val = node.init_val->getValue(context);
    std::shared_ptr<IR::IRGlobalAlloc> global_alloc;
    if (b_type == "float") {
      global_alloc = std::make_shared<IR::IRGlobalAlloc>(address, b_type, false, std::vector<float>{init_val.getFloat()});
    } else {
      global_alloc = std::make_shared<IR::IRGlobalAlloc>(address, b_type, false, std::vector<int32_t>{init_val.getInt()});
    }
    context.AddGlobal(global_alloc);
  } else {
    if (context.cur_block_has_terminator) {
      context.scope->define(sym);
      return "";
    }
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

std::string CodeGenerator::Visit(const ArrayVarDefAST& node) {
  std::string address = context.GetAddress(*node.ident);
  std::string b_type = *context.decl_type_context;
  
  std::vector<int> dims;
  for (auto& expr : node.const_exps) {
    int val = expr->getValue(context).getInt();
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
    if (context.cur_block_has_terminator) {
      context.scope->define(sym);
      return "";
    }
    if (context.current_block) {
      auto alloc_inst = std::make_shared<IR::IRAlloc>(address, b_type, dims);
      context.current_block->AddInstruction(alloc_inst);
      context.SetIRValue(address, alloc_inst);
    }
  }
  context.scope->define(sym);
  return "";
}

std::string CodeGenerator::Visit(const ArrayInitializedVarDefAST& node) {
  std::string address = context.GetAddress(*node.ident);
  std::string b_type = *context.decl_type_context;
  
  std::vector<int> dims;
  int arr_size = 1;
  for (auto& expr : node.const_exps) {
    int val = expr->getValue(context).getInt();
    dims.push_back(val);
    arr_size *= val;
  }

  auto sym = std::make_shared<Symbol>(*node.ident, SymbolKind::ARRAY);
  sym->value = ArrayInformation(address, dims);

  std::string arr_type = (b_type == "float") ? "float" : "i32";
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
    std::vector<ConstantVal> init_vals(arr_size, ConstantVal(0));
    int fill_index = 0;
    FillConstantValInitValFromInitVal(*node.init_val_list, dims, sub_sizes, -1, fill_index, init_vals, context);

    std::shared_ptr<IR::IRGlobalAlloc> global_alloc;
    if (b_type == "float") {
      std::vector<float> init_nums;
      bool is_all_zero = true;
      for (auto& val : init_vals) {
        float f = val.getFloat();
        init_nums.push_back(f);
        if (f != 0.0f) {
          is_all_zero = false;
        }
      }
      global_alloc = std::make_shared<IR::IRGlobalAlloc>(address, arr_type, is_all_zero, init_nums, dims);
    } else {
      std::vector<int32_t> init_nums;
      bool is_all_zero = true;
      for (auto& val : init_vals) {
        int i = val.getInt();
        init_nums.push_back(i);
        if (i != 0) {
          is_all_zero = false;
        }
      }
      global_alloc = std::make_shared<IR::IRGlobalAlloc>(address, arr_type, is_all_zero, init_nums, dims);
    }
    context.AddGlobal(global_alloc);
  } else {
    if (context.cur_block_has_terminator) {
      context.scope->define(sym);
      return "";
    }
    std::vector<std::string> init_vals(arr_size, (b_type == "float") ? "0.0" : "0");
    int fill_index = 0;
    FillInitValRegs(*node.init_val_list, dims, sub_sizes, -1, fill_index, init_vals, *this, context);

    std::shared_ptr<IR::IRAlloc> alloc_inst = nullptr;
    if (context.current_block) {
      alloc_inst = std::make_shared<IR::IRAlloc>(address, b_type, dims);
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

std::string CodeGenerator::Visit(const InitValAST& node) {
  if (node.exp) {
    return Generate(*node.exp);
  }
  return "";
}

std::string CodeGenerator::Visit(const InitValListAST& node) {
  return "";
}

std::string GenerateIR(const BaseAST& node, Context& context) {
  CodeGenerator generator(context);
  return generator.Generate(node);
}
