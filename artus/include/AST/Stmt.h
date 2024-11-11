#ifndef ARTUS_AST_STMT_H
#define ARTUS_AST_STMT_H

#include <memory>

#include "ASTPrinter.h"
#include "Decl.h"
#include "../Core/Span.h"
#include "../Sema/Type.h"

using std::string;
using std::vector;

namespace artus {

/// Forward declarations.
class Expr;

/// Base class for all statement nodes.
class Stmt {
protected:
  /// Positional information about this node.
  const Span span;

public:
  Stmt(const Span &span);

  virtual ~Stmt() = default;
  virtual void pass(ASTVisitor *visitor) = 0;

  /// Returns the span of this statement.
  const Span &getSpan() const;
};

/// Base class for expressions, and statements which may possess a value.
class ValueStmt : public Stmt {
protected:
  /// The type of the associated value.
  const Type *T;

public:
  ValueStmt(const Type *T, const Span &span);

  /// Returns the type of the value.
  const Type *getType() const;
};

/// Represents a list of statements, enclosed by braces.
class CompoundStmt final : public Stmt {
  friend class ASTPrinter;
  friend class Sema;

  /// The list of statements.
  const vector<std::unique_ptr<Stmt>> stmts;

  /// The scope associated with this compound statement.
  Scope *scope;

public:
  CompoundStmt(vector<std::unique_ptr<Stmt>> stmts, Scope *scope, 
               const Span &span);

  void pass(ASTVisitor *visitor) override;

  /// Returns the list of statements.
  const vector<std::unique_ptr<Stmt>> &getStmts() const;

  /// Returns the scope associated with this compound statement.
  Scope *getScope() const;
};

/// Represents a label statement. For example, `label:`.
class LabelStmt final : public Stmt {
  friend class ASTPrinter;
  friend class Sema;

  /// The name of the label.
  const string name;

  /// The associated label declaration.
  const Decl *decl;

public:
  LabelStmt(const string &name, const Decl *decl, const Span &span);

  void pass(ASTVisitor *visitor) override;

  /// Returns the name of the label.
  const string &getName() const;

  /// Returns the associated label declaration.
  const Decl *getDecl() const;

  /// Sets the associated label declaration.
  void setDecl(const Decl *decl);
};

/// Represents a return statement. For example, `ret 0`.
class RetStmt final : public ValueStmt {
  friend class ASTPrinter;
  friend class Sema;

  /// The expression to return.
  const std::unique_ptr<Expr> expr;

public:
  RetStmt(std::unique_ptr<Expr> expr, const Span &span);

  void pass(ASTVisitor *visitor) override;

  /// Returns the expression to return.
  const Expr *getExpr() const;
};

} // namespace artus

#endif // ARTUS_AST_STMT_H
