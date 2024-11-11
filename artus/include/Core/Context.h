#ifndef ARTUS_CORE_CONTEXT_H
#define ARTUS_CORE_CONTEXT_H

#include <map>
#include <memory>

#include "llvm/Target/TargetMachine.h"

#include "Input.h"
#include "UnitCache.h"
#include "../AST/DeclBase.h"
#include "../Lex/Lexer.h"
#include "../Lex/Token.h"
#include "../Sema/Type.h"

using std::map;
using std::string;
using std::vector;

namespace artus {

/// Forward declarations.
class PackageUnitDecl;

/// Context used during analysis phases of the compilation process.
class Context final {
  friend class BasicType;
  friend class Codegen;
  friend class Driver;
  friend class Parser;
  friend class Sema;

  /// The source files to be compiled.
  vector<SourceFile> files;

  /// The currently active source file.
  SourceFile active;

  /// A lexer instance used to tokenize the source code.
  std::unique_ptr<Lexer> lexer;

  /// A list of parsed package units.
  std::unique_ptr<UnitCache> cache;

  /// A map of all types in the current context.
  mutable map<string, const Type *> types;

  /// If the lexer has reached the end of the current source stream.
  unsigned int eof : 1;

public:
  Context(vector<SourceFile> files);

  /// Iterates to the next source file in the context.
  bool nextFile();

  /// Adds a package unit to the lifetime of this context.
  void addPackage(std::unique_ptr<PackageUnitDecl> pkg);

  /// Returns the type most similar to `name`. If no type is found, returns
  /// a type refernce to a possibly qualified type.
  const Type *getType(const string &name);

  /// Returns the name of the currently active source file.
  inline const string &getActiveFileName() const { return active.name; }

  /// Returns the path of the currently active source file.
  inline const string &getActiveFilePath() const { return active.path; }

  /// Prints the current state of the AST embedded in this context.
  void printAST();
};

}; // namespace artus

#endif // ARTUS_CORE_CONTEXT_H
