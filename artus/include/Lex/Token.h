#ifndef ARTUS_LEX_TOKEN_H
#define ARTUS_LEX_TOKEN_H

#include <algorithm>
#include <cstddef>
#include <vector>

#include "../Core/SourceLocation.h"

using std::size_t;
using std::string;
using std::vector;

/// Reference for reserved identifiers in the language.
inline vector<string> __RESERVED = {
  "i32",
  "i64",
  "u8",
  "u32",
  "u64",
  "fix",
  "fn",
  "jmp",
  "mut",
  "ret",
};

/// Returns 1 if the provided identifier is a reserved keyword, and 0 otherwise.
inline int is_reserved(const string &kw) {
  return std::find(__RESERVED.begin(), __RESERVED.end(), kw) \
    != __RESERVED.end();
}

namespace artus {

/// The different kinds of tokens that the lexer can produce.
enum class TokenKind {
  /// // Line Comments
  LineComment = 0,

  /// Identifiers
  Identifier,

  /// Literal Expressions
  Literal,

  /// Singular Tokens
  /// (
  OpenParen,
  /// )
  CloseParen,
  /// {
  OpenBrace,
  /// }
  CloseBrace,
  /// +
  Plus,
  /// -
  Minus,
  /// *
  Star,
  /// /
  Slash,
  /// =
  Equals,
  /// :
  Colon,
  /// @
  At,

  /// Compound Tokens
  /// ->
  Arrow,
  /// ==
  EqualsEquals,

  /// End of file
  Eof,
};

/// The different kinds of literal lexemmes that the lexer can produce.
enum class LiteralKind {
  /// 0, 1, ...
  Integer = 0,
};

/// A token produced by the lexer.
struct Token {
  /// The kind of lexmme represented by this token.
  TokenKind kind;

  /// Positional information about the token in the source code.
  SourceLocation loc;

  /// The optional, embedded value in this token.
  string value;

  /// The optional, literal kind represented by this token.
  LiteralKind literalKind;

  /// Returns true if the token is of the provided kind, and false otherwise.
  inline bool is(TokenKind k) const { return kind == k; }

  /// Returns true if the token is a literal of the provided kind, and false 
  /// otherwise.
  inline bool is(LiteralKind lk) const { return kind == TokenKind::Literal && \
    literalKind == lk; }

  /// Returns true if the token is a reserved keyword, and false otherwise.
  inline bool isKeyword(const string &kw) const { 
    return kind == TokenKind::Identifier && value == kw && is_reserved(kw); 
  }
};

} // namespace artus

#endif // ARTUS_LEX_TOKEN_H
