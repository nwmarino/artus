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
  "bool",
  "char",
  "string",
  "i32",
  "i64",
  "u8",
  "u32",
  "u64",
  "null",
  "true",
  "false",
  "fix",
  "fn",
  "jmp",
  "mut",
  "ret",
};

namespace artus {

/// Returns 1 if the provided identifier is a reserved keyword, and 0 otherwise.
inline int isReserved(const string &kw) {
  return std::find(__RESERVED.begin(), __RESERVED.end(), kw) \
    != __RESERVED.end();
}

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
  /// [
  OpenBracket,
  /// ]
  CloseBracket,
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
  /// !
  Bang,
  /// :
  Colon,
  /// ,
  Comma,
  /// @
  At,
  /// &
  Ampersand,

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

  /// a, b, ...
  Character,

  /// "a", "b", ...
  String,
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
    return kind == TokenKind::Identifier && value == kw && isReserved(kw); 
  }
};

} // namespace artus

#endif // ARTUS_LEX_TOKEN_H
