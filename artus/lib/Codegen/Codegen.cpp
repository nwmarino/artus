#include "llvm/ADT/APInt.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Verifier.h"
#include <llvm/Support/Alignment.h>

#include "../../include/AST/Expr.h"
#include "../../include/Codegen/Codegen.h"
#include "../../include/Core/Logger.h"

using namespace artus;

Codegen::Codegen(Context *ctx, llvm::TargetMachine *TM) {
  // Instantiate the LLVM context, IR builder, and module.
  context = std::make_unique<llvm::LLVMContext>();
  builder = std::make_unique<llvm::IRBuilder<>>(*context);
  module = std::make_unique<llvm::Module>(
      ctx->cache->getActive()->getIdentifier(), *context);

  // Setup the target triple and data layout for the module.
  module->setTargetTriple(TM->getTargetTriple().getTriple());
  module->setDataLayout(TM->createDataLayout());

  ctx->cache->getActive()->pass(this);
}

Codegen::~Codegen() {
  this->module.reset();
  this->builder.reset();
  this->context.reset();
}

llvm::AllocaInst *Codegen::createAlloca(llvm::Function *fn, 
                                        const std::string &var, llvm::Type *T) {
  llvm::IRBuilder<> tmp(&fn->getEntryBlock(), 
      fn->getEntryBlock().begin());
  return tmp.CreateAlloca(T, nullptr, var);
}

void Codegen::visit(PackageUnitDecl *decl) {
  // Create the function table for this package.
  for (std::unique_ptr<Decl> &d : decl->decls) {
    if (FunctionDecl *functionDecl = dynamic_cast<FunctionDecl *>(d.get())) {
      // Create the function type and function in the module.
      llvm::Type *FT = functionDecl->T->toLLVMType(*context);
      llvm::Function *FN = llvm::Function::Create(
          llvm::cast<llvm::FunctionType>(FT), 
          llvm::Function::ExternalLinkage, 
          functionDecl->getName(), 
          module.get()
      );

      // Set the function arguments names.
      unsigned idx = 0;
      for (llvm::Argument &arg : FN->args()) {
        arg.setName(functionDecl->getParam(idx++)->getName());
      }

      // Add the function to the function table.
      functions[functionDecl->getName()] = FN;
    }
  }

  // Iterate over all declarations and generate code for them.
  for (std::unique_ptr<Decl> &d : decl->decls) {
    d->pass(this);
  }
}

void Codegen::visit(FunctionDecl *decl) {
  llvm::Function *FN = functions[decl->name];
  if (!FN) {
    fatal("Function not found in function table: " + decl->name,
        { decl->span.file, decl->span.line, decl->span.col });
  }

  this->FN = FN;

  // Create the entry basic block for the function.
  llvm::BasicBlock *entryBB = llvm::BasicBlock::Create(*context, 
      "entry", FN);
  builder->SetInsertPoint(entryBB);

  // Create the allocas for the function parameters.
  allocas.clear();
  for (unsigned idx = 0; idx < decl->getNumParams(); ++idx) {
    tmp = FN->getArg(idx);
    decl->params[idx]->pass(this);
  }

  // Generate code for the function body.
  decl->body->pass(this);
  llvm::verifyFunction(*FN);
  this->FN = nullptr;
}

void Codegen::visit(ParamVarDecl *decl) {
  // Verify that a parent function exists and the temporary value is an arg.
  assert(FN && "Function must be set before generating code for parameters.");
  if (!llvm::isa<llvm::Argument>(tmp)) {
    fatal("Expected an argument value for parameter declaration.",
        { decl->span.file, decl->span.line, decl->span.col });
  }

  // Create an alloca for the parameter in the entry block.
  llvm::AllocaInst *alloca = createAlloca(FN, tmp->getName().str(),
      tmp->getType());
  builder->CreateStore(tmp, alloca);
  allocas[tmp->getName().str()] = alloca;
}

void Codegen::visit(LabelDecl *decl) { /* unused */ }

void Codegen::visit(VarDecl *decl) {
  decl->init->pass(this);

  llvm::AllocaInst *alloca = createAlloca(
      builder->GetInsertBlock()->getParent(), decl->name,
       decl->T->toLLVMType(*context)
  );

  builder->CreateStore(tmp, alloca);
  allocas[decl->name] = alloca;
}

