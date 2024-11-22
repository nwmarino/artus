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

  /// If this declaration is private.
  bool priv : 1;

public:
  NamedDecl(const string &name, const Span &span, bool priv = false);

  /// Returns the name of this declaration.
  const string &getName() const;

  /// Returns true if this declaration is private.
  bool isPrivate() const;

  /// Sets this declaration to private.
  void setPrivate();

  /// Sets this declaration to public.
  void setPublic();

  /// Returns true if this declaration can be imported to another package.
  virtual bool canImport() const = 0;
};

/// Base class for scoped declarations. Scoped declarations are those which
/// possess a link to a local scope.
class ScopedDecl : public NamedDecl {
protected:
  /// The scope in which this declaration resides.
  Scope *scope;

public:
  ScopedDecl(const string &name, Scope *scope, const Span &span, 
             bool priv = false);

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

  bool canImport() const override;
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

  bool canImport() const override;
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
               std::unique_ptr<Stmt> body, Scope *scope, const Span &span,
               bool priv = false);

  void pass(ASTVisitor *visitor) override;

  /// Returns the function type of this function declaration.
  const Type *getType() const;

  /// Returns the number of parameters in this function declaration.
  size_t getNumParams() const;

  /// Returns the parameter at the specified index, and `nullptr` if it does
  /// not exist.
  const ParamVarDecl *getParam(size_t i) const;

  bool canImport() const override;
};

/// Represents a field declaration of a struct.
class FieldDecl final : public NamedDecl {
  friend class ASTPrinter;
  friend class Sema;

  /// The type of this field declaration.
  const Type *T;

public:
  FieldDecl(const string &name, const Type *T, const Span &span);

  void pass(ASTVisitor *visitor) override;

  /// Returns the type of this field declaration.
  const Type *getType() const;

  bool canImport() const override;
};

/// Represents a struct declaration.
class StructDecl final : public ScopedDecl {
  friend class ASTPrinter;
  friend class Sema;

  /// The fields of this struct declaration.
  const vector<std::unique_ptr<FieldDecl>> fields;

  /// The implemented traits of this struct declaration.
  vector<string> traits;

public:
  StructDecl(const string &name, vector<std::unique_ptr<FieldDecl>> fields,
             Scope *scope, const Span &span, bool priv = false);

  void pass(ASTVisitor *visitor) override;

  /// Returns the number of fields in this struct declaration.
  size_t getNumFields() const;

  /// Returns the field at the specified index, and `nullptr` if it does not
  /// exist.
  const FieldDecl *getField(size_t i) const;

  /// Adds a trait to this struct declaration.
  void addTrait(const string &trait);

  bool canImport() const override;
};

} // namespace artus

#endif // ARTUS_AST_DECL_H
