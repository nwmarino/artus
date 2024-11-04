#ifndef ARTUS_CORE_CCONTEXT_H
#define ARTUS_CORE_CCONTEXT_H

#include <map>
#include <memory>

#include "../Lex/Lexer.h"
#include "../Lex/Token.h"
#include "../Sema/Type.h"

using std::map;
using std::string;
using std::vector;

namespace artus {
 
/// Represents an input source file to the compiler.
struct SourceFile {
  const string name;
  const string path;
  const char *BufferStart;
};

/// Context used during analysis phases of the compilation process.
class Context final {
  friend class BasicType;
  friend class Parser;

  /// The source files to be compiled.
  vector<SourceFile> files;

  /// The currently active source file.
  SourceFile active;

  /// A lexer instance used to tokenize the source code.
  std::unique_ptr<Lexer> lexer;

  /// A map of all types in the current context.
  mutable map<string, const Type *> types;

  /// If the lexer has reached the end of the current source stream.
  unsigned int eof : 1;

public:
  Context(vector<SourceFile> files);

  /// Iterates to the next source file in the context.
  bool nextFile();

  /// Returns the type most similar to `name`. If no type is found, returns
  /// a type refernce to a possibly qualified type.
  const Type *getType(const string &name);

  /// Returns the name of the currently active source file.
  inline const string &getActiveFileName() const { return active.name; }

  /// Returns the path of the currently active source file.
  inline const string &getActiveFilePath() const { return active.path; }
};

}; // namespace artus

#endif // ARTUS_CORE_CCONTEXT_H