void Codegen::genericCastCGN(CastExpr *expr) {
  expr->expr->pass(this);

  if (expr->T->isIntegerType()) {
    // Cast an integer to a floating point type.
    if (expr->expr->getType()->isFloatingPointType()) {
      tmp = builder->CreateFPToSI(tmp, expr->T->toLLVMType(*context));
      return;
    }

    tmp = builder->CreateIntCast(tmp, expr->T->toLLVMType(*context), false);
  } else if (expr->T->isFloatingPointType()) {
    // Cast a floating point to an integer type.
    if (expr->expr->getType()->isIntegerType()) {
      tmp = builder->CreateSIToFP(tmp, expr->T->toLLVMType(*context));
      return;
    }

    tmp = builder->CreateFPCast(tmp, expr->T->toLLVMType(*context));
  }
}

void Codegen::visit(ImplicitCastExpr *expr) { genericCastCGN(expr); }

void Codegen::visit(ExplicitCastExpr *expr) { genericCastCGN(expr); }

void Codegen::visit(DeclRefExpr *expr) {
  llvm::AllocaInst *alloca = allocas[expr->ident];
  if (!alloca) {
    fatal("unresolved variable: " + expr->ident, 
        { expr->span.file, expr->span.line, expr->span.col });
  }

  if (needPtr) {
    tmp = alloca;
    return;
  }

  tmp = builder->CreateLoad(expr->T->toLLVMType(*context), alloca);
}

void Codegen::visit(CallExpr *expr) {
  llvm::Function *callee = functions[expr->ident];
  if (!callee) {
    fatal("function not found in function table: " + expr->ident,
        { expr->span.file, expr->span.line, expr->span.col });
  }

  std::vector<llvm::Value *> args;
  for (size_t i = 0; i < expr->getNumArgs(); ++i) {
    expr->getArg(i)->pass(this);
    args.push_back(tmp);
  }

  tmp = builder->CreateCall(callee, args);
}

void Codegen::visit(UnaryExpr *expr) {
  needPtr = expr->op == UnaryExpr::UnaryOp::Ref;
  expr->base->pass(this);
  needPtr = false;
  llvm::Value *base = tmp;

  switch (expr->op) {
    case UnaryExpr::UnaryOp::Negative:
      tmp = builder->CreateNeg(base);
      break;
    case UnaryExpr::UnaryOp::Not:
      tmp = builder->CreateNot(base);
      break;
    case UnaryExpr::UnaryOp::Ref: // tmp is already the pointer.
      break;
    case UnaryExpr::UnaryOp::DeRef:
      if (!needPtr)
        tmp = builder->CreateLoad(expr->getType()->toLLVMType(*context), 
            base);
      break;
    default: 
      fatal("unknown unary operator", { expr->span.file,
        expr->span.line, expr->span.col });
  }
}

