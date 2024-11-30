//>==- Codegen.cpp --------------------------------------------------------==<//
//
// The following source implements a code generation to LLVM IR pass for a
// semantically valid abstract syntax tree rooted at a package declaration.
//
//>==----------------------------------------------------------------------==<//

#include "llvm/ADT/APInt.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

#include "../../include/AST/Decl.h"
#include "../../include/AST/DeclBase.h"
#include "../../include/AST/Expr.h"
#include "../../include/AST/Stmt.h"
#include "../../include/Codegen/Codegen.h"
#include "../../include/Core/Logger.h"

using namespace artus;

Codegen::Codegen(Context *ctx, const std::string &instance, 
                 llvm::TargetMachine *TM) : instance(instance) {
  // Instantiate the LLVM context, IR builder, and module.
  context = std::make_unique<llvm::LLVMContext>();
  builder = std::make_unique<llvm::IRBuilder<>>(*context);
  module = std::make_unique<llvm::Module>(this->instance, *context);

  this->fTable.clear();
  this->sTable.clear();
  this->allocas.clear();

  this->tmp = nullptr;
  this->FN = nullptr;
  this->hostCondBlock = nullptr;
  this->hostMergeBlock = nullptr;
  this->needPtr = false;
  this->currPkg = nullptr;

  // Setup the target triple and data layout for the module.
  module->setTargetTriple(TM->getTargetTriple().getTriple());
  module->setDataLayout(TM->createDataLayout());

  this->addStd();

  for (PackageUnitDecl *pkg : ctx->PM->getPackages())
    createStructMappings(pkg);

  for (PackageUnitDecl *pkg : ctx->PM->getPackages())
    createFunctionMappings(pkg);

  for (PackageUnitDecl *pkg : ctx->PM->getPackages())
    pkg->pass(this);
}

Codegen::~Codegen() {
  this->module.reset();
  this->builder.reset();
  this->context.reset();
}

void Codegen::addStd() {
  addStdIO_print();
  addStdIO_println();
  addStdIO_readln();
  addStdMEM_malloc();
  addStdMEM_free();
}

void Codegen::addStdIO_print() {
  // Get the printf function.
  llvm::FunctionType *printfFT = llvm::FunctionType::get(
    builder->getInt32Ty(),   
    builder->getInt8Ty()->getPointerTo(),
    false
  );
  llvm::FunctionCallee printfFN = module->getOrInsertFunction(
    "printf", 
    printfFT
  );

  // Build the print function.
  llvm::FunctionType *printFT = llvm::FunctionType::get(
    builder->getVoidTy(),
    builder->getInt8Ty()->getPointerTo(), 
    false
  );
  llvm::Function *printFN = llvm::Function::Create(
    printFT, 
    llvm::Function::ExternalLinkage, 
    "print",
    *module
  );

  // Create the entry block for print.
  llvm::BasicBlock *entry = llvm::BasicBlock::Create(
    *context, "entry", printFN
  );
  builder->SetInsertPoint(entry);

  llvm::Function::arg_iterator args = printFN->arg_begin();
  llvm::Value *arg = args++;

  // Create a call to the original printf function with the string arg.
  builder->CreateCall(printfFN, { arg });
  builder->CreateRetVoid();

  // Add the print function to the ftable.
  this->fTable["std_io"]["print"] = printFN;
}

void Codegen::addStdIO_println() {
  // Get the printf function.
  llvm::FunctionType *printfFT = llvm::FunctionType::get(
    builder->getInt32Ty(), 
    builder->getInt8Ty()->getPointerTo(),
    true
  );
  llvm::FunctionCallee printfFN = module->getOrInsertFunction(
    "printf", 
    printfFT
  );

  // Build the println function.
  llvm::FunctionType *printlnFT = llvm::FunctionType::get(
    builder->getVoidTy(),
    builder->getInt8Ty()->getPointerTo(), 
    false
  );
  llvm::Function *printlnFN = llvm::Function::Create(
    printlnFT, 
    llvm::Function::ExternalLinkage, 
    "println",
    *module
  );

  // Create the entry block for println.
  llvm::BasicBlock *entry = llvm::BasicBlock::Create(
    *context, "entry", printlnFN
  );
  builder->SetInsertPoint(entry);
  
  // Create a format string with the argument and a newline.
  llvm::Function::arg_iterator args = printlnFN->arg_begin();
  llvm::Value *arg = args++;
  llvm::Value *format = builder->CreateGlobalStringPtr("%s\n");

  // Call the original printf function with the formatted arguments.
  builder->CreateCall(printfFN, { format, arg });
  builder->CreateRetVoid();

  // Add the println function to the ftable.
  this->fTable["std_io"]["println"] = printlnFN;
}

