#include <format>
#include <cctype>

#include "../../include/Core/Logger.h"
#include "../../include/Core/Span.h"
#include "../../include/Lex/Lexer.h"

using std::size_t;
using std::string;

using namespace artus;

Lexer::Lexer(const string &file, const char *BufferStart) 
    : BufferPos(BufferStart), loc({ .file = file, .line = 1, .col = 1}), 
    previousToken({ .kind = TokenKind::Eof }), previousChar('\0') {}

Lexer::Lexer(SourceLocation loc, const char *BufferPos) 
    : BufferPos(BufferPos), loc(loc), previousToken({ .kind = TokenKind::Eof }),
    previousChar(*BufferPos != '\0' ? BufferPos[-1] : '\0') {}

string Lexer::peek(size_t n) const { return string(BufferPos + 1, n); }

void Lexer::initToken(Token &next) {
  next.kind = TokenKind::Eof;
  next.value = "";
  next.loc = loc;
}

bool Lexer::Lex(Token &next) {
entry:
  initToken(next);

  if (isEof()) {
    next.kind = TokenKind::Eof;
    return 0;
  }

  if (isWhitespace()) {
    do {
      BufferPos++;
      loc.col++;
    } while (isWhitespace());
    goto entry;
  }

  if (isNewline()) {
    loc.line++;
    loc.col = 1;
    BufferPos++;
    goto entry;
  }

  string tmp;
  size_t skp = 1;
  switch (*BufferPos) {

  /// Character literals.
  case '\'':
    next.kind = TokenKind::Literal;
    next.literalKind = LiteralKind::Character;
    BufferPos++;
    tmp = *BufferPos;
    BufferPos++;
    if (*BufferPos != '\'') {
      fatal("expected closing single quote after character: " + tmp, loc);
    }
    break;

  /// String literals.
  case '"':
    next.kind = TokenKind::Literal;
    next.literalKind = LiteralKind::String;
    BufferPos++;
    while (*BufferPos != '"') {
      if (isEof()) {
        fatal("expected closing double quote after string", loc);
      }
      tmp += *BufferPos++;
      skp++;
    }
    break;

  /// Equals operators or equality tokens.
  case '=':
    if (peek(1) == "=") {
      BufferPos++;
      skp++;
      next.kind = TokenKind::EqualsEquals;
    } else {
      next.kind = TokenKind::Equals;
    }
    break;

  /// Subtraction operators or arrow tokens.
  case '-':
    if (peek(1) == ">") {
      BufferPos++;
      skp++;
      next.kind = TokenKind::Arrow;
    } else {
      next.kind = TokenKind::Minus;
    }
    break;

  /// Division operators or line comments.
  case '/':
    if (peek(1) == "/") {
      BufferPos++;
      skp++;
      next.kind = TokenKind::LineComment;
    } else {
      next.kind = TokenKind::Slash;
    }
    break;

  /// Bang or not equals.
  case '!':
    /*
    if (peek(1) == "=") {
      BufferPos++;
      skp++;
      next.kind = TokenKind::NotEquals;
    }*/
    next.kind = TokenKind::Bang;
    break;

  /// Basic token lexing.
  case '(': next.kind = TokenKind::OpenParen; break;
  case ')': next.kind = TokenKind::CloseParen; break;
  case '{': next.kind = TokenKind::OpenBrace; break;
  case '}': next.kind = TokenKind::CloseBrace; break;
  case '[': next.kind = TokenKind::OpenBracket; break;
  case ']': next.kind = TokenKind::CloseBracket; break;
  case '+': next.kind = TokenKind::Plus; break;
  case '*': next.kind = TokenKind::Star; break;
  case ':': next.kind = TokenKind::Colon; break;
  case ',': next.kind = TokenKind::Comma; break;
  case '@': next.kind = TokenKind::At; break;
  case '&': next.kind = TokenKind::Ampersand; break;

  /// Identifiers and numeric literals.
  default:
    if (isalpha(*BufferPos) || *BufferPos == '_') {
      next.kind = TokenKind::Identifier;
      while (isalnum(*BufferPos) || *BufferPos == '_') {
        tmp += *BufferPos++;
        skp++;
      }
      BufferPos--;
      skp--;
      break;
    } else if (isdigit(*BufferPos)) {
      next.kind = TokenKind::Literal;
      next.literalKind = LiteralKind::Integer;
      while (isdigit(*BufferPos)) {
        tmp += *BufferPos++;
        skp++;
      }
      BufferPos--;
      skp--;
      break;
    }

    /// Unresolved token.
    fatal(std::string("unresolved token: '") + *BufferPos + '\'', loc);
  } // end switch

  BufferPos++;
  loc.col += skp;
  next.value = tmp;
  previousToken = next;
  return 1;
}

const string Lexer::dump() {
  Token curr;
  string result;

  string tmp;
  string loc;
  while (Lex(curr)) {
    loc = curr.loc.file + ":" + std::to_string(curr.loc.line) + ":" 
          + std::to_string(curr.loc.col);

    switch (curr.kind) {
      case TokenKind::LineComment: tmp = "LineComment"; break;
      case TokenKind::Identifier: tmp = "Identifier <" + curr.value + ">"; break;
      case TokenKind::Literal: tmp = "Literal <" + curr.value + ">"; break;
      case TokenKind::OpenParen: tmp = "OpenParen"; break;
      case TokenKind::CloseParen: tmp = "CloseParen"; break;
      case TokenKind::OpenBrace: tmp = "OpenBrace"; break;
      case TokenKind::CloseBrace: tmp = "CloseBrace"; break;
      case TokenKind::OpenBracket: tmp = "OpenBracket"; break;
      case TokenKind::CloseBracket: tmp = "CloseBracket"; break;
      case TokenKind::Plus: tmp = "Plus"; break;
      case TokenKind::Minus: tmp = "Minus"; break;
      case TokenKind::Star: tmp = "Star"; break;
      case TokenKind::Slash: tmp = "Slash"; break;
      case TokenKind::Equals: tmp = "Equals"; break;
      case TokenKind::Bang: tmp = "Bang"; break;
      case TokenKind::Colon: tmp = "Colon"; break;
      case TokenKind::Comma: tmp = "Comma"; break;
      case TokenKind::At: tmp = "At"; break;
      case TokenKind::Ampersand: tmp = "Ampersand"; break;
      case TokenKind::Arrow: tmp = "Arrow"; break;
      case TokenKind::EqualsEquals: tmp = "EqualsEquals"; break;
      case TokenKind::Eof: tmp = "Eof"; break;
    }

    result += std::format("{:15}{}\n", loc, tmp);
  }

  return result;
}