void Codegen::visit(BinaryExpr *expr) {
  // Signify that a pointer is needed if assigning directly. (=)
  needPtr = expr->isDirectAssignment();

  // Codegen pass on the left and right hand side of the binary expression.
  expr->lhs->pass(this);
  llvm::Value *lhs = tmp;
  needPtr = false;
  expr->rhs->pass(this);
  llvm::Value *rhs = tmp;

  switch (expr->op) {
    case BinaryExpr::BinaryOp::Assign: // =
      tmp = builder->CreateStore(rhs, lhs);
      break;
    case BinaryExpr::BinaryOp::AddAssign: {
      llvm::Value *addVal = nullptr;
      if (expr->T->isFloatingPointType()) {
        addVal = builder->CreateFAdd(lhs, rhs);
      } else {
        addVal = builder->CreateAdd(lhs, rhs);
      }

      // Get the pointer for the left hand side.
      needPtr = true;
      expr->lhs->pass(this);
      tmp = builder->CreateStore(addVal, tmp);
      break;
    }
    case BinaryExpr::BinaryOp::SubAssign: {
      llvm::Value *subVal = nullptr;
      if (expr->T->isFloatingPointType()) {
        subVal = builder->CreateFSub(lhs, rhs);
      } else {
        subVal = builder->CreateSub(lhs, rhs);
      }

      // Get the pointer for the left hand side.
      needPtr = true;
      expr->lhs->pass(this);
      tmp = builder->CreateStore(subVal, tmp);
      break;
    }
    case BinaryExpr::BinaryOp::MultAssign: {
      llvm::Value *multVal = nullptr;
      if (expr->T->isFloatingPointType()) {
        multVal = builder->CreateFMul(lhs, rhs);
      } else {
        multVal = builder->CreateMul(lhs, rhs);
      }

      // Get the pointer for the left hand side.
      needPtr = true;
      expr->lhs->pass(this);
      tmp = builder->CreateStore(multVal, tmp);
      break;
    }
    case BinaryExpr::BinaryOp::DivAssign: {
      llvm::Value *divVal = nullptr;
      if (expr->T->isFloatingPointType()) {
        divVal = builder->CreateFDiv(lhs, rhs);
      } else {
        divVal = builder->CreateSDiv(lhs, rhs);
      }

      // Get the pointer for the left hand side.
      needPtr = true;
      expr->lhs->pass(this);
      tmp = builder->CreateStore(divVal, tmp);
      break;
    }
    case BinaryExpr::BinaryOp::Equals:
      if (expr->T->isFloatingPointType()) {
        tmp = builder->CreateFCmpOEQ(lhs, rhs);
        break;
      }
      tmp = builder->CreateICmpEQ(lhs, rhs);
      break;
    case BinaryExpr::BinaryOp::NotEquals:
      if (expr->T->isFloatingPointType()) {
        tmp = builder->CreateFCmpONE(lhs, rhs);
        break;
      }
      tmp = builder->CreateICmpNE(lhs, rhs);
      break;
    case BinaryExpr::BinaryOp::LessThan:
      if (expr->T->isFloatingPointType()) {
        tmp = builder->CreateFCmpOLT(lhs, rhs);
        break;
      }
      tmp = builder->CreateICmpSLT(lhs, rhs);
      break;
    case BinaryExpr::BinaryOp::GreaterThan:
      if (expr->T->isFloatingPointType()) {
        tmp = builder->CreateFCmpOGT(lhs, rhs);
        break;
      }
      tmp = builder->CreateICmpSGT(lhs, rhs);
      break;
    case BinaryExpr::BinaryOp::LessEquals:
      if (expr->T->isFloatingPointType()) {
        tmp = builder->CreateFCmpOLE(lhs, rhs);
        break;
      }
      tmp = builder->CreateICmpSLE(lhs, rhs);
      break;
    case BinaryExpr::BinaryOp::GreaterEquals:
      if (expr->T->isFloatingPointType()) {
        tmp = builder->CreateFCmpOGE(lhs, rhs);
        break;
      }
      tmp = builder->CreateICmpSGE(lhs, rhs);
      break;
    case BinaryExpr::BinaryOp::LogicalAnd:
      tmp = builder->CreateAnd(lhs, rhs);
      break;
    case BinaryExpr::BinaryOp::LogicalOr:
      tmp = builder->CreateOr(lhs, rhs);
      break;
    case BinaryExpr::BinaryOp::LogicalXor:
      tmp = builder->CreateXor(lhs, rhs);
      break;
    case BinaryExpr::BinaryOp::Add:
      if (expr->T->isFloatingPointType()) {
        tmp = builder->CreateFAdd(lhs, rhs);
        break;
      }
      tmp = builder->CreateAdd(lhs, rhs);
      break;
    case BinaryExpr::BinaryOp::Sub:
      if (expr->T->isFloatingPointType()) {
        tmp = builder->CreateFSub(lhs, rhs);
        break;
      }
      tmp = builder->CreateSub(lhs, rhs);
      break;
    case BinaryExpr::BinaryOp::Mult:
      if (expr->T->isFloatingPointType()) {
        tmp = builder->CreateFMul(lhs, rhs);
        break;
      }
      tmp = builder->CreateMul(lhs, rhs);
      break;
    case BinaryExpr::BinaryOp::Div:
      if (expr->T->isFloatingPointType()) {
        tmp = builder->CreateFDiv(lhs, rhs);
        break;
      }
      tmp = builder->CreateSDiv(lhs, rhs);
      break;
    default: fatal("unknown binary operator", { expr->span.file,
        expr->span.line, expr->span.col });
  } // end switch

  needPtr = false;
}

