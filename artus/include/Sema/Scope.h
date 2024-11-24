#ifndef ARTUS_SEMA_SCOPE_H
#define ARTUS_SEMA_SCOPE_H

#include <algorithm>

#include "../AST/Decl.h"
#include "../Lex/Token.h"

using std::vector;

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
  vector<NamedDecl *> decls;

  /// Context for this scope instance.
  ScopeContext ctx;

public:
  Scope(Scope *parent, vector<NamedDecl *> decls, ScopeContext ctx) 
      : parent(parent), decls(std::move(decls)), ctx(ctx) {}

  /// Returns the parent scope of this scope.
  Scope *getParent() const { return parent; }

  /// Returns the declarations within this local scope.
  const vector<NamedDecl *> &getDecls() const { return decls; }

  /// Add a declaration to this scope.
  void addDecl(NamedDecl *decl) { decls.push_back(decl); }

  /// Deletes a declaration from this scope.
  void deleteDecl(NamedDecl *decl) {
    auto d = std::find(decls.begin(), decls.end(), decl);
    if (d != decls.end())
      decls.erase(d);
  }

  /// Retrieves a declaration by name, if it exists, and `nullptr` otherwise.
  NamedDecl *getDecl(const string &name) const {
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

  /// Returns true if this is a global scope, and false otherwise.
  bool isUnitScope() const { return ctx.isUnitScope; }

  /// Returns true if this is a function scope, and false otherwise.
  bool isFunctionScope() const { return ctx.isFunctionScope; }

  /// Returns true if this is a compound statement scope, and false otherwise.
  bool isCompoundScope() const { return ctx.isCompoundScope; }

  /// Returns true if this is a struct scope, and false otherwise.
  bool isStructScope() const { return ctx.isStructScope; }
};

} // namespace artus

#endif // ARTUS_SEMA_SCOPE_H
