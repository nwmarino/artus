//>==- Stmt.h -------------------------------------------------------------==<//
//
// This header file declares inline statement AST nodes parsed from source.
//
//>==----------------------------------------------------------------------==<//

#ifndef ARTUS_AST_STMT_H
#define ARTUS_AST_STMT_H

#include <memory>
#include <vector>

#include "ASTPrinter.h"
#include "Decl.h"
#include "../Core/SourceLocation.h"
#include "../Sema/Type.h"

namespace artus {

/// Forward declarations.
class Expr;

/// Base class for all statement nodes.
class Stmt {
protected:
  /// Positional information about this node.
  const Span span;

public:
  Stmt(const Span &span) : span(span) {}

  virtual ~Stmt() = default;
  virtual void pass(ASTVisitor *visitor) = 0;

  /// \returns The span of this statement.
  const Span &getSpan() const { return span; }

  /// \returns The SourceLocation of the start of this statement's span.
  const SourceLocation getStartLoc() const { return getSpan().begin; }

  /// \returns The SourceLocation of the end of this statement's span.
  const SourceLocation getEndLoc() const { return getSpan().end; }
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
  ~ValueStmt() = default;

  /// \returns The type of the value.
  const Type *getType() const { return T; }
};

/// Represents a `break` statement in a loop.
class BreakStmt final : public Stmt {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

public:
  BreakStmt(const Span &span) : Stmt(span) {}
  ~BreakStmt() = default;

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }
};

/// Represents a `continue` statement in a loop.
class ContinueStmt final : public Stmt {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

public:
  ContinueStmt(const Span &span) : Stmt(span) {}
  ~ContinueStmt() = default;

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }
};

/// Represents a list of statements, enclosed by a block.
class CompoundStmt final : public Stmt {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The list of statements.
  const std::vector<std::unique_ptr<Stmt>> stmts;

  /// The scope associated with this compound statement.
  Scope *scope;

public:
  CompoundStmt(std::vector<std::unique_ptr<Stmt>> stmts, Scope *scope, 
               const Span &span);
  ~CompoundStmt();

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

  /// \returns The statements of this block.
  const std::vector<Stmt *> getStmts() const;

  /// \returns The scope associated with this compound statement.
  Scope *getScope() const { return scope; }
};

/// Represents a declaration statement. For example, `fix x: int = 0`.
class DeclStmt final : public Stmt {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The declaration of the variable.
  const std::unique_ptr<Decl> decl;

public:
  DeclStmt(std::unique_ptr<Decl> decl, const Span &span);
  ~DeclStmt() = default;

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }
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
  ~IfStmt() = default;

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

  /// \returns `true` if this if statement has an else statement.
  bool hasElse() const;
};

/// Represents a while loop statement.
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
  ~WhileStmt() = default;

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }
};

/// Represents an until loop statement.
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
  ~UntilStmt() = default;

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }
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
  ~MatchCase() = default;

  /// \returns `true` if this is a default case, and `false` otherwise.
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
  ~CaseStmt() = default;

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

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
  ~DefaultStmt() = default;

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

  bool isDefault() const override;
};

/// Represents a match statement.
class MatchStmt final : public Stmt {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The expression to match.
  std::unique_ptr<Expr> expr;

  /// The list of case statements.
  std::vector<std::unique_ptr<MatchCase>> cases;

public:
  MatchStmt(std::unique_ptr<Expr> expr, 
            std::vector<std::unique_ptr<MatchCase>> cases, const Span &span);
  ~MatchStmt() = default;

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

  /// \returns `true` if this match statement has a default case.
  bool hasDefault() const;

  /// \returns The default case of this match statement.
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
  ~RetStmt() = default;

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

  /// \returns The expression to return.
  Expr *getExpr() const { return expr.get(); }
};

} // end namespace artus

#endif // ARTUS_AST_STMT_H
