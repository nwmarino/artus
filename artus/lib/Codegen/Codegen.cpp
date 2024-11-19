#include "llvm/IR/Verifier.h"
#include <llvm/ADT/APInt.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>

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
    fatal("Function not found in function table: " + decl->name);
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
    fatal("Expected an argument value for parameter declaration.");
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

  llvm::AllocaInst *alloca = createAlloca(builder->GetInsertBlock()->getParent(),
      decl->name, decl->T->toLLVMType(*context));

  builder->CreateStore(tmp, alloca);
  allocas[decl->name] = alloca;
}

void Codegen::genericCastCGN(CastExpr *expr) {
  expr->expr->pass(this);

  if (expr->T->isIntegerType()) {
    tmp = builder->CreateIntCast(tmp, expr->T->toLLVMType(*context), false);
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

  if (!isLValue) {
    tmp = builder->CreateLoad(expr->T->toLLVMType(*context), alloca);
    return;
  }

  tmp = alloca;
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
  // Dereference bases are pointers, so need lvalue flag.
  if (expr->op == UnaryExpr::UnaryOp::DeRef) {
    isLValue = true;
  }

  expr->base->pass(this);
  llvm::Value *base = tmp;

  switch (expr->op) {
    case UnaryExpr::UnaryOp::Negative:
      tmp = builder->CreateNeg(base);
      break;
    case UnaryExpr::UnaryOp::Not:
      tmp = builder->CreateNot(base);
      break;
    case UnaryExpr::UnaryOp::Ref:
      tmp = base;
      break;
    case UnaryExpr::UnaryOp::DeRef:
      tmp = builder->CreateLoad(expr->getType()->toLLVMType(*context), base);
      break;
    default:
      fatal("unknown unary operator");
  }

  isLValue = false;
}

void Codegen::visit(BinaryExpr *expr) {
  if (expr->isAssignment())
    this->isLValue = true;

  expr->lhs->pass(this);
  llvm::Value *lhs = tmp;
  expr->rhs->pass(this);
  llvm::Value *rhs = tmp;

  switch (expr->op) {
    case BinaryExpr::BinaryOp::Assign:
      tmp = builder->CreateStore(rhs, lhs);
      break;
    case BinaryExpr::BinaryOp::Add:
      tmp = builder->CreateAdd(lhs, rhs);
      break;
    case BinaryExpr::BinaryOp::Sub:
      tmp = builder->CreateSub(lhs, rhs);
      break;
    case BinaryExpr::BinaryOp::Mult:
      tmp = builder->CreateMul(lhs, rhs);
      break;
    case BinaryExpr::BinaryOp::Div:
      tmp = builder->CreateSDiv(lhs, rhs);
      break;
    default:
      fatal("unknown binary operator");
  }

  this->isLValue = false;
}

void Codegen::visit(BooleanLiteral *expr) {
  tmp = llvm::ConstantInt::get(*context, llvm::APInt(1, expr->value, true));
}

void Codegen::visit(IntegerLiteral *expr) {
  tmp = llvm::ConstantInt::get(*context, llvm::APInt(
      expr->T->getBitWidth(), expr->value, true));
}

void Codegen::visit(CharLiteral *expr) {
  tmp = llvm::ConstantInt::get(*context, llvm::APInt(8, expr->value, true));
}

void Codegen::visit(StringLiteral *expr) { /* unsupported for now */ }

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

  // Non lvalue array access need to be loaded; no element ptr.
  if (!isLValue) {
    const ArrayType *AT = dynamic_cast<const ArrayType *>(baseExpr->T);
    assert(AT && "expected array type for array access expression");

    llvm::Value *elementPtr = builder->CreateGEP(AT->toLLVMType(*context), 
        allocas[baseExpr->ident], { builder->getInt64(0), index });

    tmp = builder->CreateLoad(AT->getElementType()->toLLVMType(*context), elementPtr);
    return;
  }

  if (baseExpr->T->isPointerType()) {
    vector<llvm::Value *> indices = { index };
    llvm::LoadInst *ptr = builder->CreateLoad(expr->T->toLLVMType(*context), base);

    const PointerType *PT = dynamic_cast<const PointerType *>(expr->T);
    assert(PT && "expected pointer type for array access expression");

    tmp = builder->CreateGEP(PT->getPointeeType()->toLLVMType(*context), ptr, indices);
    return;
  }
  
  if (baseExpr->T->isArrayType()) {
    tmp = builder->CreateInBoundsGEP(baseExpr->T->toLLVMType(*context), 
        allocas[baseExpr->ident], { builder->getInt64(0), index });
    return;
  }

  fatal("unknown array access expression type");
}

void Codegen::visit(CompoundStmt *stmt) {
  for (const std::unique_ptr<Stmt> &s : stmt->stmts) {
    s->pass(this);
  }
}

void Codegen::visit(DeclStmt *stmt) {
  stmt->decl->pass(this);
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