void Codegen::visit(BooleanLiteral *expr)
{ tmp = llvm::ConstantInt::get(*context, llvm::APInt(1, expr->value, true)); }

void Codegen::visit(IntegerLiteral *expr) {
  tmp = llvm::ConstantInt::get(*context, llvm::APInt(
      expr->T->getBitWidth(), expr->value, true));
}

void Codegen::visit(FPLiteral *expr) 
{ tmp = llvm::ConstantFP::get(*context, llvm::APFloat(expr->value)); }

void Codegen::visit(CharLiteral *expr) 
{ tmp = llvm::ConstantInt::get(*context, llvm::APInt(8, expr->value, true)); }

void Codegen::visit(StringLiteral *expr) 
{ tmp = builder->CreateGlobalStringPtr(expr->value); }

void Codegen::visit(NullExpr *expr) {
  tmp = llvm::ConstantPointerNull::get(llvm::PointerType::get(
      expr->T->toLLVMType(*context), 0));
}

void Codegen::visit(ArrayInitExpr *expr) {
  llvm::Type *T = expr->T->toLLVMType(*context);
  llvm::Value *array = llvm::UndefValue::get(T);

  for (size_t i = 0; i < expr->getNumExprs(); ++i) {
    expr->getExpr(i)->pass(this);
    array = builder->CreateInsertValue(array, tmp, i);
  }

  tmp = array;
}

void Codegen::visit(ArrayAccessExpr *expr) {
  expr->base->pass(this);
  llvm::Value *base = tmp;
  expr->index->pass(this);
  llvm::Value *index = tmp;

  DeclRefExpr *baseExpr = dynamic_cast<DeclRefExpr *>(expr->base.get());
  assert (baseExpr && "expected decl ref expression for array access base");

  // Non-lvalue array access: need element load, not element ptr.
  if (!needPtr) {
    if (baseExpr->T->isArrayType()) {
      const ArrayType *AT = dynamic_cast<const ArrayType *>(baseExpr->T);
      assert(AT && "expected array type for array access expression");

      llvm::Value *elementPtr = builder->CreateGEP(AT->toLLVMType(*context), 
          allocas[baseExpr->ident], { builder->getInt64(0), index });

      tmp = builder->CreateLoad(AT->getElementType()->toLLVMType(*context), 
          elementPtr);
    } else if (baseExpr->T->isStringType()) {
      llvm::Value *elementPtr = builder->CreateInBoundsGEP(builder->getInt8Ty(), 
          base, index);
      tmp = builder->CreateLoad(builder->getInt8Ty(), elementPtr);
    }

    return;
  }

  // If the base reference is a pointer, load it first.
  if (baseExpr->T->isPointerType()) {
    vector<llvm::Value *> indices = { index };
    llvm::LoadInst *ptr = builder->CreateLoad(expr->T->toLLVMType(*context), 
        base);

    const PointerType *PT = dynamic_cast<const PointerType *>(expr->T);
    assert(PT && "expected pointer type for array access expression");

    tmp = builder->CreateGEP(PT->getPointeeType()->toLLVMType(*context), 
        ptr, indices);
    return;
  }
  
  // Generic array access through an element pointer.
  if (baseExpr->T->isArrayType()) {
    tmp = builder->CreateInBoundsGEP(baseExpr->T->toLLVMType(*context), 
        allocas[baseExpr->ident], { builder->getInt64(0), index });
    return;
  }

  fatal("unknown array access expression type", { expr->span.file, 
      expr->span.line, expr->span.col });
}

void Codegen::visit(CompoundStmt *stmt) {
  for (const std::unique_ptr<Stmt> &s : stmt->stmts) {
    s->pass(this);
  }
}

void Codegen::visit(DeclStmt *stmt) {
  stmt->decl->pass(this);
}

