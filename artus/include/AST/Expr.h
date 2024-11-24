#ifndef ARTUS_AST_EXPR_H
#define ARTUS_AST_EXPR_H

#include <memory>

#include "ASTPrinter.h"
#include "Stmt.h"
#include "../Codegen/Codegen.h"
#include "../Core/Span.h"
#include "../Sema/Type.h"

namespace artus {

/// Base class for all Expression nodes. Expressions are also statements.
class Expr : public ValueStmt {
protected:
  /// If this expression is a valid lvalue.
  bool lvalue : 1;

public:
  Expr(const Type *T, const Span &span) : ValueStmt(T, span), lvalue(false) {}

  /// Returns true if this expression is an lvalue.
  bool isLValue() const { return lvalue; }
};

/// Base class for all Cast Expressions.
class CastExpr : public Expr {
  friend class Codegen;

protected:
  /// The expression to cast.
  std::unique_ptr<Expr> expr;

  /// The identifier of the type cast.
  const string ident;

public:
  CastExpr(std::unique_ptr<Expr> expr, const string &ident, const Type *T, 
           const Span &span)
      : Expr(T, span), expr(std::move(expr)), ident(ident) {}
};

/// An implicit cast. Usually injected by the ompiler during sema to recover
/// from type mismatches or undefined behaviour.
class ImplicitCastExpr final : public CastExpr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

public:
  ImplicitCastExpr(std::unique_ptr<Expr> expr, const string &ident, 
                   const Type *T, const Span &span)
      : CastExpr(std::move(expr), ident, T, span) {}

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }
};

/// An explicit cast. For example, `i64 0`.
class ExplicitCastExpr final : public CastExpr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

public:
  ExplicitCastExpr(std::unique_ptr<Expr> expr, const string &ident, 
                   const Type *T, const Span &span)
      : CastExpr(std::move(expr), ident, T, span) {}

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }
};

/// A reference to a declaration. For example, `x`.
class DeclRefExpr : public Expr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The identifier of the referenced declaration.
  const string ident;

  /// The declaration being referenced.
  const Decl *decl;

public:
  DeclRefExpr(const string ident, const Decl *decl, const Type *T, 
              const Span &span) : Expr(T, span), ident(ident), decl(decl) {
    this->lvalue = true;
  }

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

  /// Returns the identifier of this reference.
  const string &getIdent() const { return ident; }
};

/// A call expression. For example, `@foo()`.
class CallExpr final : public DeclRefExpr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The arguments of the call.
  std::vector<std::unique_ptr<Expr>> args;

public:
  CallExpr(const string callee, const Decl *decl, const Type *T,
           vector<std::unique_ptr<Expr>> args, const Span &span)
      : DeclRefExpr(callee, decl, T, span), args(std::move(args)) {}

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

  /// Returns the number of arguments of this call.
  size_t getNumArgs() const { return args.size(); }

  /// Returns the argument at the given index.
  Expr *getArg(size_t i) const { return args[i].get(); }
};

/// A unary expression. For example `-1`, `!true`, etc.
class UnaryExpr final : public Expr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

public:
  /// Possible kinds of unary operations.
  enum class UnaryOp {
    Unknown = -1,
    // -
    Negative,
    // !
    Not,
    // &
    Ref,
    // *
    DeRef,
  };

private:
  /// The base of the unary expression.
  std::unique_ptr<Expr> base;

  /// The operator of this unary expression.
  const UnaryOp op;

public:
  UnaryExpr(std::unique_ptr<Expr> base, UnaryOp op, const Span &span)
      : Expr(base->getType(), span), base(std::move(base)), op(op) {
    this->lvalue = this->base->isLValue();
  }

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }
};

/// A binary expression. For example, `1 + 1`, `2 * 3`, etc.
class BinaryExpr final : public Expr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

