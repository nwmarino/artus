#ifndef ARTUS_AST_DECLBASE_H
#define ARTUS_AST_DECLBASE_H

#include <memory>
#include <vector>

#include "ASTPrinter.h"
#include "../Core/SourcePath.h"
#include "../Core/Span.h"

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

  virtual void pass(ASTVisitor *visitor) = 0;
};

/// Base class for all in-line declaration nodes.
class Decl : public DeclBase {
protected:
  /// Positional information about this node.
  const Span span;

public:
  Decl(const Span &span);

  /// Returns the span of this declaration.
  const Span &getSpan() const;
};

/// Represents an import declaration. An import may be used to import a local
/// source file or ones from the standard library.
class ImportDecl final : public Decl {
  friend class ASTPrinter;
  friend class ReferenceAnalysis;

  /// The name of the package to import.
  const SourcePath path;

  /// If the package is from the local source tree.
  const bool local;

public:
  ImportDecl(const SourcePath &path, const Span &span, bool local = true);

  void pass(ASTVisitor *visitor) override;

  /// Returns the path of the package to import.
  const SourcePath &getPath() const;

  /// Returns if the package is from the local source tree.
  bool isLocal() const;
};

/// Represents a package declaration. A package represents a singular source
/// file, and may import the declarations of other packages in a source tree.
class PackageUnitDecl final : public DeclBase {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

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
                  vector<std::unique_ptr<Decl>> decls);

  void pass(ASTVisitor *visitor) override;

  /// Returns the identifier of this package unit.
  const string &getIdentifier() const;

  /// Returns the names of imported package units.
  const vector<string> &getImports() const;

  /// Adds a declaration to this package unit.
  void addDecl(std::unique_ptr<Decl> decl);

  /// Adds an imported package unit to this package unit.
  void addImport(const string &import);
};

} // namespace artus

#endif // ARTUS_AST_DECLBASE_H
