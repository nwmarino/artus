//>==- Decl.h -------------------------------------------------------------==<//
//
// This header file declares inline declaration AST nodes parsed from source.
//
//>==----------------------------------------------------------------------==<//

#ifndef ARTUS_AST_DECL_H
#define ARTUS_AST_DECL_H

#include <string>
#include <vector>

#include "ASTPrinter.h"
#include "ASTVisitor.h"
#include "DeclBase.h"
#include "../Sema/Type.h"

namespace artus {

/// Forward declarations.
class Expr;
class Scope;
class Stmt;

/// Base class for declarations with a name.
///
/// Named declarations are those which exist in a scope, and potentially define 
/// a symbol.
class NamedDecl : public Decl {
protected:
  /// The name of this declaration.
  const std::string name;

  /// If 1, then this declaration is private.
  bool priv : 1;

public:
  NamedDecl(const std::string &name, bool priv, const Span &span);
  virtual ~NamedDecl() = default;

  /// \returns The name of this declaration.
  const std::string &getName() const { return name; }

  /// \returns `true` if this declaration is private.
  bool isPrivate() const { return priv; }

  /// Sets this declaration to private.
  void setPrivate() { priv = true; }

  /// Sets this declaration to public.
  void setPublic() { priv = false; }

  /// Sets this declaration's visibility to \p visible.
  void setVisibility(bool visible) { this->priv = !visible; }

  /// \returns True if this declaration can be imported.
  virtual bool canImport() const = 0;
};

/// Base class for scoped declarations. 
///
/// Scoped declarations are those which have a link to a local scope.
class ScopedDecl : public NamedDecl {
protected:
  /// The scope this declaration is linked to.
  Scope *scope;

public:
  ScopedDecl(const std::string &name, Scope *scope, bool priv, 
             const Span &span);
  virtual ~ScopedDecl() = default;

  /// \returns The scope this declaration is linked to.
  Scope *getScope() const { return scope; }
};

/// Represents a variable declaration.
///
/// Variable declarations are nested declarations that must have some explicit
/// initializer expression.
class VarDecl : public NamedDecl {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The initializer expression of this variable.
  const std::unique_ptr<Expr> init;

protected:
  /// The type of this variable.
  const Type *T;

  /// If 1, then the variable is mutable.
  const bool mut : 1;

public:
  VarDecl(const std::string &name, const Type *T, std::unique_ptr<Expr> init,
          bool mut, const Span &span);
  ~VarDecl() = default;
      
  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

  /// \returns The initialization expression of this declaration.
  const Expr *getInit() const { return init.get(); }

  /// \returns The type of this variable.
  const Type *getType() const { return T; }

  /// \returns `true` if this declaration is a parameter.
  bool isParam() const { return false; }

  /// \returns `true` if the variable is mutable, and `false` otherwise.
  unsigned isMutable() const { return mut; }

  bool canImport() const override { return false; }
};

/// Represents a parameter to a function.
///
/// Parameter declarations are a subset of variable declarations, without
/// initializers.
class ParamVarDecl final : public VarDecl {
  friend class ASTPrinter;
  friend class Sema;

public:
  ParamVarDecl(const std::string &name, const Type *T, bool mut, 
               const Span &span);
  ~ParamVarDecl() = default;

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

  /// \returns `true` if this declaration is a parameter.
  bool isParam() const { return true; }
};

/// Represents a function declaration.
class FunctionDecl final : public ScopedDecl {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The owned function type of this declaration.
  const Type *T;

  /// The parameters of this declaration.
  const std::vector<std::unique_ptr<ParamVarDecl>> params;

  /// The body of this function declaration.
  const std::unique_ptr<Stmt> body;

public:
  FunctionDecl(const std::string &name, const Type *T, Scope *scope,
               std::vector<std::unique_ptr<ParamVarDecl>> params,
               bool priv, const Span &span);

  FunctionDecl(const std::string &name, const Type *T, Scope *scope,
               std::vector<std::unique_ptr<ParamVarDecl>> params,
               std::unique_ptr<Stmt> body, bool priv, const Span &span);

  ~FunctionDecl() override;

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

  /// \returns The function type declaration.
  const Type *getType() const { return T; }

  /// \returns The number of parameters of this function.
  std::size_t getNumParams() const { return params.size(); }

  /// \returns The parameter at index \p i, or `nullptr` if it does not exist.
  const ParamVarDecl *getParam(size_t i) const;

  /// \returns The body of this function declaration, if it exists.
  const Stmt *getBody() const { return body.get(); }

  /// \returns `true` if this function is an entry point.
  bool isMain() const;

  /// \returns `true` if this function has a body, and `false` otherwise.
  bool hasBody() const;

  void setParent(PackageUnitDecl *p) override;

  bool canImport() const override { return true; }
};

/// Represents an enumeration declaration.
class EnumDecl final : public NamedDecl {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The enumerators of this declaration.
  std::vector<std::string> variants;

  /// The associated enumeration type.
  const Type *T;

public:
  EnumDecl(const std::string &name, std::vector<std::string> variants, 
           const Type *T, bool priv, const Span &span);
  ~EnumDecl();

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

  /// \returns The type of this declaration.
  const Type *getType() const { return T; }

  /// \returns The number of variants of this declaration.
  std::size_t getNumVariants() const { return variants.size(); }

  /// \returns The index of \p variant, and -1 if it does not exist.
  int getVariantIndex(const std::string &variant) const;

  bool canImport() const override { return true; }
};

/// Represents a field declaration of a struct.
class FieldDecl final : public NamedDecl {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The type of this field declaration.
  const Type *T;

  /// If 1, then the field is mutable.
  const bool mut : 1;

public:
  FieldDecl(const std::string &name, const Type *T, bool mut, bool priv,
            const Span &span);
  ~FieldDecl() = default;

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

  /// \returns The type of this field declaration.
  const Type *getType() const { return T; }

  /// \returns `true` if this field is mutable, and `false` otherwise.
  bool isMutable() const { return mut; }

  bool canImport() const override { return false; }
};

/// Represents a struct declaration.
class StructDecl final : public ScopedDecl {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The fields of this struct declaration.
  const std::vector<std::unique_ptr<FieldDecl>> fields;

  /// The associated struct type.
  const Type *T;

public:
  StructDecl(const std::string &name, const Type *T, Scope *scope,
             std::vector<std::unique_ptr<FieldDecl>> fields, bool priv, 
             const Span &span);
  ~StructDecl() override;

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

  /// \returns The number of fields of this declaration.
  std::size_t getNumFields() const { return fields.size(); }

  /// \returns The field at index \p i, and `nullptr` if it does not exist.
  const FieldDecl *getField(std::size_t i) const;

  /// \returns The index of \p field, and -1 if it does not exist.
  int getFieldIndex(const std::string &field) const;

  /// \returns The type of the field \p name, and `nullptr` if it does not exist.
  const Type *getFieldType(const std::string &name) const;

  /// \returns `true` if the field \p name is mutable, and `false` otherwise.
  bool isFieldMutable(const std::string &name) const;

  /// \returns The type of this struct.
  const Type *getType() const { return T; }

  void setParent(PackageUnitDecl *p) override;

  bool canImport() const override { return true; }
};

} // end namespace artus

#endif // ARTUS_AST_DECL_H
