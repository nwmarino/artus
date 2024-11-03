#ifndef ARTUS_LEX_TOKEN_H
#define ARTUS_LEX_TOKEN_H

#include <algorithm>
#include <cstddef>
#include <vector>

#include "../Core/Span.h"

using std::find;
using std::size_t;
using std::string;
using std::vector;

/// Reference for reserved identifiers in the language.
inline vector<string> __RESERVED = {
  "fn",
  "function",
  "i32",
  "ret",
};

/// Returns 1 if the provided identifier is a reserved keyword, and 0 otherwise.
inline int is_reserved(const string &kw) {
  return find(__RESERVED.begin(), __RESERVED.end(), kw) != __RESERVED.end();
}

namespace artus {

/// The different kinds of tokens that the lexer can produce.
enum class TokenKind {
  /// // Line Comments
  LineComment,

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
  /// :
  Colon,
  /// @
  At,

  /// Compound Tokens
  /// ->
  Arrow,

  /// End of file
  Eof,
};

/// The different kinds of literal lexemmes that the lexer can produce.
enum class LiteralKind {
  /// 0, 1, ...
  Integer,
};

/// A token produced by the lexer.
struct Token {
  /// The kind of lexmme represented by this token.
  TokenKind kind;

  /// Positional information about the token in the source code.
  Span span;

  /// The optional, embedded value in this token.
  string value;

  /// The optional, literal kind represented by this token.
  LiteralKind literalKind;

  inline int is_ident() const { return kind == TokenKind::Identifier; }
  inline int is_keyword(const string &kw) const { return kind == TokenKind::Identifier && value == kw && is_reserved(kw); }
  inline int is_integer() const { return kind == TokenKind::Literal && literalKind == LiteralKind::Integer; }
};

} // namespace artus

#endif // ARTUS_LEX_TOKEN_H
