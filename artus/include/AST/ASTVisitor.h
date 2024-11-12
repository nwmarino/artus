#ifndef ARTUS_AST_ASTVISITOR_H
#define ARTUS_AST_ASTVISITOR_H

namespace artus {

/// Forward declarations.
class PackageUnitDecl;
class FunctionDecl;
class ParamVarDecl;
class LabelDecl;

class ImplicitCastExpr;
class ExplicitCastExpr;
class IntegerLiteral;

class CompoundStmt;
class LabelStmt;
class RetStmt;

/// This class defines a visitor pattern interface to traverse a built AST.
class ASTVisitor {
public:
  virtual ~ASTVisitor() = default;

  virtual void visit(PackageUnitDecl *decl) = 0;
  virtual void visit(FunctionDecl *decl) = 0;
  virtual void visit(ParamVarDecl *decl) = 0;
  virtual void visit(LabelDecl *decl) = 0;

  virtual void visit(ImplicitCastExpr *expr) = 0;
  virtual void visit(ExplicitCastExpr *expr) = 0;
  virtual void visit(IntegerLiteral *expr) = 0;

  virtual void visit(CompoundStmt *stmt) = 0;
  virtual void visit(LabelStmt *stmt) = 0;
  virtual void visit(RetStmt *stmt) = 0;
};

} // namespace artus

#endif // ARTUS_AST_ASTVISITOR_H