void Codegen::visit(IfStmt *stmt) {
  // Fetch the value for the if statement condition.
  stmt->cond->pass(this);
  llvm::Value *condVal = tmp;
  if (!condVal) {
    fatal("expected expression in if statement condition", 
        { stmt->span.file, stmt->span.line, stmt->span.col });
  }

  // Initialize basic blocks for this if control flow.
  llvm::BasicBlock *thenBlock = llvm::BasicBlock::Create(*context, 
      "then", FN);
  llvm::BasicBlock *elseBlock = llvm::BasicBlock::Create(*context,
      "else");
  llvm::BasicBlock *mergeBlock = llvm::BasicBlock::Create(*context,
      "merge");

  // Create a branch from the the then block to the else block, should it exist.
  if (stmt->hasElse()) {
    builder->CreateCondBr(condVal, thenBlock, elseBlock);
  } else { // No else block, go straight to merge after the 'then' statement.
    builder->CreateCondBr(condVal, thenBlock, mergeBlock);
  }

  // Codegen pass on the then body, and insert instructions to its basic block.
  builder->SetInsertPoint(thenBlock);
  stmt->thenStmt->pass(this);
  llvm::Value *thenVal = tmp;
  assert(thenVal && "expected value for then statement");

  // If no terminator was made, branch to the merge block.
  thenBlock = builder->GetInsertBlock();
  if (!thenBlock->getTerminator()) {
    builder->CreateBr(mergeBlock);
  }

  // Codegen pass on the else statement, if it exists.
  builder->SetInsertPoint(elseBlock);
  llvm::Value *elseVal = nullptr;
  if (stmt->hasElse()) {
    FN->insert(FN->end(), elseBlock);
    stmt->elseStmt->pass(this);
    elseBlock = builder->GetInsertBlock();
  }

  // If no terminator was made, branch to the merge block.
  if (!stmt->hasElse() || !elseBlock->getTerminator()) {
    builder->CreateBr(mergeBlock);
  }

  // Insert the merge block if it used.
  if (mergeBlock->hasNPredecessorsOrMore(1)) {
    FN->insert(FN->end(), mergeBlock);
    builder->SetInsertPoint(mergeBlock);
  }
}

void Codegen::visit(WhileStmt *stmt) {
  // Initialize basic blocks for the while control flow.
  llvm::BasicBlock *condBlock = llvm::BasicBlock::Create(*context, "cond", FN);
  llvm::BasicBlock *loopBlock = llvm::BasicBlock::Create(*context, "while");
  llvm::BasicBlock *mergeBlock = llvm::BasicBlock::Create(*context, "merge");

  // Create a branch to the conditional evaluation block.
  builder->CreateBr(condBlock);
  builder->SetInsertPoint(condBlock);

  // Codegen pass on the loop condition.
  stmt->cond->pass(this);
  llvm::Value *condVal = tmp;
  if (!condVal) {
    fatal("expected expression in while statement condition", 
        { stmt->span.file, stmt->span.line, stmt->span.col });
  }

  // Repeat the loop if the condition is true.
  builder->CreateCondBr(condVal, loopBlock, mergeBlock);
  FN->insert(FN->end(), loopBlock);
  builder->SetInsertPoint(loopBlock);

  // Codegen pass on the loop body.
  stmt->body->pass(this);
  builder->CreateBr(condBlock);

  // Create a branch to the merge block if the loop body has no terminator.
  FN->insert(FN->end(), mergeBlock);
  builder->SetInsertPoint(mergeBlock);
}

void Codegen::visit(UntilStmt *stmt) {
  // Initialize basic blocks for the while control flow.
  llvm::BasicBlock *condBlock = llvm::BasicBlock::Create(*context, "cond", FN);
  llvm::BasicBlock *loopBlock = llvm::BasicBlock::Create(*context, "until");
  llvm::BasicBlock *mergeBlock = llvm::BasicBlock::Create(*context, "merge");

  // Create a branch to the conditional evaluation block.
  builder->CreateBr(condBlock);
  builder->SetInsertPoint(condBlock);

  // Codegen pass on the loop condition.
  stmt->cond->pass(this);
  llvm::Value *condVal = tmp;
  if (!condVal) {
    fatal("expected expression in while statement condition", 
        { stmt->span.file, stmt->span.line, stmt->span.col });
  }

  // Repeat the loop if the condition is true.
  builder->CreateCondBr(condVal, mergeBlock, loopBlock);
  FN->insert(FN->end(), loopBlock);
  builder->SetInsertPoint(loopBlock);

  // Codegen pass on the loop body.
  stmt->body->pass(this);
  builder->CreateBr(condBlock);

  // Create a branch to the merge block if the loop body has no terminator.
  FN->insert(FN->end(), mergeBlock);
  builder->SetInsertPoint(mergeBlock);
}

