#ifndef ARTUS_LEX_LEXER_H
#define ARTUS_LEX_LEXER_H

#include "../Core/Span.h"
#include "Token.h"

using std::size_t;
using std::string;

namespace artus {

/// A lexer interface for tokenizing source code.
class Lexer {
  /// Current character position in the source stream.
  const char *BufferPos;

  /// Current location in the source code.
  SourceLocation loc;

  /// Most recent character in the source stream.
  unsigned char previousChar;

  /// Most recent token in the source stream.
  Token previousToken;

  /// Peeks at the proceeding n characters in the source stream. Does not modify
  /// the current state of the lexer.
  inline string peek(size_t n) const;

  /// Returns true if the lexer has reached the end of the source stream, and
  /// false otherwise.
  inline bool isEof() const { return *BufferPos == '\0'; }

  /// Returns true if the next character is whitespace, and false otherwise.
  inline bool isWhitespace() const { return *BufferPos == ' '; }

  /// Returns true if the next character is a newline character, and false
  /// otherwise.
  inline bool isNewline() const { return *BufferPos == '\n'; }

public:
  /// Constructs a new lexer instance with the given source stream, starting
  /// at the origin of the stream.
  Lexer(const string &file, const char *BufferStart);

  /// Constructs a new lexer instance with the given source stream, starting
  /// at the provided location.
  Lexer(SourceLocation loc, const char *BufferPos);

  /// Advances the lexer and assigns the next token in the source stream.
  /// Returns if the lexer has reached the end of the stream.
  bool Lex(Token &next);

  /// Returns the most recent token in the source stream. Does not advance the
  /// lexer, nor does it consume the most recent token.
  Token lastToken() const { return previousToken; }

  /// Clears current data on a token and assigns it the current positional data.
  void initToken(Token &next);

  /// Dumps and stringifies the remaining source in this lexer instance.
  /// Once dumped, the lexer state cannot be restored.
  const string dump();
};

} // namespace artus

#endif // ARTUS_LEX_LEXER_H
