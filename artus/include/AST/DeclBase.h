#ifndef ARTUS_AST_DECLBASE_H
#define ARTUS_AST_DECLBASE_H

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../Core/Context.h"

using std::string;
using std::vector;

namespace artus {

/// Forward declarations.
class Decl;
class Scope;

/// Base class for all Declaration nodes. Primarily used to distinguish inline
/// declarations and that of packages and the like.
class DeclBase {
public:
  virtual ~DeclBase() = default;
};

/// Represents a package declaration. A package represents a singular source
/// file, and may import the declarations of other packages in a source tree.
class PackageUnitDecl final : public DeclBase {
  /// The unique name or identifier associated with this package.
  const string identifier;

  /// The names or identifiers of imported packages.
  vector<string> imports;

  /// The declarations of this package.
  vector<std::unique_ptr<Decl>> decls;

  /// The scope of this package.
  Scope *scope;

public:
  PackageUnitDecl(const string &id, vector<string> imports, Scope *scope,
                  vector<std::unique_ptr<Decl>> decls)
      : identifier(id), imports(std::move(imports)), decls(std::move(decls)),
        scope(scope) {}

  /// Returns the identifier of this package unit.
  const string &getIdentifier() const { return identifier; }

  /// Returns the names of imported package units.
  const vector<string> &getImports() const { return imports; }

  /// Returns the declarations of this package unit.
  const vector<std::unique_ptr<Decl>> &getDecls() const { return decls; }

  /// Adds a declaration to this package unit.
  void addDecl(std::unique_ptr<Decl> decl) { decls.push_back(std::move(decl)); }

  /// Adds an imported package unit to this package unit.
  void addImport(const string &import) { imports.push_back(import); }
};

} // namespace artus

#endif // ARTUS_AST_DECLBASE_H