public:
  /// Possible kinds of binary operations.
  enum class BinaryOp {
    Unknown = -1,
    // =
    Assign,
    // +=
    AddAssign,
    // -=
    SubAssign,
    // *=
    MultAssign,
    // /=
    DivAssign,
    // ==
    Equals,
    // !=
    NotEquals,
    // <
    LessThan,
    // >
    GreaterThan,
    // <=
    LessEquals,
    // >=
    GreaterEquals,
    // &&
    LogicalAnd,
    // ||
    LogicalOr,
    // ^^
    LogicalXor,
    // +
    Add,
    // -
    Sub,
    // *
    Mult,
    // /
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
             BinaryOp op, const Span &span) : Expr(lhs->getType(), span), 
      lhs(std::move(lhs)), rhs(std::move(rhs)), op(op) {
    this->lvalue = this->lhs->isLValue();
  }

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

  /// Returns true if this binary expression is a direct assignment.
  bool isDirectAssignment() const { return op == BinaryOp::Assign; }

  /// Returns true if this binary expression is an assignment.
  bool isAssignment() const 
  { return op >= BinaryOp::Assign && op <= BinaryOp::DivAssign; }

  /// Returns true if this binary expression is a comparison.
  bool isComparison() const 
  { return op >= BinaryOp::Equals && op <= BinaryOp::LogicalXor; }
};

/// A boolean literal; `true` or `false`.
class BooleanLiteral final : public Expr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The literal value nested in this node.
  const bool value;

public:
  BooleanLiteral(const bool value, const Type *T, const Span &span)
      : Expr(T, span), value(value) {}

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }
};

/// An integer literal. For example, `0`, `1`, etc.
class IntegerLiteral final : public Expr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
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

/// A floating point literal. For example, `0.0`, `1.0`, etc.
class FPLiteral final : public Expr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The literal value nested in this node.
  const double value;

public:
  FPLiteral(const double value, const Type *T, const Span &span)
      : Expr(T, span), value(value) {}

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }
};

/// A character literal. For example, `'a'`, `'b'`, etc.
class CharLiteral final : public Expr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The literal value nested in this node.
  const char value;

public:
  CharLiteral(const char value, const Type *T, const Span &span)
      : Expr(T, span), value(value) {}

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }
};

/// A string literal. For example, `"hello"`, `"world"`, etc.
class StringLiteral final : public Expr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The literal value nested in this node.
  const string value;

public:
  StringLiteral(const string value, const Type *T, const Span &span)
      : Expr(T, span), value(value) {}

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }
};

/// A `null` expression.
class NullExpr final : public Expr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

public:
  NullExpr(const Type *T, const Span &span) : Expr(T, span) {}

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }
};

/// An array initialization expression. For example, `[1, 2, 3]`.
class ArrayExpr final : public Expr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The list of expressions in the array.
  vector<std::unique_ptr<Expr>> exprs;

public:
  ArrayExpr(vector<std::unique_ptr<Expr>> exprs, const Type *T, 
                const Span &span)
      : Expr(T, span), exprs(std::move(exprs)) {}

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

  /// Returns the number of expressions in this array.
  size_t getNumExprs() const { return exprs.size(); }

  /// Returns the expression at the given index.
  Expr *getExpr(size_t i) const { return exprs[i].get(); }
};

/// An array access expression. For example, `arr[0]`.
class ArraySubscriptExpr final : public Expr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The name of the base.
  const string name;

  /// The base of the array access.
  std::unique_ptr<Expr> base;

  /// The index of the array access.
  std::unique_ptr<Expr> index;

public:
  ArraySubscriptExpr(const string &name, std::unique_ptr<Expr> base, 
                  std::unique_ptr<Expr> index, const Type *T, const Span &span)
      : Expr(T, span), name(name), base(std::move(base)), 
      index(std::move(index)) {
    this->lvalue = true;
  }

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }
};

} // namespace artus

#endif // ARTUS_AST_EXPR_H
