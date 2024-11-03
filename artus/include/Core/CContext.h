#ifndef ARTUS_CORE_CCONTEXT_H
#define ARTUS_CORE_CCONTEXT_H

#include <map>
#include <string>
#include <vector>

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
  const char *BufferStart;
};

/// Context for the compiler used during analysis passes.
class CContext final {
  friend class BasicType;

  /// Most recent lexed token.
  Token currentToken;
  
  /// The token canonically previous to the current token.
  Token previousToken;

  /// Basic and source-defined types as discovered during parsing.
  mutable map<string, Type *> types;

  vector<SourceFile> files;

  /// A lexer instance used to tokenize the source code.
  Lexer *lexer;

  /// True if the lexer has reached the end of the source stream, and false
  /// otherwise.
  unsigned int eof : 1;

  /// Initializes the basic types for the context.
  void InitTypes() const;

  /// Initialize the lexer with the next in-line source file.
  void InitLexer();

public:
  CContext(vector<SourceFile> &files);

  /// Returns the most similar type to the provided name, or a TypeRef
  /// instance to a possibly qualified type if the type is not found.
  const Type *getType(const string &name) const;

  /// Returns the next token from the lexer. Consumes the previous token.
  const Token &next();

  /// Returns the currently lexed token. Does not discard the token.
  const Token &curr() const { return currentToken; }

  /// Returns the previously lexed token. Does not discard the token.
  const Token &prev() const { return previousToken; }
};

}; // namespace artus

#endif // ARTUS_CORE_CCONTEXT_H
