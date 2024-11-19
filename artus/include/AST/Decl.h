#ifndef ARTUS_AST_DECL_H
#define ARTUS_AST_DECL_H

#include "ASTPrinter.h"
#include "ASTVisitor.h"
#include "DeclBase.h"
#include "../Sema/Type.h"

using std::size_t;
using std::string;
using std::vector;

namespace artus {

/// Forward declarations.
class Expr;
class Scope;
class Stmt;

/// Base class for all declarations. Named declarations are those which exist in
/// a scope, and sometimes define a symbol.
class NamedDecl : public Decl {
protected:
  /// The name of the declaration.
  const string name;

public:
  NamedDecl(const string &name, const Span &span);

  /// Returns the name of this declaration.
  const string &getName() const;
};

/// Base class for scoped declarations. Scoped declarations are those which
/// possess a link to a local scope.
class ScopedDecl : public NamedDecl {
protected:
  /// The scope in which this declaration resides.
  Scope *scope;

public:
  ScopedDecl(const string &name, Scope *scope, const Span &span);

  /// Returns the scope in which this declaration resides.
  Scope *getScope() const;
};

/// Represents the declaration of a label statement.
class LabelDecl final : public NamedDecl {
  friend class Sema;

  /// The associated label statement.
  const Stmt *stmt;

public:
  LabelDecl(const string &name, const Span &span);

  void pass(ASTVisitor *visitor) override;

  /// Returns the associated label statement.
  const Stmt *getStmt() const;

  /// Sets the associated label statement.
  void setStmt(const Stmt *stmt);
};

/// Represents a variable declaration.
class VarDecl : public NamedDecl {
  friend class ASTPrinter;
  friend class Codegen;
  friend class Sema;

  /// The initializer expression of this variable.
  const std::unique_ptr<Expr> init;

protected:
  /// The type of this variable.
  const Type *T;

  /// If the variable is mutable.
  const bool mut : 1;

public:
  VarDecl(const string &name, const Type *T, std::unique_ptr<Expr> init,
          const bool mut, const Span &span);
      
  void pass(ASTVisitor *visitor) override;

  /// Returns ther type of this variable.
  const Type *getType() const;

  /// Returns true if this declaration is a parameter.
  bool isParam() const;

  /// Returns true if the variable is mutable, and false otherwise.
  unsigned isMutable() const;
};

/// Represents a parameter to a function.
class ParamVarDecl final : public VarDecl {
  friend class ASTPrinter;
  friend class Sema;

public:
  ParamVarDecl(const string &name, const Type *T, const bool mut, 
               const Span &span);

  void pass(ASTVisitor *visitor) override;
};

/// Represents a function declaration.
class FunctionDecl final : public ScopedDecl {
  friend class ASTPrinter;
  friend class Codegen;
  friend class Sema;

  /// The return type of this function declaration.
  const Type *T;

  /// The parameters of this function declaration.
  const vector<std::unique_ptr<ParamVarDecl>> params;

  /// The body of this function declaration.
  const std::unique_ptr<Stmt> body;

public:
  FunctionDecl(const string &name, const Type *T,
               vector<std::unique_ptr<ParamVarDecl>> params,
               std::unique_ptr<Stmt> body, Scope *scope, const Span &span);

  void pass(ASTVisitor *visitor) override;

  /// Returns the function type of this function declaration.
  const Type *getType() const;

  /// Returns the number of parameters in this function declaration.
  size_t getNumParams() const;

  /// Returns the parameter at the specified index, and `nullptr` if it does
  /// not exist.
  const ParamVarDecl *getParam(size_t i) const;
};

} // namespace artus

#endif // ARTUS_AST_DECL_H
