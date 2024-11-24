#ifndef ARTUS_AST_STMT_H
#define ARTUS_AST_STMT_H

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
  friend class ReferenceAnalysis;
  friend class Sema;

protected:
  /// The type of the associated value.
  const Type *T;

public:
  ValueStmt(const Type *T, const Span &span);

  /// Returns the type of the value.
  const Type *getType() const;
};

/// Represents a `break` statement in a loop.
class BreakStmt final : public Stmt {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

public:
  BreakStmt(const Span &span);

  void pass(ASTVisitor *visitor) override;
};

/// Represents a `cont` statement in a loop.
class ContinueStmt final : public Stmt {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

public:
  ContinueStmt(const Span &span);

  void pass(ASTVisitor *visitor) override;
};

/// Represents a list of statements, enclosed by braces.
class CompoundStmt final : public Stmt {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
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

/// Represents a declaration statement. For example, `fix x: int = 0`. This
/// node nests the declaration of a variable, as to not inline it.
class DeclStmt final : public Stmt {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The declaration of the variable.
  const std::unique_ptr<Decl> decl;

public:
  DeclStmt(std::unique_ptr<Decl> decl, const Span &span);

  void pass(ASTVisitor *visitor) override;
};

/// Represents an if statement.
class IfStmt final : public Stmt {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The condition of the if statement.
  std::unique_ptr<Expr> cond;

  /// The body of the if statement.
  std::unique_ptr<Stmt> thenStmt;

  /// The body of the else statement.
  std::unique_ptr<Stmt> elseStmt;

public:
  IfStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> thenStmt, 
         std::unique_ptr<Stmt> elseStmt, const Span &span);

  void pass(ASTVisitor *visitor) override;

  /// Returns true if this if statement has an else statement.
  bool hasElse() const;
};

/// Represents a while loop statement: `while <expr> <stmt>`.
class WhileStmt final : public Stmt {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The condition of the while loop.
  std::unique_ptr<Expr> cond;

  /// The body of the while loop.
  std::unique_ptr<Stmt> body;

public:
  WhileStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> body, 
            const Span &span);

  void pass(ASTVisitor *visitor) override;
};

/// Represents an until loop statement: `until <expr> <stmt>`.
class UntilStmt final : public Stmt {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The condition of the until loop.
  std::unique_ptr<Expr> cond;

  /// The body of the until loop.
  std::unique_ptr<Stmt> body;

public:
  UntilStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> body, 
            const Span &span);

  void pass(ASTVisitor *visitor) override;
};

/// Represents a generic case in a match statement.
class MatchCase : public Stmt {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

protected:
  /// The body of the case statement.
  std::unique_ptr<Stmt> body;

public:
  MatchCase(std::unique_ptr<Stmt> body, const Span &span);

  virtual bool isDefault() const = 0;
};

/// Represents an expression-based case in a match statement.
class CaseStmt final : public MatchCase {
  friend class ASTPrinter;
  friend class Codegen;
  friend class MatchStmt;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The expression to match.
  std::unique_ptr<Expr> expr;

public:
  CaseStmt(std::unique_ptr<Expr> expr, std::unique_ptr<Stmt> body, 
           const Span &span);

  void pass(ASTVisitor *visitor) override;

  bool isDefault() const override;
};

/// Represents a default `_` case in a match statement.
class DefaultStmt final : public MatchCase {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

public:
  DefaultStmt(std::unique_ptr<Stmt> body, const Span &span);

  void pass(ASTVisitor *visitor) override;

  bool isDefault() const override;
};

/// Represents a match statement: `match <expr> { <case> ... }`.
class MatchStmt final : public Stmt {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The expression to match.
  std::unique_ptr<Expr> expr;

  /// The list of case statements.
  vector<std::unique_ptr<MatchCase>> cases;

public:
  MatchStmt(std::unique_ptr<Expr> expr, 
            vector<std::unique_ptr<MatchCase>> cases, const Span &span);

  void pass(ASTVisitor *visitor) override;

  /// Returns true if this match statement has a default case.
  bool hasDefault() const;

  /// Returns the default case of this match statement.
  DefaultStmt *getDefault() const;
};

/// Represents a return statement. For example, `ret 0`.
class RetStmt final : public ValueStmt {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The expression to return.
  std::unique_ptr<Expr> expr;

public:
  RetStmt(std::unique_ptr<Expr> expr, const Span &span);

  void pass(ASTVisitor *visitor) override;

  /// Returns the expression to return.
  const Expr *getExpr() const;
};

} // namespace artus

#endif // ARTUS_AST_STMT_H
