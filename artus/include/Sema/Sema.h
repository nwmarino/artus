#ifndef ARTUS_SEMA_H
#define ARTUS_SEMA_H

#include "../AST/ASTVisitor.h"
#include "Scope.h"

namespace artus {

/// This class implements a Semantic Analysis pass over an AST. The checks
/// involved include name resolution, type checking, and control flow analysis.
class Sema final : public ASTVisitor {
  friend class PackageUnitDecl;
  friend class FunctionDecl;
  friend class LabelDecl;
  friend class IntegerLiteral;
  friend class CompoundStmt;
  friend class LabelStmt;
  friend class RetStmt;

  /// The possible kinds of loop that the visitor can be traversing.
  enum class LoopKind {
    /// No loop.
    NOL = 0,
  };

  /// Flag to indicate if the main function has been found.
  unsigned hasMain : 1;

  /// Flag to indicate if the visitor is currently traversing a loop.
  unsigned inLoop : 1;
  LoopKind loopKind = LoopKind::NOL;

  /// Relevant scope quantifiers.
  Scope *globalScope;
  Scope *currentScope;

  /// The return type of the top-level function, if it exists.
  const Type *returnType;

  void visit(PackageUnitDecl *decl) override;
  void visit(FunctionDecl *decl) override;
  void visit(LabelDecl *decl) override;
  void visit(IntegerLiteral *expr) override;
  void visit(CompoundStmt *stmt) override;
  void visit(LabelStmt *stmt) override;
  void visit(RetStmt *stmt) override;
  
public:
  Sema();
};

} // namespace artus

#endif // ARTUS_SEMA_H