void Codegen::addStdIO_readln() {
  std::vector<llvm::Type *> readstrArgs = { builder->getInt8Ty()->getPointerTo() };
  llvm::FunctionType *readstrFT = llvm::FunctionType::get(
    builder->getVoidTy(), readstrArgs, false
  );

  llvm::Function *readstrFN = llvm::Function::Create(
    readstrFT,
    llvm::Function::ExternalLinkage, 
    "read_str", 
    *module
  );

  llvm::BasicBlock *entry = llvm::BasicBlock::Create(
    *context, 
    "entry", 
    readstrFN
  );
  builder->SetInsertPoint(entry);

  llvm::Value *formatStr = builder->CreateGlobalStringPtr("%s");

  llvm::Function::arg_iterator args = readstrFN->arg_begin();
  llvm::Value *strPtr = args++;

  std::vector<llvm::Value *> scanfArgs = {};
  scanfArgs.push_back(formatStr);
  scanfArgs.push_back(strPtr);

  // Define the scanf function type: int (char*, ...)
  std::vector<llvm::Type *> scanfArgsTypes = { builder->getInt8Ty()->getPointerTo() };
  llvm::FunctionType *scanfFT = llvm::FunctionType::get(
    builder->getInt32Ty(), 
    scanfArgsTypes, 
    true
  );
  
  llvm::Function *scanfFN = llvm::Function::Create(
    scanfFT, 
    llvm::Function::ExternalLinkage, 
    "scanf",
    *module
  );

  builder->CreateCall(scanfFN, scanfArgs);
  builder->CreateRetVoid();

  llvm::verifyFunction(*readstrFN);

  this->fTable["std_io"]["read_str"] = readstrFN;
}

void Codegen::addStdMEM_malloc() {
  llvm::FunctionType *mallocFT = llvm::FunctionType::get(
    llvm::Type::getInt8Ty(*context)->getPointerTo(),
    { llvm::Type::getInt64Ty(*context) },
    false
  );

  llvm::Function *mallocFN = llvm::Function::Create(
    mallocFT,
    llvm::Function::ExternalLinkage,
    "__malloc",
    *module
  );

  llvm::Argument *sizeArg = mallocFN->arg_begin();
  sizeArg->setName("size");

  llvm::BasicBlock *entryBlock = llvm::BasicBlock::Create(
    *context, "entry", mallocFN
  );
  builder->SetInsertPoint(entryBlock);

  // Declare the standard malloc function: void* malloc(size_t size).
  llvm::FunctionType *std_mallocFT = llvm::FunctionType::get(
    llvm::Type::getInt8Ty(*context)->getPointerTo(),
    { llvm::Type::getInt64Ty(*context) },
    false
  );
  llvm::FunctionCallee mallocCallee = module->getOrInsertFunction("malloc", std_mallocFT);
  llvm::Value *mallocCall = builder->CreateCall(mallocCallee, { sizeArg });
  builder->CreateRet(mallocCall);

  this->fTable["std_memory"]["malloc"] = mallocFN;
}

void Codegen::addStdMEM_free() {
  llvm::FunctionType *freeFT = llvm::FunctionType::get(
    llvm::Type::getVoidTy(*context),
    { llvm::Type::getInt8Ty(*context)->getPointerTo() },
    false
  );

  llvm::Function *freeFN = llvm::Function::Create(
    freeFT,
    llvm::Function::ExternalLinkage,
    "__free",
    *module
  );

  llvm::Argument *ptrArg = freeFN->arg_begin();
  ptrArg->setName("ptr");

  // Create a basic block and set the insertion point
  llvm::BasicBlock *entryBlock = llvm::BasicBlock::Create(
    *context, "entry", freeFN
  );
  builder->SetInsertPoint(entryBlock);

  // Declare the standard free function: void free(void* ptr).
  llvm::FunctionType *std_freeFT = llvm::FunctionType::get(
      llvm::Type::getVoidTy(*context),
      { llvm::Type::getInt8Ty(*context)->getPointerTo() },
      false
  );

  llvm::FunctionCallee freeCallee = module->getOrInsertFunction("free", std_freeFT);
  builder->CreateCall(freeCallee, { ptrArg });
  builder->CreateRetVoid();

  this->fTable["std_memory"]["free"] = freeFN;
}

