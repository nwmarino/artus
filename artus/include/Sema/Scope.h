//>==- Scope.h ------------------------------------------------------------==<//
//
// This header file declares a data structure used to represent scopes in a
// a syntax tree. The scope is used to store declarations and to resolve
// references to variables. The scope is transient and is destroyed after
// semantic analysis.
//
//>==----------------------------------------------------------------------==<//

#ifndef ARTUS_SEMA_SCOPE_H
#define ARTUS_SEMA_SCOPE_H

#include <algorithm>
#include <string>
#include <vector>

#include "../AST/Decl.h"
#include "../Core/Logger.h"
#include "../Lex/Token.h"

namespace artus {

/// Represents scope context. In particular, flags that indicate the location
/// of a scope relative to other scopes.
struct ScopeContext {
  /// If this scope is a global scope.
  unsigned int isUnitScope : 1;

  /// If this scope is a function scope.
  unsigned int isFunctionScope : 1;

  /// If this scope is a compound statement scope.  
  unsigned int isCompoundScope : 1;

  /// If this scope is a struct scope.
  unsigned int isStructScope : 1;
};

/// Represents a scope in the source code. This tree data structure is transient
/// in that it is only related to the AST and destroyed after semantic analysis.
class Scope {
  /// The parent scope of this scope.
  Scope *parent = nullptr;

  /// The declarations within this local scope.
  std::vector<NamedDecl *> decls;

  /// Context for this scope instance.
  ScopeContext ctx;

public:
  Scope(Scope *parent, std::vector<NamedDecl *> decls, ScopeContext ctx) 
      : parent(parent), decls(std::move(decls)), ctx(ctx) {}
  ~Scope() = default;

  /// \returns The parent scope of this scope.
  Scope *getParent() const { return parent; }

  /// \returns The declarations within this local scope.
  const std::vector<NamedDecl *> &getDecls() const { return decls; }

  /// Adds the declaration \p decl to this scope.
  void addDecl(NamedDecl *decl) {
    // Check if a declaration with the same name already exists in the scope.
    if (NamedDecl *d = getDecl(decl->getName()))
      fatal("redeclaration of: " + decl->getName(), decl->getStartLoc());

    decls.push_back(decl); 
  }

  /// Deletes the declaration \p decl from this scope, if it exists.
  void deleteDecl(NamedDecl *decl) {
    auto d = std::find(decls.begin(), decls.end(), decl);
    if (d != decls.end())
      decls.erase(d);
  }

  /// \returns The declaration most similar in name to \p name.
  NamedDecl *getDecl(const std::string &name) const {
    if (isReserved(name))
      return nullptr;

    for (NamedDecl *decl : decls) {
      if (decl->getName() == name)
        return decl;
    }
    
    if (parent)
      return parent->getDecl(name);

    return nullptr;
  }

  /// \returns `true` if this is a global scope.
  bool isUnitScope() const { return ctx.isUnitScope; }

  /// \returns `true` if this is a function scope.
  bool isFunctionScope() const { return ctx.isFunctionScope; }

  /// \returns `true` if this is a compound statement scope.
  bool isCompoundScope() const { return ctx.isCompoundScope; }

  /// \returns `true` if this is a struct scope.
  bool isStructScope() const { return ctx.isStructScope; }
};

} // end namespace artus

#endif // ARTUS_SEMA_SCOPE_H
