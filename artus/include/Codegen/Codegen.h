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

/// Forward declarations.
class CastExpr;

/// This class implements a Code Generation pass over a semantically valid AST. 
class Codegen final : public ASTVisitor {
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

  /// Flag used to indicate if a reference is needed by pointer or by value.
  bool needPtr : 1 = 0;

  /// Creates an alloca instruction in the entry block of the function.
  [[nodiscard]] 
  llvm::AllocaInst *createAlloca(llvm::Function *fn, const std::string &var, 
                                llvm::Type *T);

public:
  Codegen(Context *ctx, llvm::TargetMachine *TM);
  ~Codegen();
  
  llvm::Module *getModule() { return module.get(); }
  void genericCastCGN(CastExpr *expr);

  void visit(PackageUnitDecl *decl) override;
  void visit(FunctionDecl *decl) override;
  void visit(ParamVarDecl *decl) override;
  void visit(LabelDecl *decl) override;
  void visit(VarDecl *decl) override;

  void visit(ImplicitCastExpr *expr) override;
  void visit(ExplicitCastExpr *expr) override;
  void visit(DeclRefExpr *expr) override;
  void visit(CallExpr *expr) override;
  void visit(UnaryExpr *expr) override;
  void visit(BinaryExpr *expr) override;
  void visit(BooleanLiteral *expr) override;
  void visit(IntegerLiteral *expr) override;
  void visit(FPLiteral *expr) override;
  void visit(CharLiteral *expr) override;
  void visit(StringLiteral *expr) override;
  void visit(NullExpr *expr) override;
  void visit(ArrayInitExpr *expr) override;
  void visit(ArrayAccessExpr *expr) override;

  void visit(CompoundStmt *stmt) override;
  void visit(DeclStmt *stmt) override;
  void visit(IfStmt *stmt) override;
  void visit(WhileStmt *stmt) override;
  void visit(UntilStmt *stmt) override;
  void visit(CaseStmt *stmt) override;
  void visit(DefaultStmt *stmt) override;
  void visit(MatchStmt *stmt) override;
  void visit(LabelStmt *stmt) override;
  void visit(JmpStmt *stmt) override;
  void visit(RetStmt *stmt) override;
};

} // namespace artus

#endif // ARTUS_CODEGEN_CODEGEN_H
