//>==- Codegen.h ----------------------------------------------------------==<//
//
// This header file declares the main LLVM lowering pass over a semantically 
// valid abstract syntax tree.
//
//>==----------------------------------------------------------------------==<//

#ifndef ARTUS_CODEGEN_CODEGEN_H
#define ARTUS_CODEGEN_CODEGEN_H

#include <unordered_map>

#include "llvm/IR/Instruction.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Target/TargetMachine.h"

#include "../AST/ASTVisitor.h"
#include "../Core/Context.h"

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

  /// Table of functions by package and then identifier.
  std::unordered_map<std::string, 
      std::unordered_map<std::string, llvm::Function *>> fTable;

  /// Table of structs by package and then identifier.
  std::unordered_map<std::string,
      std::unordered_map<std::string, llvm::StructType *>> sTable;

  /// Table of allocas in scope.
  std::unordered_map<std::string, llvm::AllocaInst *> allocas;

  /// The current value being generated.
  llvm::Value *tmp;

  /// The current function being generated.
  llvm::Function *FN;

  /// The current target condition block, if it exists. This is used to insert
  /// branches for control flow statements found in nested bodies. (i.e. loops).
  llvm::BasicBlock *hostCondBlock;

  /// The current target merge block, if it exists. This is used to insert
  /// branches for control flow statements found in nested bodies. (i.e. loops).
  llvm::BasicBlock *hostMergeBlock;

  /// Flag used to indicate if a reference is needed by pointer or by value.
  bool needPtr : 1;

  /// The name of the current compilation instance.
  const std::string instance;

  /// The active package unit.
  PackageUnitDecl *currPkg;

  /// Creates an alloca instruction for a variable \p var in the entry block of 
  /// the function \p fn.
  [[nodiscard]] 
  llvm::AllocaInst *createAlloca(llvm::Function *fn, const std::string &var, 
                                 llvm::Type *T);

  /// Creates struct mappings for package \p pkg.
  void createStructMappings(PackageUnitDecl *pkg);

  /// Creates function mappings for package \p pkg.
  void createFunctionMappings(PackageUnitDecl *pkg);

  /// \returns The struct with name \p name defined in package \p p.
  llvm::StructType *getStruct(PackageUnitDecl *p, const std::string &name) const;

  /// \returns The function with name \p name defined in package \p p.
  llvm::Function *getFunction(PackageUnitDecl *p, const std::string &name) const;

  /// \returns The corresponding LLVM type for type \p ty.
  llvm::Type *getLLVMType(const Type *Ty) const;

public:
  Codegen(Context *ctx, const std::string &instance, llvm::TargetMachine *TM);
  ~Codegen();
  
  /// \returns The module generated by this pass.
  llvm::Module *getModule() { return module.get(); }

  void genericCastCGN(CastExpr *expr);

  void visit(PackageUnitDecl *decl) override;
  void visit(ImportDecl *decl) override;
  void visit(FunctionDecl *decl) override;
  void visit(ParamVarDecl *decl) override;
  void visit(VarDecl *decl) override;
  void visit(EnumDecl *decl) override;
  void visit(FieldDecl *decl) override;
  void visit(StructDecl *decl) override;

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
  void visit(ArrayExpr *expr) override;
  void visit(ArraySubscriptExpr *expr) override;
  void visit(StructInitExpr *expr) override;
  void visit(MemberExpr *expr) override;

  void visit(BreakStmt *stmt) override;
  void visit(ContinueStmt *stmt) override;
  void visit(CompoundStmt *stmt) override;
  void visit(DeclStmt *stmt) override;
  void visit(IfStmt *stmt) override;
  void visit(WhileStmt *stmt) override;
  void visit(UntilStmt *stmt) override;
  void visit(CaseStmt *stmt) override;
  void visit(DefaultStmt *stmt) override;
  void visit(MatchStmt *stmt) override;
  void visit(RetStmt *stmt) override;
};

} // end namespace artus

#endif // ARTUS_CODEGEN_CODEGEN_H
