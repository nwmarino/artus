#ifndef ARTUS_AST_VISITOR_H
#define ARTUS_AST_VISITOR_H

namespace artus {

/// Forward declarations.
class PackageUnitDecl;
class FunctionDecl;
class ParamVarDecl;
class LabelDecl;
class IntegerLiteral;
class CompoundStmt;
class LabelStmt;
class RetStmt;

/// This class defines a visitor pattern interface to traverse a built AST.
class ASTVisitor {
  friend class PackageUnitDecl;
  friend class FunctionDecl;
  friend class ParamVarDecl;
  friend class LabelDecl;
  friend class IntegerLiteral;
  friend class CompoundStmt;
  friend class LabelStmt;
  friend class RetStmt;

protected:
  virtual void visit(PackageUnitDecl *decl) = 0;
  virtual void visit(FunctionDecl *decl) = 0;
  virtual void visit(ParamVarDecl *decl) = 0;
  virtual void visit(LabelDecl *decl) = 0;

  virtual void visit(IntegerLiteral *expr) = 0;

  virtual void visit(CompoundStmt *stmt) = 0;
  virtual void visit(LabelStmt *stmt) = 0;
  virtual void visit(RetStmt *stmt) = 0;

public:
  virtual ~ASTVisitor() = default;
};

} // namespace artus

#endif // ARTUS_AST_VISITOR_H
