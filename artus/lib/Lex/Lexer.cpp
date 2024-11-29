//>==- Lexer.cpp ----------------------------------------------------------==<//
//
// The following source implements a lexer interface to tokenize source code.
//
//>==----------------------------------------------------------------------==<//

#include <format>
#include <cctype>

#include "../../include/Core/Logger.h"
#include "../../include/Core/SourceLocation.h"
#include "../../include/Lex/Lexer.h"

using namespace artus;

Lexer::Lexer(const std::string &file, const char *BufferStart) 
    : BufferPos(BufferStart), loc({ .file = file, .line = 1, .col = 1}), 
    previousToken({ .kind = TokenKind::Eof }), previousChar('\0') {}

Lexer::Lexer(SourceLocation loc, const char *BufferPos) 
    : BufferPos(BufferPos), loc(loc), previousToken({ .kind = TokenKind::Eof }),
    previousChar(*BufferPos != '\0' ? BufferPos[-1] : '\0') {}

std::string Lexer::peek(size_t n) const 
{ return std::string(BufferPos + 1, n); }

void Lexer::initToken(Token &next) {
  next.kind = TokenKind::Eof;
  next.value = "";
  next.literalKind = LiteralKind::None;
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

  std::string tmp;
  std::size_t skp = 1;
  switch (*BufferPos) {
  // Character literals.
  case '\'':
    next.kind = TokenKind::Literal;
    next.literalKind = LiteralKind::Character;

    BufferPos++;
    tmp = *BufferPos;

    BufferPos++;
    if (*BufferPos != '\'')
      fatal("expected closing single quote after character: " + tmp, loc);

    skp += 2;
    break;

  // String literals.
  case '"':
    next.kind = TokenKind::Literal;
    next.literalKind = LiteralKind::String;
    BufferPos++;

    // Lex the entire string literal.
    while (*BufferPos != '"') {
      if (isEof())
        fatal("expected closing double quote after string", loc);

      if (*BufferPos == '\\') {
        BufferPos++;
        switch (*BufferPos) {
        case 'n': 
          tmp += "\n"; 
          break;
        case 't':
          tmp += "\t";
          break;
        case '"':
          tmp += '"';
          break;
        default: 
          tmp += "\\"; 
          break;
        }
      } else
        tmp += *BufferPos;

      BufferPos++;
      skp++;
    }
    break;

  // Equals operators or equality tokens.
  case '=':
    if (peek(1) == "=") {
      BufferPos++;
      skp++;
      next.kind = TokenKind::EqualsEquals;
    } else if (peek(1) == ">") {
      BufferPos++;
      skp++;
      next.kind = TokenKind::FatArrow;
    } else
      next.kind = TokenKind::Equals;

    break;

  // Addition operators.
  case '+':
    if (peek(1) == "=") {
      BufferPos++;
      skp++;
      next.kind = TokenKind::PlusEquals;
    } else
      next.kind = TokenKind::Plus;

    break;

  // Subtraction operators or arrow tokens.
  case '-':
    if (peek(1) == ">") {
      BufferPos++;
      skp++;
      next.kind = TokenKind::Arrow;
    } else if (peek(1) == "=") {
      BufferPos++;
      skp++;
      next.kind = TokenKind::MinusEquals;
    } else
      next.kind = TokenKind::Minus;

    break;

  // Multiplication operators.
  case '*':
    if (peek(1) == "=") {
      BufferPos++;
      skp++;
      next.kind = TokenKind::StarEquals;
    } else
      next.kind = TokenKind::Star;

    break;

  // Division operators or line comments.
  case '/':
    if (peek(1) == "/") {
      while (*BufferPos != '\n') {
        BufferPos++;
        skp++;
      }
      goto entry;
    } else if (peek(1) == "=") {
      BufferPos++;
      skp++;
      next.kind = TokenKind::SlashEquals;
    } else
      next.kind = TokenKind::Slash;

    break;

  // Bang or not equals.
  case '!':
    if (peek(1) == "=") {
      BufferPos++;
      skp++;
      next.kind = TokenKind::BangEquals;
    } else
      next.kind = TokenKind::Bang;

    break;

  // Less than inequalities.
  case '<':
    if (peek(1) == "=") {
      BufferPos++;
      skp++;
      next.kind = TokenKind::LessEquals;
    } else
      next.kind = TokenKind::Less;

    break;

  // Greater than inequalities.
  case '>':
    if (peek(1) == "=") {
      BufferPos++;
      skp++;
      next.kind = TokenKind::GreaterEquals;
    } else
      next.kind = TokenKind::Greater;

    break;

  // Or operators.
  case '|':
    if (peek(1) == "|") {
      BufferPos++;
      skp++;
      next.kind = TokenKind::OrOr;
    }
    break;

  // And operators.
  case '&':
    if (peek(1) == "&") {
      BufferPos++;
      skp++;
      next.kind = TokenKind::AndAnd;
    } else
      next.kind = TokenKind::Ampersand;

    break;

  // Xor operators.
  case '^':
    if (peek(1) == "^") {
      BufferPos++;
      skp++;
      next.kind = TokenKind::XorXor;
    }
    break;

  // Colon or path operator.
  case ':':
    if (peek(1) == ":") {
      BufferPos++;
      skp++;
      next.kind = TokenKind::Path;
    } else 
      next.kind = TokenKind::Colon;

    break;

  // Basic token lexing.
  case '(': next.kind = TokenKind::OpenParen; break;
  case ')': next.kind = TokenKind::CloseParen; break;
  case '{': next.kind = TokenKind::OpenBrace; break;
  case '}': next.kind = TokenKind::CloseBrace; break;
  case '[': next.kind = TokenKind::OpenBracket; break;
  case ']': next.kind = TokenKind::CloseBracket; break;
  case '.': next.kind = TokenKind::Dot; break;
  case ',': next.kind = TokenKind::Comma; break;
  case '@': next.kind = TokenKind::At; break;
  case '#': next.kind = TokenKind::Hash; break;

  // Identifiers and numeric literals.
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
      while (isdigit(*BufferPos) || *BufferPos == '.') {
        if (*BufferPos == '.' && next.literalKind == LiteralKind::Integer) {
          next.literalKind = LiteralKind::Float;
        }
        tmp += *BufferPos++;
        skp++;
      }
      BufferPos--;
      skp--;
      break;
    }

    fatal(std::string("unresolved token: '") + *BufferPos + '\'', loc);
  } // end switch

  BufferPos++;
  loc.col += skp;
  next.value = tmp;
  previousToken = next;
  return 1;
}

