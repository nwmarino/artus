//>==- ASTVisitor.h -------------------------------------------------------==<//
//
// This header file declares a visitor pattern upon an abstract syntax tree.
//
//>==----------------------------------------------------------------------==<//

#ifndef ARTUS_AST_ASTVISITOR_H
#define ARTUS_AST_ASTVISITOR_H

namespace artus {

/// Forward declarations.
class PackageUnitDecl;
class ImportDecl;
class VarDecl;
class ParamVarDecl;
class FunctionDecl;
class EnumDecl;
class FieldDecl;
class StructDecl;

class ImplicitCastExpr;
class ExplicitCastExpr;
class DeclRefExpr;
class CallExpr;
class UnaryExpr;
class BinaryExpr;
class BooleanLiteral;
class IntegerLiteral;
class FPLiteral;
class CharLiteral;
class StringLiteral;
class NullExpr;
class ArrayExpr;
class ArraySubscriptExpr;
class StructInitExpr;
class MemberExpr;

class CompoundStmt;
class BreakStmt;
class ContinueStmt;
class DeclStmt;
class IfStmt;
class WhileStmt;
class UntilStmt;
class CaseStmt;
class DefaultStmt;
class MatchStmt;
class RetStmt;

/// This class defines a visitor pattern interface to traverse a built AST.
class ASTVisitor {
public:
  virtual ~ASTVisitor() = default;

  virtual void visit(PackageUnitDecl *decl) = 0;
  virtual void visit(ImportDecl *decl) = 0;
  virtual void visit(VarDecl *decl) = 0;
  virtual void visit(ParamVarDecl *decl) = 0;
  virtual void visit(FunctionDecl *decl) = 0;
  virtual void visit(EnumDecl *decl) = 0;
  virtual void visit(FieldDecl *decl) = 0;
  virtual void visit(StructDecl *decl) = 0;

  virtual void visit(ImplicitCastExpr *expr) = 0;
  virtual void visit(ExplicitCastExpr *expr) = 0;
  virtual void visit(DeclRefExpr *expr) = 0;
  virtual void visit(CallExpr *expr) = 0;
  virtual void visit(UnaryExpr *expr) = 0;
  virtual void visit(BinaryExpr *expr) = 0;
  virtual void visit(BooleanLiteral *expr) = 0;
  virtual void visit(IntegerLiteral *expr) = 0;
  virtual void visit(FPLiteral *expr) = 0;
  virtual void visit(CharLiteral *expr) = 0;
  virtual void visit(StringLiteral *expr) = 0;
  virtual void visit(NullExpr *expr) = 0;
  virtual void visit(ArrayExpr *expr) = 0;
  virtual void visit(ArraySubscriptExpr *expr) = 0;
  virtual void visit(StructInitExpr *expr) = 0;
  virtual void visit(MemberExpr *expr) = 0;

  virtual void visit(BreakStmt *stmt) = 0;
  virtual void visit(ContinueStmt *stmt) = 0;
  virtual void visit(CompoundStmt *stmt) = 0;
  virtual void visit(DeclStmt *stmt) = 0;
  virtual void visit(IfStmt *stmt) = 0;
  virtual void visit(WhileStmt *stmt) = 0;
  virtual void visit(UntilStmt *stmt) = 0;
  virtual void visit(CaseStmt *stmt) = 0;
  virtual void visit(DefaultStmt *stmt) = 0;
  virtual void visit(MatchStmt *stmt) = 0;
  virtual void visit(RetStmt *stmt) = 0;
};

} // end namespace artus

#endif // ARTUS_AST_ASTVISITOR_H