llvm::AllocaInst *Codegen::createAlloca(llvm::Function *fn, 
                                        const std::string &var, llvm::Type *T) {
  llvm::IRBuilder<> tmp(&fn->getEntryBlock(), 
      fn->getEntryBlock().begin());
  return tmp.CreateAlloca(T, nullptr, var);
}

void Codegen::createStructMappings(PackageUnitDecl *pkg) {
  for (Decl *decl : pkg->decls) {
    if (StructDecl *SD = dynamic_cast<StructDecl *>(decl)) {
      // Create the struct type and struct in the module.
      llvm::Type *T = SD->getType()->toLLVMType(*context);
      llvm::StructType *ST = llvm::cast<llvm::StructType>(T);
      if (!ST)
        fatal("invalid struct type: " + SD->getName(), decl->getStartLoc());

      ST->setName('_' + instance + SD->getParent()->getIdentifier() + '_' 
          + SD->getName());

      // Add the struct to the struct table.
      sTable[SD->getParent()->getIdentifier()][SD->getName()] = ST;
    }
  }
}

void Codegen::createFunctionMappings(PackageUnitDecl *pkg) {
  for (Decl *decl : pkg->decls) {
    if (FunctionDecl *FD = dynamic_cast<FunctionDecl *>(decl)) {
      // Create a function identifier dependent on its package.
      const std::string uid = FD->isMain() ? FD->getName() : '_' + instance 
          + FD->getParent()->getIdentifier() + '_' + FD->getName();

      // Create the function type and function in the module.
      const FunctionType *AFT = dynamic_cast<const FunctionType *>(FD->getType());
      assert(AFT && "expected function type for function declaration.");
      llvm::Type *RT = getLLVMType(AFT->getReturnType());
      std::vector<llvm::Type *> argTys;
      for (const Type *T : AFT->getParamTypes())
        argTys.push_back(getLLVMType(T));

      llvm::FunctionType *FT = llvm::FunctionType::get(RT, argTys, false);
      llvm::Function *FN = llvm::Function::Create(
        FT, 
        llvm::Function::ExternalLinkage, 
        uid,
        module.get()
      );

      // Set the function arguments names.
      unsigned idx = 0;
      for (llvm::Argument &arg : FN->args())
        arg.setName(FD->getParam(idx++)->getName());

      // Add the function to the function table.
      fTable[FD->getParent()->getIdentifier()][FD->getName()] = FN;
    }
  }
}

llvm::StructType *Codegen::getStruct(PackageUnitDecl *p, 
                                     const std::string &name) const {
  return sTable.at(p->getIdentifier()).at(name);
}

llvm::Function *Codegen::getFunction(PackageUnitDecl *p, 
                                     const std::string &name) const {
  return fTable.at(p->getIdentifier()).at(name);
}

llvm::Type *Codegen::getLLVMType(const Type *Ty) const {
  if (Ty->isStructType()) {
    const StructType *ST = dynamic_cast<const StructType *>(Ty);
    if (!ST)
      fatal("expected struct type for struct type lookup");

    return getStruct(ST->getDecl()->getParent(), ST->toString());
  }

  return Ty->toLLVMType(*context);
}

void Codegen::visit(PackageUnitDecl *decl) {
  this->currPkg = decl;
  for (Decl *d : decl->decls)
    d->pass(this);
  
  this->currPkg = nullptr;
}

void Codegen::visit(ImportDecl *decl) { /* no work to be done */ }

void Codegen::visit(FunctionDecl *decl) {
  llvm::Function *FN = fTable[decl->getParent()->getIdentifier()][decl->name];
  if (!FN)
    fatal("function not found in ftable: " + decl->name, decl->getStartLoc());

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
  assert(FN && "function must be set before generating code for parameters.");
  if (!llvm::isa<llvm::Argument>(tmp)) {
    fatal("expected an argument value for parameter declaration.",
        decl->getStartLoc());
  }

  // Create an alloca for the parameter in the entry block.
  llvm::AllocaInst *alloca = createAlloca(FN, tmp->getName().str(),
      tmp->getType());
  builder->CreateStore(tmp, alloca);
  allocas[tmp->getName().str()] = alloca;
}