const std::string Lexer::dump() {
  Token curr;
  std::string result;

  std::string tmp;
  std::string loc;
  while (Lex(curr)) {
    loc = curr.loc.file + ":" + std::to_string(curr.loc.line) + ":" 
          + std::to_string(curr.loc.col);

    switch (curr.kind) {
    case TokenKind::LineComment: 
      tmp = "LineComment"; 
      break;
    case TokenKind::Identifier: 
      tmp = "Identifier <" + curr.value + ">"; 
      break;
    case TokenKind::Literal: 
      tmp = "Literal <" + curr.value + ">"; 
      break;
    case TokenKind::OpenParen: 
      tmp = "("; 
      break;
    case TokenKind::CloseParen: 
      tmp = ")"; 
      break;
    case TokenKind::OpenBrace: 
      tmp = "{"; 
      break;
    case TokenKind::CloseBrace: 
      tmp = "}"; 
      break;
    case TokenKind::OpenBracket: 
      tmp = "["; 
      break;
    case TokenKind::CloseBracket: 
      tmp = "]"; 
      break;
    case TokenKind::Plus: 
      tmp = "+"; 
      break;
    case TokenKind::Minus: 
      tmp = "-"; 
      break;
    case TokenKind::Star: 
      tmp = "*"; 
      break;
    case TokenKind::Slash: 
      tmp = "/"; 
      break;
    case TokenKind::Equals: 
      tmp = "="; 
      break;
    case TokenKind::Bang: 
      tmp = "!"; 
      break;
    case TokenKind::Colon: 
      tmp = ":"; 
      break;
    case TokenKind::Dot: 
      tmp = "."; 
      break;
    case TokenKind::Comma: 
      tmp = ","; 
      break;
    case TokenKind::At: 
      tmp = "@"; 
      break;
    case TokenKind::Hash: 
      tmp = "#"; 
      break;
    case TokenKind::Ampersand: 
      tmp = "&"; 
      break;
    case TokenKind::Arrow: 
      tmp = "->"; 
      break;
    case TokenKind::FatArrow: 
      tmp = "=>"; 
      break;
    case TokenKind::EqualsEquals: 
      tmp = "=="; 
      break;
    case TokenKind::Less: 
      tmp = "<"; 
      break;
    case TokenKind::Greater: 
      tmp = ">"; 
      break;
    case TokenKind::BangEquals: 
      tmp = "!="; 
      break;
    case TokenKind::PlusEquals: 
      tmp = "+="; 
      break;
    case TokenKind::MinusEquals: 
      tmp = "-="; 
      break;
    case TokenKind::StarEquals: 
      tmp = "*="; 
      break;
    case TokenKind::SlashEquals: 
      tmp = "/="; 
      break;
    case TokenKind::LessEquals: 
      tmp = "<="; 
      break;
    case TokenKind::GreaterEquals: 
      tmp = ">="; 
      break;
    case TokenKind::AndAnd: 
      tmp = "&&"; 
      break;
    case TokenKind::OrOr: 
      tmp = "||"; 
      break;
    case TokenKind::XorXor: 
      tmp = "^^"; 
      break;
    case TokenKind::Path: 
      tmp = "::"; 
      break;
    case TokenKind::Eof: 
      tmp = "Eof"; 
      break;
    }

    result += std::format("{:15}{}\n", loc, tmp);
  }

  return result;
}
