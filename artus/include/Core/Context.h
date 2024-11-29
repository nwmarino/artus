//>==- Context.h ----------------------------------------------------------==<//
//
// This header file declares an important context object used to organize
// information of the abstract syntax tree during compilation.
//
//>==----------------------------------------------------------------------==<//

#ifndef ARTUS_CORE_CONTEXT_H
#define ARTUS_CORE_CONTEXT_H

#include <unordered_map>
#include <memory>
#include <string>
#include <vector>

#include "Input.h"
#include "PackageManager.h"
#include "../AST/Decl.h"
#include "../AST/DeclBase.h"
#include "../Lex/Lexer.h"
#include "../Sema/Type.h"

namespace artus {

/// Forward declarations.
class PackageUnitDecl;

/// Context used during analysis phases of the compilation process.
class Context final {
  friend class BasicType;
  friend class Codegen;
  friend class Driver;
  friend class Parser;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The source files to be compiled.
  std::vector<SourceFile> files;

  /// The currently active source file.
  SourceFile active;

  /// A lexer instance used to tokenize the source code.
  std::unique_ptr<Lexer> lexer;

  /// A list of parsed package units.
  std::unique_ptr<PackageManager> PM;

  /// A map of all built-in, owned types.
  std::unordered_map<std::string, const Type *> bTyTable;

  /// A map of all defined, non-owning types in the current context.
  std::map<std::string, const Type *> tyTable;

  /// A map of built-in libraries.
  std::map<std::string, std::unique_ptr<PackageUnitDecl>> stdTable;

  /// If 1, then the lexer has reached the end of the current source stream.
  unsigned int eof : 1;

  /// If 1, then AST passes have found a main function.
  unsigned int foundEntry : 1;

  /// Resets the defined type table for a new package.
  void resetTypes();

  /// Add a new defined type (struct/enum) to the context. Raises a fatal error
  /// at location \p loc if the type is already defined.
  void addDefinedType(const std::string &name, const Type *T, 
                      const SourceLocation &loc);

  /// \returns The `print` function for std io, and adds it to scope \p ioScope.
  std::unique_ptr<NamedDecl> getPrintFunction(Scope *ioScope);

  /// \returns The `println` function for std io, and adds it to scope \p ioScope.
  std::unique_ptr<NamedDecl> getPrintlnFunction(Scope *ioScope);

  /// \returns The `readln` function for std oi, and adds it to scope \p ioScope.
  std::unique_ptr<NamedDecl> getReadlnFunction(Scope *ioScope);

public:
  Context(std::vector<SourceFile> files);
  ~Context();

  /// Iterates to the next source file in the context. \returns `true` if
  /// there is a next file, and `false` otherwise.
  bool nextFile();

  /// Adds the \p pkg to the lifetime of this context.
  void addPackage(std::unique_ptr<PackageUnitDecl> pkg);

  /// Resolves a package unit by its \p id. This function will raise an
  /// exception on location \p loc if the package is unresolved.
  PackageUnitDecl *resolvePackage(const std::string &id, 
                                  const SourceLocation &loc) const;

  /// \returns The type most similar to `name`. If no type is found, returns
  /// a type refernce to a possibly qualified type.
  const Type *getType(const std::string &name);

  /// \returns The name of the currently active source file.
  inline const std::string &getActiveFileName() const 
  { return active.name; }

  /// \returns The full name of the currently active source file.
  inline const std::string &getFullActiveFileName() const 
  { return active.filename; }

  /// \returns The path of the currently active source file.
  inline const std::string &getActiveFilePath() const 
  { return active.path; }

  /// Prints the current state of the AST embedded in this context.
  void printAST();
};

}; // end namespace artus

#endif // ARTUS_CORE_CONTEXT_H