void Codegen::visit(VarDecl *decl) {
  decl->init->pass(this);

  llvm::Type *Ty = getLLVMType(decl->getType());
  llvm::AllocaInst *alloca = createAlloca(
    builder->GetInsertBlock()->getParent(), 
    decl->getName(),
    Ty
  );

  builder->CreateStore(tmp, alloca);
  allocas[decl->getName()] = alloca;
}

void Codegen::visit(EnumDecl *decl) { /* no work to be done */ }
void Codegen::visit(FieldDecl *decl) { /* no work to be done */ }
void Codegen::visit(StructDecl *decl) { /* no work to be done */ }

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
  // Handle enum references.
  if (expr->hasSpecifier()) {
    const EnumType *ET = dynamic_cast<const EnumType *>(expr->getType());
    if (!ET)
      fatal("expected enum type for enum reference", expr->getStartLoc());

    tmp = llvm::ConstantInt::get(ET->toLLVMType(*context), 
        ET->getVariant(expr->getSpecifier()));
    return;
  }

  llvm::AllocaInst *alloca = allocas[expr->ident];
  if (!alloca)
    fatal("unresolved variable: " + expr->ident, expr->getStartLoc());

  if (needPtr) {
    tmp = alloca;
    return;
  }

  tmp = builder->CreateLoad(getLLVMType(expr->getType()), alloca);
}

void Codegen::visit(CallExpr *expr) {
  llvm::Function *callee = fTable[expr->decl->getParent()->getIdentifier()]
      [expr->ident];
  if (!callee) {
    fatal("function not found in function table: " + expr->ident, 
        expr->getStartLoc());
  }

  std::vector<llvm::Value *> args;
  for (std::size_t i = 0; i < expr->getNumArgs(); ++i) {
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
  case UnaryExpr::UnaryOp::Ref: // tmp is already the desired pointer.
    break;
  case UnaryExpr::UnaryOp::DeRef:
    if (!needPtr)
      tmp = builder->CreateLoad(expr->getType()->toLLVMType(*context), 
          base);
    break;
  default: 
    fatal("unknown unary operator", expr->getStartLoc());
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
  case BinaryExpr::BinaryOp::Assign:
    tmp = builder->CreateStore(rhs, lhs);
    break;
  case BinaryExpr::BinaryOp::AddAssign: {
    llvm::Value *addVal = nullptr;
    if (expr->T->isFloatingPointType())
      addVal = builder->CreateFAdd(lhs, rhs);
    else
      addVal = builder->CreateAdd(lhs, rhs);

    // Get the pointer for the left hand side.
    needPtr = true;
    expr->lhs->pass(this);
    tmp = builder->CreateStore(addVal, tmp);
    break;
  }
  case BinaryExpr::BinaryOp::SubAssign: {
    llvm::Value *subVal = nullptr;
    if (expr->T->isFloatingPointType())
      subVal = builder->CreateFSub(lhs, rhs);
    else
      subVal = builder->CreateSub(lhs, rhs);

    // Get the pointer for the left hand side.
    needPtr = true;
    expr->lhs->pass(this);
    tmp = builder->CreateStore(subVal, tmp);
    break;
  }
  case BinaryExpr::BinaryOp::MultAssign: {
    llvm::Value *multVal = nullptr;
    if (expr->T->isFloatingPointType())
      multVal = builder->CreateFMul(lhs, rhs);
    else
      multVal = builder->CreateMul(lhs, rhs);

    // Get the pointer for the left hand side.
    needPtr = true;
    expr->lhs->pass(this);
    tmp = builder->CreateStore(multVal, tmp);
    break;
  }
  case BinaryExpr::BinaryOp::DivAssign: {
    llvm::Value *divVal = nullptr;
    if (expr->T->isFloatingPointType())
      divVal = builder->CreateFDiv(lhs, rhs);
    else
      divVal = builder->CreateSDiv(lhs, rhs);

    // Get the pointer for the left hand side.
    needPtr = true;
    expr->lhs->pass(this);
    tmp = builder->CreateStore(divVal, tmp);
    break;
  }
  case BinaryExpr::BinaryOp::Equals:
    if (expr->T->isFloatingPointType())
      tmp = builder->CreateFCmpOEQ(lhs, rhs);
    else
      tmp = builder->CreateICmpEQ(lhs, rhs);
    break;
  case BinaryExpr::BinaryOp::NotEquals:
    if (expr->T->isFloatingPointType())
      tmp = builder->CreateFCmpONE(lhs, rhs);
    else
      tmp = builder->CreateICmpNE(lhs, rhs);
    break;
  case BinaryExpr::BinaryOp::LessThan:
    if (expr->T->isFloatingPointType())
      tmp = builder->CreateFCmpOLT(lhs, rhs);
    else
      tmp = builder->CreateICmpSLT(lhs, rhs);
    break;
  case BinaryExpr::BinaryOp::GreaterThan:
    if (expr->T->isFloatingPointType())
      tmp = builder->CreateFCmpOGT(lhs, rhs);
    else
      tmp = builder->CreateICmpSGT(lhs, rhs);
    break;
  case BinaryExpr::BinaryOp::LessEquals:
    if (expr->T->isFloatingPointType())
      tmp = builder->CreateFCmpOLE(lhs, rhs);
    else
      tmp = builder->CreateICmpSLE(lhs, rhs);
    break;
  case BinaryExpr::BinaryOp::GreaterEquals:
    if (expr->T->isFloatingPointType())
      tmp = builder->CreateFCmpOGE(lhs, rhs);
    else
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
    if (expr->T->isFloatingPointType())
      tmp = builder->CreateFAdd(lhs, rhs);
    else
      tmp = builder->CreateAdd(lhs, rhs);
    break;
  case BinaryExpr::BinaryOp::Sub:
    if (expr->T->isFloatingPointType())
      tmp = builder->CreateFSub(lhs, rhs);
    else
      tmp = builder->CreateSub(lhs, rhs);
    break;
  case BinaryExpr::BinaryOp::Mult:
    if (expr->T->isFloatingPointType())
      tmp = builder->CreateFMul(lhs, rhs);
    else
      tmp = builder->CreateMul(lhs, rhs);
    break;
  case BinaryExpr::BinaryOp::Div:
    if (expr->T->isFloatingPointType())
      tmp = builder->CreateFDiv(lhs, rhs);
    else
      tmp = builder->CreateSDiv(lhs, rhs);
    break;
  default: 
    fatal("unknown binary operator", expr->getStartLoc());
  } // end switch

  needPtr = false;
}

