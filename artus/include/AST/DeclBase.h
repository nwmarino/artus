//>==- DeclBase.h ---------------------------------------------------------==<//
//
// This header file defines important declaration classes.
//
//>==----------------------------------------------------------------------==<//

#ifndef ARTUS_AST_DECLBASE_H
#define ARTUS_AST_DECLBASE_H

#include <vector>

#include "ASTPrinter.h"
#include "../Core/SourcePath.h"
#include "../Core/Span.h"

using std::string;
using std::vector;

namespace artus {

/// Forward declarations.
class Decl;
class NamespaceDecl;
class Scope;

/// Base class for all formal declaration nodes. 
///
/// The existence of this class as opposed to the sole `Decl` class is to
/// further distinguish inline declarations and that of package declarations
/// and the like.
class DeclBase {
public:
  virtual ~DeclBase() = default;
  virtual void pass(ASTVisitor *visitor) = 0;
};

/// Base class for all inline, explicit declaration nodes.
class Decl : public DeclBase {
protected:
  /// Positional information about this node.
  const Span span;

public:
  Decl(const Span &span) : span(span) {}

  const Span &getSpan() const { return span; }

  /// Returns a SourceLocation of the start of this declaration's span.
  const SourceLocation getBeginLoc() const
  { return { .file=span.file, .line=span.line, .col=span.col }; }

  /// Returns a SourceLocation of the end of this declaration's span.
  const SourceLocation getEndLoc() const
  { return { .file=span.file, .line=span.line_nd, .col=span.col_nd }; }
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
  vector<ImportDecl *> imports;

  /// The declarations of this package.
  vector<Decl *> decls;

  /// The scope of this package.
  Scope *scope;

public:
  PackageUnitDecl(const string &id, vector<ImportDecl *> imports, Scope *scope,
                  vector<Decl *> decls);

  void pass(ASTVisitor *visitor) override;

  /// Returns the identifier of this package unit.
  const string &getIdentifier() const;

  /// Returns the names of imported package units.
  const vector<string> &getImports() const;

  /// Adds a declaration to this package unit.
  void addDecl(Decl *decl);

  /// Deletes a declaration from this package unit.
  void delDecl(Decl *decl);

  /// Adds an imported package unit to this package unit.
  void addImport(ImportDecl *import);
};

} // end namespace artus

#endif // ARTUS_AST_DECLBASE_H
