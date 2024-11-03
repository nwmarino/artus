#include <format>
#include <cctype>

#include "../../include/Core/Logger.h"
#include "../../include/Lex/Lexer.h"

using std::size_t;
using std::string;

using namespace artus;

Lexer::Lexer(const string &file, const char *BufferStart) 
    : BufferPos(BufferStart), loc(file, 1, 1), previousChar('\0'), 
    previousToken(Token(TokenKind::Eof, Span(file))) {}

Lexer::Lexer(SourceLocation loc, const char *BufferPos) 
    : BufferPos(BufferPos), loc(loc), 
    previousChar(*BufferPos != '\0' ? BufferPos[-1] : '\0'), 
    previousToken(Token(TokenKind::Eof, Span(loc.file))) {}

string Lexer::peek(size_t n) const { return string(BufferPos + 1, n); }

void Lexer::initToken(Token &next) {
  next.kind = TokenKind::Eof;
  next.value = "";
  next.span = Span(loc.file, loc.line, loc.col);
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

  /// Subtraction operators or arrow tokens.
  case '-':
    if (peek(1) == ">") {
      BufferPos++;
      skp++;
      next.kind = TokenKind::Arrow;
    }
    break;

  /// Division operators or line comments.
  case '/':
    if (peek(1) == "/") {
      BufferPos++;
      skp++;
      next.kind = TokenKind::LineComment;
    }
    break;

  /// Basic token lexing.
  case '(': next.kind = TokenKind::OpenParen; break;
  case ')': next.kind = TokenKind::CloseParen; break;
  case '{': next.kind = TokenKind::OpenBrace; break;
  case '}': next.kind = TokenKind::CloseBrace; break;
  case ':': next.kind = TokenKind::Colon; break;
  case '@': next.kind = TokenKind::At; break;

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
  string span;
  while (Lex(curr)) {
    span = curr.span.file + ":" + std::to_string(curr.span.line) +
        ":" + std::to_string(curr.span.col);

    switch (curr.kind) {
      case TokenKind::LineComment: tmp = "LineComment"; break;
      case TokenKind::Identifier: tmp = "Identifier <" + curr.value + ">"; break;
      case TokenKind::Literal: tmp = "Literal <" + curr.value + ">"; break;
      case TokenKind::OpenParen: tmp = "OpenParen"; break;
      case TokenKind::CloseParen: tmp = "CloseParen"; break;
      case TokenKind::OpenBrace: tmp = "OpenBrace"; break;
      case TokenKind::CloseBrace: tmp = "CloseBrace"; break;
      case TokenKind::Colon: tmp = "Colon"; break;
      case TokenKind::At: tmp = "At"; break;
      case TokenKind::Arrow: tmp = "Arrow"; break;
      case TokenKind::Eof: tmp = "Eof"; break;
    }

    result += std::format("{:15}{}\n", span, tmp);
  }

  return result;
}