void Codegen::visit(BooleanLiteral *expr)
{ tmp = llvm::ConstantInt::get(*context, llvm::APInt(1, expr->value, true)); }

void Codegen::visit(IntegerLiteral *expr) {
  /// TODO: Type check for unsigned integers.
  tmp = llvm::ConstantInt::get(*context, llvm::APInt(
      expr->T->getBitWidth(), expr->value, true));
}

void Codegen::visit(FPLiteral *expr) 
{ tmp = llvm::ConstantFP::get(*context, llvm::APFloat(expr->value)); }

void Codegen::visit(CharLiteral *expr) 
{ tmp = llvm::ConstantInt::get(*context, llvm::APInt(8, expr->value, true)); }

void Codegen::visit(StringLiteral *expr) 
{ 
  tmp = builder->CreateGlobalStringPtr(expr->getValue());
}

void Codegen::visit(NullExpr *expr) {
  tmp = llvm::ConstantPointerNull::get(llvm::PointerType::get(
      expr->T->toLLVMType(*context), 0));
}

void Codegen::visit(ArrayExpr *expr) {
  llvm::Type *T = expr->T->toLLVMType(*context);
  llvm::Value *array = llvm::UndefValue::get(T);

  for (std::size_t i = 0; i < expr->getNumExprs(); ++i) {
    expr->getExpr(i)->pass(this);
    array = builder->CreateInsertValue(array, tmp, i);
  }

  tmp = array;
}

void Codegen::visit(ArraySubscriptExpr *expr) {
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
    std::vector<llvm::Value *> indices = { index };
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

  fatal("unknown array access expression type", expr->getStartLoc());
}

