//>==- Lexer.h ------------------------------------------------------------==<//
//
// This header file declares an interface to lex a source stream into tokens
// recognized by the parser.
//
//>==--------------------------------------------------------------------==<//

#ifndef ARTUS_LEX_LEXER_H
#define ARTUS_LEX_LEXER_H

#include <string>

#include "Token.h"
#include "../Core/SourceLocation.h"

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

  /// Peeks at the proceeding \p n characters in the source stream.
  /// \returns The result of the peek.
  inline std::string peek(std::size_t n) const;

  /// \returns `true` if the lexer has reached the end of the source stream.
  inline bool isEof() const { return *BufferPos == '\0'; }

  /// \returns `true` if the next character is whitespace.
  inline bool isWhitespace() const { return *BufferPos == ' '; }

  /// \returns `true` if the next character is a newline character.
  inline bool isNewline() const { return *BufferPos == '\n'; }

public:
  /// Constructs a new lexer instance with the given source stream, starting
  /// at the origin of the stream.
  Lexer(const std::string &file, const char *BufferStart);

  /// Constructs a new lexer instance with the given source stream, starting
  /// at the provided location.
  Lexer(SourceLocation loc, const char *BufferPos);

  /// Advances the lexer and assigns the \p next token in the source stream.
  /// \returns If the lexer has reached the end of the stream.
  bool Lex(Token &next);

  /// \returns The most recent token in the source stream.
  Token lastToken() const { return previousToken; }

  /// Clears current data on \p next and assigns it the latest positional data.
  void initToken(Token &next);

  /// Dumps and stringifies the remaining source in this lexer instance.
  /// Once dumped, the lexer state cannot be restored.
  /// \returns The readable source dump.
  const std::string dump();
};

} // end namespace artus

#endif // ARTUS_LEX_LEXER_H
