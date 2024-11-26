//>==- Token.h ------------------------------------------------------------==<//
//
// This header file defines the different kinds of tokens that the lexer can
// produce, as well as the different kinds of literal values recognized by the
// lexer.
//
//>==----------------------------------------------------------------------==<//

#ifndef ARTUS_LEX_TOKEN_H
#define ARTUS_LEX_TOKEN_H

#include <algorithm>
#include <string>
#include <vector>

#include "../Core/SourceLocation.h"

/// Reference for reserved identifiers in the language.
inline std::vector<std::string> __RESERVED = {
  // Type-related literals and identifiers.
  "bool",
  "char",
  "str",
  "f64",
  "i32",
  "i64",
  "u8",
  "u32",
  "u64",
  "null",
  "true",
  "false",

  // Keywords.
  "break",
  "continue",
  "else",
  "enum",
  "fix",
  "fn",
  "if",
  "import",
  "match",
  "mut",
  "priv",
  "ret",
  "struct",
  "while",
  "until",
};

namespace artus {

/// Returns 1 if the provided identifier is a reserved keyword, and 0 otherwise.
inline int isReserved(const std::string &kw) {
  return std::find(__RESERVED.begin(), __RESERVED.end(), kw)
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
  /// .
  Dot,
  /// ,
  Comma,
  /// @
  At,
  /// #
  Hash,
  /// &
  Ampersand,
  /// <
  Less,
  /// >
  Greater,

  /// Compound Tokens
  /// ->
  Arrow,
  /// =>
  FatArrow,
  /// ==
  EqualsEquals,
  /// !=
  BangEquals,
  /// +=
  PlusEquals,
  /// -=
  MinusEquals,
  /// *=
  StarEquals,
  /// /=
  SlashEquals,
  /// <=
  LessEquals,
  /// >=
  GreaterEquals,
  /// &&
  AndAnd,
  /// ||
  OrOr,
  /// ^^
  XorXor,
  /// ::
  Path,

  /// End of file
  Eof,
};

/// The different kinds of literal lexemmes that the lexer can produce.
enum class LiteralKind {
  None = -1,

  /// 0, 1, ...
  Integer,

  /// 0.1, 3.14, ...
  Float,

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
  std::string value;

  /// The optional, literal kind represented by this token.
  LiteralKind literalKind;

  /// \returns `true` if the token is of the provided kind.
  inline bool is(TokenKind k) const { return kind == k; }

  /// \returns `true` if the token is a literal of the provided kind.
  inline bool is(LiteralKind lk) const 
  { return kind == TokenKind::Literal && literalKind == lk; }

  /// \returns `true` if the token is a reserved keyword.
  inline bool isKeyword(const std::string &kw) const 
  { return kind == TokenKind::Identifier && value == kw && isReserved(kw); }
};

} // end namespace artus

#endif // ARTUS_LEX_TOKEN_H