void Codegen::visit(StructInitExpr *expr) {
  llvm::StructType *ST = sTable[expr->decl->getParent()->getIdentifier()]
      [expr->getName()];
  llvm::Value *structVal = llvm::UndefValue::get(ST);

  for (std::size_t i = 0; i < expr->getNumFields(); ++i) {
    expr->getField(i)->pass(this);
    structVal = builder->CreateInsertValue(structVal, tmp, i);
  }

  tmp = structVal;
}

void Codegen::visit(MemberExpr *expr) {
  unsigned oldNeedPtr = this->needPtr;
  this->needPtr = true;
  expr->base->pass(this);
  llvm::Value *basePtr = tmp;
  this->needPtr = oldNeedPtr;
  
  const DeclRefExpr *baseExpr = dynamic_cast<const DeclRefExpr *>(expr->base.get());
  assert(baseExpr && "expected decl ref expression for member access base");

  const StructType *AST = dynamic_cast<const StructType *>(baseExpr->getType());
  llvm::StructType *ST = sTable[AST->getDecl()->getParent()->getIdentifier()]
      [AST->toString()];
  assert(ST && "expected struct type for member access expression");

  int idx = expr->index;
  if (idx == -1) {
    fatal("member " + expr->getMember() + " undeclared in base struct",
        expr->getStartLoc());
  }

  // Create a GEP to access the member by the struct pointer.
  llvm::Value *memberPtr = builder->CreateStructGEP(ST, basePtr, idx);

  // Return the pointer to the member if it's needed.
  if (needPtr) {
    tmp = memberPtr;
    return;
  }

  // Otherwise, return the loaded value of the member.
  tmp = builder->CreateLoad(expr->getType()->toLLVMType(*context), memberPtr);
}

void Codegen::visit(BreakStmt *stmt) {
  if (!builder->GetInsertBlock()->getTerminator())
    builder->CreateBr(hostMergeBlock);
}

void Codegen::visit(ContinueStmt *stmt) {
  if (!builder->GetInsertBlock()->getTerminator())
    builder->CreateBr(hostCondBlock);
}

void Codegen::visit(CompoundStmt *stmt) {
  for (const std::unique_ptr<Stmt> &s : stmt->stmts)
    s->pass(this);
}

void Codegen::visit(DeclStmt *stmt) 
{ stmt->decl->pass(this); }

void Codegen::visit(IfStmt *stmt) {
  // Fetch the value for the if statement condition.
  stmt->cond->pass(this);
  llvm::Value *condVal = tmp;
  if (!condVal)
    fatal("expected expression in if statement condition", stmt->getStartLoc());

  // Initialize basic blocks for this `if` control flow.
  llvm::BasicBlock *thenBlock = llvm::BasicBlock::Create(*context, 
      "then", FN);
  llvm::BasicBlock *elseBlock = llvm::BasicBlock::Create(*context,
      "else");
  llvm::BasicBlock *mergeBlock = llvm::BasicBlock::Create(*context,
      "merge");

  // Create a branch from the the then block to the else block, should it exist.
  if (stmt->hasElse())
    builder->CreateCondBr(condVal, thenBlock, elseBlock);
  else // No else block, go straight to merge after the 'then' statement.
    builder->CreateCondBr(condVal, thenBlock, mergeBlock);

  // Codegen pass on the then body, and insert instructions to its basic block.
  builder->SetInsertPoint(thenBlock);
  stmt->thenStmt->pass(this);
  llvm::Value *thenVal = tmp;
  assert(thenVal && "expected value for then statement");

  // If no terminator was made, branch to the merge block.
  thenBlock = builder->GetInsertBlock();
  if (!thenBlock->getTerminator())
    builder->CreateBr(mergeBlock);

  // Codegen pass on the else statement, if it exists.
  builder->SetInsertPoint(elseBlock);
  llvm::Value *elseVal = nullptr;
  if (stmt->hasElse()) {
    FN->insert(FN->end(), elseBlock);
    stmt->elseStmt->pass(this);
    elseBlock = builder->GetInsertBlock();
  }

  // If no terminator was made, branch to the merge block.
  if (!stmt->hasElse() || !elseBlock->getTerminator())
    builder->CreateBr(mergeBlock);

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

  // Forward the condition and merge blocks.
  llvm::BasicBlock *prevCondBlock = this->hostCondBlock;
  llvm::BasicBlock *prevMergeBlock = this->hostMergeBlock;
  this->hostCondBlock = condBlock;
  this->hostMergeBlock = mergeBlock;

  // Create a branch to the conditional evaluation block.
  builder->CreateBr(condBlock);
  builder->SetInsertPoint(condBlock);

  // Codegen pass on the loop condition.
  stmt->cond->pass(this);
  llvm::Value *condVal = tmp;
  if (!condVal)
    fatal("expected expression in while statement condition", stmt->getStartLoc());

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

  // Reset the condition and merge blocks.
  this->hostCondBlock = prevCondBlock;
  this->hostMergeBlock = prevMergeBlock;
}

