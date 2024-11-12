#ifndef ARTUS_AST_EXPR_H
#define ARTUS_AST_EXPR_H

#include "ASTPrinter.h"
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
