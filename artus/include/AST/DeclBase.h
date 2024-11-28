//>==- DeclBase.h ---------------------------------------------------------==<//
//
// This header file declares important semantic declaration classes.
//
//>==----------------------------------------------------------------------==<//

#ifndef ARTUS_AST_DECLBASE_H
#define ARTUS_AST_DECLBASE_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ASTPrinter.h"
#include "../Core/SourceLocation.h"
#include "../Core/SourcePath.h"

namespace artus {

/// Forward declarations.
class NamedDecl;
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
  /// The package this declaration is contained in.
  PackageUnitDecl *parent;

  /// Positional information about this node.
  const Span span;

public:
  Decl(const Span &span) : span(span), parent(nullptr) {}
  ~Decl() = default;

  /// \returns The source code span of this declaration.
  const Span &getSpan() const { return span; }

  /// \returns The SourceLocation of the start of this declaration's span.
  const SourceLocation getStartLoc() const { return getSpan().begin; }

  /// \returns The SourceLocation of the end of this declaration's span.
  const SourceLocation getEndLoc() const { return getSpan().end; }

  /// \returns The parent package of this declaration.
  PackageUnitDecl *getParent() const { return parent; }

  /// Sets this declaration's parent to \p p.
  void setParent(PackageUnitDecl *p) { parent = p; }
};

/// Defines a context for owned declarations.
///
/// This interface provides a way to manage and query owned declarations.
class DeclContext final {
  /// The declarations owned by this context.
  std::vector<std::unique_ptr<Decl>> decls;

  /// A map of non-owning declarations by name.
  std::map<std::string, NamedDecl *> map;

public:
  DeclContext() = default;
  ~DeclContext() = default;

  /// Moves the \p decl to be owned by this context.
  Decl *addDeclaration(std::unique_ptr<Decl> decl);

  /// Retrives a non-owning reference to a declaration by \p name.
  NamedDecl *getDeclaration(const std::string &name) const;

  /// \returns Non-owning declarations of this context.
  std::vector<Decl *> getDeclarations() const;
};

/// Represents an import declaration. 
///
/// An import may be used to import a local source file or one from std library.
class ImportDecl final : public Decl {
  friend class ASTPrinter;
  friend class ReferenceAnalysis;

  /// The provided path of the package to import.
  const SourcePath path;

  /// If 1, the package is from the local source tree.
  const bool local : 1;

public:
  ImportDecl(const SourcePath &path, bool local, const Span &span);
  ~ImportDecl() = default;

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

  /// \returns The SourcePath of the package to import.
  const SourcePath &getPath() const { return path; }

  /// \returns The name of the package to import.
  const std::string &getName() const { return path.curr; }

  /// \returns 1 if the target package is from the local source tree.
  bool isLocal() const { return local; }
};

/// Represents a source file/package declaration. 
///
/// A package represents a singular source file, and may import the declarations 
/// of other local packages or libraries.
class PackageUnitDecl final : public DeclBase {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The unique name or identifier associated with this package.
  const std::string identifier;

  /// Declaration context for this package.
  DeclContext *ctx;

  /// The owned import declarations of this package.
  std::vector<std::unique_ptr<ImportDecl>> imports;

  /// Non-owning declarations contained in this package.
  std::vector<Decl *> decls;

  /// The scope of this package.
  Scope *scope;

public:
  PackageUnitDecl(const std::string &id, DeclContext *ctx, Scope *scope,
                  std::vector<std::unique_ptr<ImportDecl>> imports);
  ~PackageUnitDecl();

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

  /// \returns The identifier of this package unit.
  const std::string &getIdentifier() const { return identifier; }

  /// \returns The non-owning imports of this package.
  std::vector<ImportDecl *> getImports() const; 

  /// Adds a new, owned \p decl to this package unit.
  void addDecl(std::unique_ptr<Decl> decl);

  /// Imports a new \p decl to this package unit.
  void addDecl(Decl *decl);
};

} // end namespace artus

#endif // ARTUS_AST_DECLBASE_H
