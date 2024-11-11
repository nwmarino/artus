#ifndef ARTUS_CODEGEN_CODEGEN_H
#define ARTUS_CODEGEN_CODEGEN_H

#include "llvm/IR/Instruction.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Target/TargetMachine.h"

#include "../AST/ASTVisitor.h"
#include "../Core/Context.h"

using std::map;
using std::string;

namespace artus {

/// This class implements a Code Generation pass over a semantically valid AST. 
class Codegen : public ASTVisitor {
  /// The LLVM module to generate code into.
  std::unique_ptr<llvm::Module> module;

  /// The LLVM IR builder to generate instructions.
  std::unique_ptr<llvm::IRBuilder<>> builder;

  /// The LLVM context to generate code in.
  std::unique_ptr<llvm::LLVMContext> context;

  /// A map of all functions and allocas in the current context.
  map<string, llvm::Function *> functions;
  map<string, llvm::AllocaInst *> allocas;

  /// The current value being generated.
  llvm::Value *tmp;

  /// The current function being generated.
  llvm::Function *FN;

  /// Creates an alloca instruction in the entry block of the function.
  [[nodiscard]] 
  llvm::AllocaInst *createAlloca(llvm::Function *fn, const std::string &var, 
                                llvm::Type *T);

public:
  Codegen(Context *ctx, llvm::TargetMachine *TM);
  ~Codegen();
  
  llvm::Module *getModule() { return module.get(); }

  void visit(PackageUnitDecl *decl) override;
  void visit(FunctionDecl *decl) override;
  void visit(ParamVarDecl *decl) override;
  void visit(LabelDecl *decl) override;

  void visit(IntegerLiteral *expr) override;

  void visit(CompoundStmt *stmt) override;
  void visit(LabelStmt *stmt) override;
  void visit(RetStmt *stmt) override;
};

} // namespace artus

#endif // ARTUS_CODEGEN_CODEGEN_H