void Codegen::visit(CaseStmt *stmt) {
  stmt->body->pass(this);
}

void Codegen::visit(DefaultStmt *stmt) {
  stmt->body->pass(this);
}

void Codegen::visit(MatchStmt *stmt) {
  // Fetch the value for the match statement expression.
  stmt->expr->pass(this);
  llvm::Value *matchVal = tmp;
  if (!matchVal) {
    fatal("expected expression in match statement", 
        { stmt->span.file, stmt->span.line, stmt->span.col });
  }

  // Initialize basic blocks for the match control flow.
  llvm::BasicBlock *mergeBlock = llvm::BasicBlock::Create(*context, 
      "merge");
  llvm::BasicBlock *defaultBlock = llvm::BasicBlock::Create(*context, 
      "default");

  llvm::SwitchInst *swInst = nullptr;
  if (stmt->hasDefault()) {
    swInst = builder->CreateSwitch(matchVal, defaultBlock, 
        stmt->cases.size() - 1);
  } else {
    swInst = builder->CreateSwitch(matchVal, mergeBlock, 
        stmt->cases.size() - 1);
  }

  // Codegen each of the match cases.
  for (std::unique_ptr<MatchCase> &c : stmt->cases) {
    // Only codegen on case statements for now.
    CaseStmt *caseStmt = dynamic_cast<CaseStmt *>(c.get());
    if (!caseStmt) {
      continue;
    }

    // Initialize the basic block for the case statement body.
    llvm::BasicBlock *caseBlock = llvm::BasicBlock::Create(*context, "case", FN);

    // Codegen pass on the case statement expression.
    caseStmt->expr->pass(this);
    if (!tmp) {
      fatal("expected expression in case statement", 
          { stmt->span.file, stmt->span.line, stmt->span.col });
    }

    // Evaluate the expression as an integer constant.
    llvm::ConstantInt *caseVal = llvm::dyn_cast<llvm::ConstantInt>(tmp);
    if (!caseVal) {
      fatal("expected integer evaluable expression in case statement", 
          { stmt->span.file, stmt->span.line, stmt->span.col });
    }

    // Add the case to the switch instruction, and codegen the body of the case.
    swInst->addCase(caseVal, caseBlock);
    builder->SetInsertPoint(caseBlock);
    caseStmt->pass(this);

    // Branch to the merge block if the case body has no terminator.
    if (!caseBlock->getTerminator()) {
      builder->CreateBr(mergeBlock);
    }
  }

  // If the default block is actually used, codegen for it and insert it.
  if (defaultBlock->hasNPredecessorsOrMore(1)) {
    FN->insert(FN->end(), defaultBlock);
    builder->SetInsertPoint(defaultBlock);
    stmt->getDefault()->pass(this);
    builder->CreateBr(mergeBlock);
  }

  // If the merge block is actually used, insert it.
  
  if (mergeBlock->hasNPredecessorsOrMore(1)) {
    FN->insert(FN->end(), mergeBlock);
    builder->SetInsertPoint(mergeBlock);
  }

  builder->SetInsertPoint(mergeBlock);
}

void Codegen::visit(LabelStmt *stmt) {
  if (stmt->name == "entry") {
    return;
  }

  llvm::BasicBlock *labelBB = llvm::BasicBlock::Create(*context, 
    stmt->name, FN);
  builder->CreateBr(labelBB);
  builder->SetInsertPoint(labelBB);
}

void Codegen::visit(JmpStmt *stmt) {
  /*
  llvm::BasicBlock *jmpBB = nullptr;
  for (llvm::BasicBlock &BB : *FN) {
    if (BB.getName() == stmt->name) {
      jmpBB = &BB;
      break;
    }
  }

  if (!jmpBB) {
    fatal("label not found in function: " + stmt->name);
  }

  builder->CreateBr(jmpBB);*/

  llvm::BasicBlock *jmpBB = llvm::BasicBlock::Create(*context, 
    stmt->name, FN);
  builder->CreateBr(jmpBB);
  builder->SetInsertPoint(jmpBB);
}

void Codegen::visit(RetStmt *stmt) {
  stmt->expr->pass(this);
  builder->CreateRet(tmp);
}
