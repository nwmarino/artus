#ifndef ARTUS_SEMA_SEMA_H
#define ARTUS_SEMA_SEMA_H

#include "../AST/ASTVisitor.h"
#include "../AST/Decl.h"
#include "../AST/Expr.h"
#include "../Core/Context.h"
#include "Scope.h"

using std::size_t;

namespace artus {

/// This class implements a Semantic Analysis pass over an AST. The checks
/// involved include name resolution, type checking, and control flow analysis.
class Sema final : public ASTVisitor {
  /// The context associated with the semantic analysis pass.
  Context *ctx;

  /// The possible kinds of loop that the visitor can be traversing.
  enum class LoopKind {
    /// No loop.
    NOL = 0,
  };

  /// Flag to indicate if the main function has been found.
  unsigned hasMain : 1;

  /// Flag to indicate if the visitor is currently traversing a function.
  unsigned inFunction : 1;

  /// Flag to indicate if the visitor is currently traversing a loop.
  unsigned inLoop : 1;
  LoopKind loopKind = LoopKind::NOL;

  /// Relevant scope quantifiers.
  Scope *globalScope;
  Scope *localScope;

  /// The type of the top-level function, if it exists.
  const FunctionType *parentFunctionType;

  /// The index of the current parameter being visited.
  size_t paramIndex;

public:
  Sema(Context *ctx);

  void visit(PackageUnitDecl *decl) override;
  void visit(FunctionDecl *decl) override;
  void visit(ParamVarDecl *decl) override;
  void visit(LabelDecl *decl) override;

  void visit(ImplicitCastExpr *expr) override;
  void visit(ExplicitCastExpr *expr) override;
  void visit(IntegerLiteral *expr) override;

  void visit(CompoundStmt *stmt) override;
  void visit(LabelStmt *stmt) override;
  void visit(RetStmt *stmt) override;
};

} // namespace artus

#endif // ARTUS_SEMA_SEMA_H
