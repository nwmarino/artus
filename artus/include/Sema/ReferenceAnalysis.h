//>==- ReferenceAnalysis.h -----------------------------------------------<//
//
// This header file declares an AST pass that resolves name and type
// references throughout a parsed AST. The provided AST is not assumed
// semantically valid.
//
//>==--------------------------------------------------------------------==<//

#ifndef ARTUS_SEMA_REFERENCEANALYSIS_H
#define ARTUS_SEMA_REFERENCEANALYSIS_H

#include "../AST/ASTVisitor.h"
#include "../Core/Context.h"

namespace artus {

/// This class implements a reference analysis pass over an AST.
class ReferenceAnalysis final : public ASTVisitor {
  /// Non-owning context associated with the reference analysis pass.
  Context *ctx;

  /// Relevant scope quantifiers.
  Scope *globalScope;
  Scope *localScope;

  /// Resolves a declaration most similar to \p ident. If the declaration is
  /// unresolved, then a fatal error is raised at the location \p loc.
  /// \returns The declaration most similar to \p ident.
  const Decl *resolveReference(const std::string &ident, 
                               const SourceLocation &loc) const;
  
  /// Resolves a type most similar to \p ident. If the type is unresolved, then 
  /// a fatal error is raised at the location \p loc. 
  /// \returns The type most similar to \p ident.
  const Type *resolveType(const std::string &ident, const SourceLocation &loc) const;

  /// Recursively imports dependencies for a package unit declaration.
  void importDependencies(PackageUnitDecl *pkg);

public:
  ReferenceAnalysis(Context *ctx);
  ~ReferenceAnalysis() = default;

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

#endif // ARTUS_SEMA_REFERENCEANALYSIS_H
