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
    NOL = -1,

    /// While loops.
    WHILE,

    /// Until loops.
    UNTIL,
  };

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

  /// The type of the parent lvalue, if it exists.
  const Type *lvalueType;

  /// The index of the current parameter being visited.
  std::size_t paramIndex = 0;

  /// The most previous variable reference.
  std::string lastReference;

  /// Resolves a corresponding variable declaration for a given lvalue.
  const VarDecl *resolveReference(Expr *lvalue);

public:
  Sema(Context *ctx);
  ~Sema() = default;

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

} // namespace artus

#endif // ARTUS_SEMA_SEMA_H
