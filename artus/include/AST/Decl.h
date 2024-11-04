#ifndef ARTUS_AST_DECL_H
#define ARTUS_AST_DECL_H

#include "DeclBase.h"
#include "../Core/Span.h"
#include "../Sema/Type.h"

using std::size_t;
using std::string;
using std::vector;

namespace artus {

/// Forward declarations.
class Scope;
class Stmt;

/// Base class for all in-line Declaration nodes.
class Decl : public DeclBase {
protected:
  /// Positional information about this node.
  const Span span;

public:
  Decl(const Span &span) : span(span) {}

  /// Returns the span of this declaration.
  const Span &getSpan() const { return span; }
};

/// Base class for all declarations. Named declarations are those which exist in
/// a scope, and sometimes define a symbol.
class NamedDecl : public Decl {
protected:
  /// The name of the declaration.
  const string name;

public:
  NamedDecl(const string &name, const Span &span) : Decl(span), name(name) {}

  /// Returns the name of this declaration.
  const string &getName() const { return name; }
};

/// Base class for scoped declarations. Scoped declarations are those which
/// possess a link to a local scope.
class ScopedDecl : public NamedDecl {
protected:
  /// The scope in which this declaration resides.
  Scope *scope;

public:
  ScopedDecl(const string &name, Scope *scope, const Span &span)
      : NamedDecl(name, span), scope(scope) {}

  /// Returns the scope in which this declaration resides.
  Scope *getScope() const { return scope; }
};

/// Represents the declaration of a label statement.
class LabelDecl final : public NamedDecl {
  /// The associated label statement.
  const Stmt *stmt;

public:
  LabelDecl(const string &name, const Span &span)
      : NamedDecl(name, span), stmt(nullptr) {}

  /// Returns the associated label statement.
  const Stmt *getStmt() const { return stmt; }

  /// Sets the associated label statement.
  void setStmt(const Stmt *stmt) { this->stmt = stmt; }
};

/// Represents a parameter to a function.
class ParamVarDecl final : public NamedDecl {
  /// The type of this parameter.
  const Type *T;

public:
  ParamVarDecl(const string &name, const Type *T, const Span &span)
      : NamedDecl(name, span), T(T) {}

  /// Returns the type of this parameter.
  const Type *getType() const { return T; }
};

/// Represents a function declaration.
class FunctionDecl final : public ScopedDecl {
  /// The return type of this function declaration.
  const Type *T;

  /// The parameters of this function declaration.
  const vector<std::unique_ptr<ParamVarDecl>> params;

  /// The body of this function declaration.
  const std::unique_ptr<Stmt> body;

public:
  FunctionDecl(const string &name, const Type *T,
               vector<std::unique_ptr<ParamVarDecl>> params,
               std::unique_ptr<Stmt> body, Scope *scope, const Span &span)
      : ScopedDecl(name, scope, span), T(T), params(std::move(params)),
        body(std::move(body)) {}

  /// Returns the return type of this function declaration.
  const Type *getType() const { return T; }

  /// Returns the number of parameters in this function declaration.
  inline size_t getNumParams() const { return params.size(); }

  /// Returns the parameter at the specified index, and `nullptr` if it does
  /// not exist.
  inline const ParamVarDecl *getParam(size_t i) const {
    return i < params.size() ? params[i].get() : nullptr;
  }

  /// Returns the body of this function declaration.
  const Stmt *getBody() const { return body.get(); }
};

} // namespace artus

#endif // ARTUS_AST_DECL_H
