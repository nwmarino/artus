#ifndef ARTUS_AST_DECLBASE_H
#define ARTUS_AST_DECLBASE_H

#include <memory>
#include <string>
#include <utility>
#include <vector>

using std::string;
using std::vector;

/// Forward declarations.
class Decl;
class Scope;

namespace artus {

/// Base class for all Declaration nodes. Primarily used to distinguish inline
/// declarations and that of translation units and the like.
class DeclBase {
public:
  virtual ~DeclBase() = default;
};

/// Represents a translation unit declaration. A translation unit is a single
/// source file, and may import the declarations of other translation units.
class TranslationUnitDecl : public DeclBase {
  /// The name or identifier associated with this translation unit.
  const string identifier;

  /// The names or identifiers of imported translation units.
  vector<string> imports;

  /// The declarations of this translation unit.
  vector<std::unique_ptr<Decl>> decls;

  Scope *scope;

public:
  TranslationUnitDecl(const string &id, vector<string> imports,
                      vector<std::unique_ptr<Decl>> decls, Scope *scope)
      : identifier(id), imports(std::move(imports)), decls(std::move(decls)),
        scope(scope) {}

  /// Returns the identifier of this translation unit.
  const string &getIdentifier() const { return identifier; }

  /// Returns the names of imported translation units.
  const vector<string> &getImports() const { return imports; }

  /// Returns the declarations of this translation unit.
  const vector<std::unique_ptr<Decl>> &getDecls() const { return decls; }

  /// Adds a declaration to this translation unit.
  void addDecl(std::unique_ptr<Decl> decl) { decls.push_back(std::move(decl)); }

  /// Adds an imported translation unit to this translation unit.
  void addImport(const string &import) { imports.push_back(import); }
};

} // namespace artus

#endif // ARTUS_AST_DECLBASE_H
