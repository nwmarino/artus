#ifndef ARTUS_AST_EXPR_H
#define ARTUS_AST_EXPR_H

#include "Stmt.h"
#include "../Core/Span.h"
#include "../Sema/Type.h"

namespace artus {

/// Base class for all Expression nodes. Expressions are also statements.
class Expr : public ValueStmt {
public:
  Expr(const Type *T, const Span &span) : ValueStmt(T, span) {}
};

/// An integer literal. For example, `0`, `1`, etc.
class IntegerLiteral final : public Expr {
  /// The literal value nested in this node.
  const int value;

public:
  IntegerLiteral(const int value, const Type *T, const Span &span) 
      : Expr(T, span), value(value) {}

  /// Returns the value nested in this node.
  const int getValue() const { return value; }
};

} // namespace artus

#endif // ARTUS_AST_EXPR_H
