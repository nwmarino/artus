#ifndef ARTUS_AST_ASTVISITOR_H
#define ARTUS_AST_ASTVISITOR_H

namespace artus {

/// Forward declarations.
class PackageUnitDecl;
class FunctionDecl;
class ParamVarDecl;
class LabelDecl;
class VarDecl;

class ImplicitCastExpr;
class ExplicitCastExpr;
class DeclRefExpr;
class UnaryExpr;
class BinaryExpr;
class IntegerLiteral;

class CompoundStmt;
class DeclStmt;
class LabelStmt;
class JmpStmt;
class RetStmt;

/// This class defines a visitor pattern interface to traverse a built AST.
class ASTVisitor {
public:
  virtual ~ASTVisitor() = default;

  virtual void visit(PackageUnitDecl *decl) = 0;
  virtual void visit(FunctionDecl *decl) = 0;
  virtual void visit(ParamVarDecl *decl) = 0;
  virtual void visit(LabelDecl *decl) = 0;
  virtual void visit(VarDecl *decl) = 0;

  virtual void visit(ImplicitCastExpr *expr) = 0;
  virtual void visit(ExplicitCastExpr *expr) = 0;
  virtual void visit(DeclRefExpr *expr) = 0;
  virtual void visit(UnaryExpr *expr) = 0;
  virtual void visit(BinaryExpr *expr) = 0;
  virtual void visit(IntegerLiteral *expr) = 0;

  virtual void visit(CompoundStmt *stmt) = 0;
  virtual void visit(DeclStmt *stmt) = 0;
  virtual void visit(LabelStmt *stmt) = 0;
  virtual void visit(JmpStmt *stmt) = 0;
  virtual void visit(RetStmt *stmt) = 0;
};

} // namespace artus

#endif // ARTUS_AST_ASTVISITOR_H
