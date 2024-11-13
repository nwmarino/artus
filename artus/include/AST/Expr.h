#ifndef ARTUS_AST_EXPR_H
#define ARTUS_AST_EXPR_H

#include "ASTPrinter.h"
#include "DeclBase.h"
#include "Stmt.h"
#include "../Core/Span.h"
#include "../Sema/Type.h"

namespace artus {

/// Base class for all Expression nodes. Expressions are also statements.
class Expr : public ValueStmt {
public:
  Expr(const Type *T, const Span &span) : ValueStmt(T, span) {}
};

/// Base class for all Cast Expressions. 
class CastExpr : public Expr {
protected:
  /// The expression to cast.
  std::unique_ptr<Expr> expr;

public:
  CastExpr(std::unique_ptr<Expr> expr, const Type *T, const Span &span)
      : Expr(T, span), expr(std::move(expr)) {}
};

/// An implicit cast. Usually injected by the ompiler during sema to recover
/// from type mismatches or undefined behaviour.
class ImplicitCastExpr final : public CastExpr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class Sema;

public:
  ImplicitCastExpr(std::unique_ptr<Expr> expr, const Type *T, const Span &span)
      : CastExpr(std::move(expr), T, span) {}

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }
};

/// An explicit cast. For example, `i64 0`.
class ExplicitCastExpr final : public CastExpr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class Sema;

public:
  ExplicitCastExpr(std::unique_ptr<Expr> expr, const Type *T, const Span &span)
      : CastExpr(std::move(expr), T, span) {}

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }
};

/// A reference to a declaration. For example, `x`.
class DeclRefExpr final : public Expr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class Sema;

  /// The identifier of the referenced declaration.
  const string ident;

  /// The declaration being referenced.
  const Decl *decl;

public:
  DeclRefExpr(const string ident, const Decl *decl, const Type *T, 
              const Span &span)
      : Expr(T, span), ident(ident), decl(decl) {}

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

  /// Returns the identifier of this reference.
  const string &getIdent() const { return ident; }
};

/// A unary expression. For example `-1`, `!true`, etc.
class UnaryExpr final : public Expr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class Sema;

public:
  /// Possible kinds of unary operations.
  enum class UnaryOp {
    Unknown = -1,
    Negative,
    Not,
  };

private:
  /// The base of the unary expression.
  std::unique_ptr<Expr> base;

  /// The operator of this unary expression.
  const UnaryOp op;

public:
  UnaryExpr(std::unique_ptr<Expr> base, UnaryOp op, const Span &span)
      : Expr(base->getType(), span), base(std::move(base)), op(op) {}

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }
};

/// A binary expression. For example, `1 + 1`, `2 * 3`, etc.
class BinaryExpr final : public Expr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class Sema;

public:
  /// Possible kinds of binary operations.
  enum class BinaryOp {
    Unknown = -1,
    Assign,
    Add,
    Sub,
    Mult,
    Div,
  };

private:
  /// The left-hand side of the binary expression.
  std::unique_ptr<Expr> lhs;

  /// The right-hand side of the binary expression.
  std::unique_ptr<Expr> rhs;

  /// The operator of this binary expression.
  const BinaryOp op;

public:
  BinaryExpr(std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs, 
             BinaryOp op, const Span &span)
      : Expr(lhs->getType(), span), lhs(std::move(lhs)), rhs(std::move(rhs)), 
      op(op) {}

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }
};

/// An integer literal. For example, `0`, `1`, etc.
class IntegerLiteral final : public Expr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class Sema;

  /// The literal value nested in this node.
  const int value;

  /// If this literal is signed.
  const unsigned signedness : 1;

public:
  IntegerLiteral(const int value, const Type *T, const unsigned signedness, 
                 const Span &span) 
      : Expr(T, span), value(value), 
      signedness(signedness ? signedness : value < 0) {}

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }
};

} // namespace artus

#endif // ARTUS_AST_EXPR_H