void Codegen::visit(UntilStmt *stmt) {
  // Initialize basic blocks for the while control flow.
  llvm::BasicBlock *condBlock = llvm::BasicBlock::Create(*context, "cond", FN);
  llvm::BasicBlock *loopBlock = llvm::BasicBlock::Create(*context, "until");
  llvm::BasicBlock *mergeBlock = llvm::BasicBlock::Create(*context, "merge");

  // Forward the condition and merge blocks.
  llvm::BasicBlock *prevCondBlock = this->hostCondBlock;
  llvm::BasicBlock *prevMergeBlock = this->hostMergeBlock;
  this->hostCondBlock = condBlock;
  this->hostMergeBlock = mergeBlock;

  // Create a branch to the conditional evaluation block.
  builder->CreateBr(condBlock);
  builder->SetInsertPoint(condBlock);

  // Codegen pass on the loop condition.
  stmt->cond->pass(this);
  llvm::Value *condVal = tmp;
  if (!condVal)
    fatal("expected expression in while statement condition", 
        stmt->getStartLoc());

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

  // Reset the condition and merge blocks.
  this->hostCondBlock = prevCondBlock;
  this->hostMergeBlock = prevMergeBlock;
}

void Codegen::visit(CaseStmt *stmt) 
{ stmt->body->pass(this); }

void Codegen::visit(DefaultStmt *stmt)
{ stmt->body->pass(this); }

void Codegen::visit(MatchStmt *stmt) {
  // Fetch the value for the match statement expression.
  stmt->expr->pass(this);
  llvm::Value *matchVal = tmp;
  if (!matchVal)
    fatal("expected expression in match statement", stmt->getStartLoc());

  // Initialize basic blocks for the match control flow.
  llvm::BasicBlock *mergeBlock = llvm::BasicBlock::Create(*context, 
      "merge");
  llvm::BasicBlock *defaultBlock = llvm::BasicBlock::Create(*context, 
      "default");

  llvm::SwitchInst *swInst = nullptr;
  if (stmt->hasDefault())
    swInst = builder->CreateSwitch(matchVal, defaultBlock, 
        stmt->cases.size() - 1);
  else
    swInst = builder->CreateSwitch(matchVal, mergeBlock, 
        stmt->cases.size() - 1);

  // Codegen each of the match cases.
  for (std::unique_ptr<MatchCase> &c : stmt->cases) {
    // Only codegen on case statements for now.
    CaseStmt *caseStmt = dynamic_cast<CaseStmt *>(c.get());
    if (!caseStmt)
      continue;

    // Initialize the basic block for the case statement body.
    llvm::BasicBlock *caseBlock = llvm::BasicBlock::Create(*context, "case", FN);

    // Codegen pass on the case statement expression.
    caseStmt->expr->pass(this);
    if (!tmp)
      fatal("expected expression in case statement", stmt->getStartLoc());

    // Evaluate the expression as an integer constant.
    llvm::ConstantInt *caseVal = llvm::dyn_cast<llvm::ConstantInt>(tmp);
    if (!caseVal)
      fatal("expected integer evaluable expression in case statement", 
          stmt->getStartLoc());

    // Add the case to the switch instruction, and codegen the body of the case.
    swInst->addCase(caseVal, caseBlock);
    builder->SetInsertPoint(caseBlock);
    caseStmt->pass(this);

    // Branch to the merge block if the case body has no terminator.
    if (!caseBlock->getTerminator())
      builder->CreateBr(mergeBlock);
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

void Codegen::visit(RetStmt *stmt) {
  stmt->expr->pass(this);
  builder->CreateRet(tmp);
}
