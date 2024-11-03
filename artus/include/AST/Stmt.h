#ifndef ARTUS_AST_STMT_H
#define ARTUS_AST_STMT_H

#include <memory>
#include <utility>

#include "Decl.h"
#include "../Core/Span.h"
#include "../Sema/Type.h"

using std::string;

/// Forward declarations.
class Expr;

namespace artus {

/// Base class for all statement nodes.
class Stmt {
protected:
  /// Positional information about this node.
  const Span span;

public:
  Stmt(const Span &span) : span(span) {}

  /// Returns the span of this statement.
  const Span &getSpan() const { return span; }
};

/// Base class for expressions, and statements which may possess a value.
class ValueStmt : public Stmt {
protected:
  /// The type of the associated value.
  const Type *T;

public:
  ValueStmt(const Type *T, const Span &span) : Stmt(span), T(T) {}

  /// Returns the type of the value.
  const Type *getType() const { return T; }
};

/// Represents a list of statements, enclosed by braces.
class CompoundStmt final : public Stmt {
  /// The list of statements.
  const std::vector<std::unique_ptr<Stmt>> stmts;

  /// The scope associated with this compound statement.
  Scope *scope;

public:
  CompoundStmt(std::vector<std::unique_ptr<Stmt>> stmts, Scope *scope, 
               const Span &span)
      : Stmt(span), stmts(std::move(stmts)), scope(scope) {}

  /// Returns the list of statements.
  const std::vector<std::unique_ptr<Stmt>> &getStmts() const { return stmts; }

  /// Returns the scope associated with this compound statement.
  Scope *getScope() const { return scope; }
};

/// Represents a label statement. For example, `label:`.
class LabelStmt final : public Stmt {
  /// The name of the label.
  const string name;

  /// The body of the label statement.
  const std::unique_ptr<Stmt> body;

  /// The associated label declaration.
  const Decl *decl;

public:
  LabelStmt(const string &name, const Decl *decl, std::unique_ptr<Stmt> body, 
            const Span &span) 
      : Stmt(span), name(name), decl(decl), body(std::move(body)) {}

  /// Returns the name of the label.
  const string &getName() const { return name; }

  /// Returns the associated label declaration.
  const Decl *getDecl() const { return decl; }

  /// Sets the associated label declaration.
  void setDecl(const Decl *decl) { this->decl = decl; }

  /// Returns the body of the label statement.
  const Stmt *getBody() const { return body.get(); }
};

/// Represents a return statement. For example, `ret 0`.
class RetStmt final : public ValueStmt {
  /// The expression to return.
  const std::unique_ptr<Expr> expr;

public:
  RetStmt(std::unique_ptr<Expr> expr, const Type *T, const Span &span)
      : ValueStmt(T, span), expr(std::move(expr)) {}

  /// Returns the expression to return.
  const Expr *getExpr() const { return expr.get(); }
};

} // namespace artus

#endif // ARTUS_AST_STMT_H
