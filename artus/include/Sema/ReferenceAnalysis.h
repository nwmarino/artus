#ifndef ARTUS_SEMA_REFERENCEANALYSIS_H
#define ARTUS_SEMA_REFERENCEANALYSIS_H

#include "../AST/ASTVisitor.h"
#include "../Core/Context.h"

using std::string;

namespace artus {

/// This class implements a reference analysis pass over an AST. The checks
/// involved include name and type resolution.
class ReferenceAnalysis final : public ASTVisitor {
  /// The context associated with the reference analysis pass.
  Context *ctx;

  /// Relevant scope quantifiers.
  Scope *globalScope;
  Scope *localScope;

  /// Resolves a name reference given its identifier. Raises a fatal error if an
  /// indentifier is unresolved. Otherwise, returns the declaration.
  const Decl *resolveReference(const string &ident, 
                               const SourceLocation &loc) const;

  /// Resolves a type given its identifier. Raises a fatal error if a type is
  /// unresolved, that is, the returned type would have been a `nullptr`.
  const Type *resolveType(const string &ident, const SourceLocation &loc) const;

public:
  ReferenceAnalysis(Context *ctx);
  ~ReferenceAnalysis() = default;

  void visit(PackageUnitDecl *decl) override;
  void visit(FunctionDecl *decl) override;
  void visit(ParamVarDecl *decl) override;
  void visit(VarDecl *decl) override;
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

#endif // ARTUS_SEMA_REFERENCEANALYSIS_H
