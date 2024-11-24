#include "../../include/AST/Stmt.h"
#include "../../include/Parse/Parser.h"
#include "../../include/Sema/Type.h"

using namespace artus;

bool Parser::nextToken() {
  if (peeking) {
    tok = peek;
    lastLoc = peek.loc;
    peeking = 0;
    return false;
  }

  if (ctx->lexer->Lex(tok)) {
    lastLoc = { ctx->getActiveFileName(), tok.loc.line, 
                tok.loc.col };
    return false;
  }

  return ctx->nextFile();
}

bool Parser::peekToken() 
{ return !peeking && ctx->lexer->Lex(peek) ? ++peeking : false; }

const Span Parser::createSpan(const SourceLocation &firstLoc) const {
  return { .file = firstLoc.file, .line =  firstLoc.line, 
            .col =  firstLoc.col, .line_nd = lastLoc.line, 
            .col_nd =  lastLoc.col };
}

const Span Parser::createSpan(const SourceLocation &firstLoc, 
                              const SourceLocation &lastLoc) const {
  return { .file = firstLoc.file, .line =  firstLoc.line, 
            .col =  firstLoc.col, .line_nd = lastLoc.line, 
            .col_nd =  lastLoc.col };
}

void Parser::exitScope() { this->scope = scope->getParent(); }

void Parser::enterScope(const ScopeContext &ctx) 
{ this->scope = new Scope(this->scope, {}, ctx); }

int Parser::getPrecedence() const {
  switch (tok.kind) {
    case TokenKind::Star:
    case TokenKind::Slash:
      return 6;
    case TokenKind::Plus:
    case TokenKind::Minus:
      return 5;
    case TokenKind::Less:
    case TokenKind::Greater:
    case TokenKind::LessEquals:
    case TokenKind::GreaterEquals:
      return 4;
    case TokenKind::EqualsEquals:
    case TokenKind::BangEquals:
      return 3;
    case TokenKind::AndAnd:
    case TokenKind::OrOr:
    case TokenKind::XorXor:
      return 2;
    case TokenKind::Equals:
    case TokenKind::PlusEquals:
    case TokenKind::MinusEquals:
    case TokenKind::StarEquals:
    case TokenKind::SlashEquals:
      return 1;
    default: 
      return -1;
  }

  return -1;
}

UnaryExpr::UnaryOp Parser::getUnaryOp() const {
  switch (tok.kind) {
    case TokenKind::Minus: 
      return UnaryExpr::UnaryOp::Negative;
    case TokenKind::Bang: 
      return UnaryExpr::UnaryOp::Not;
    case TokenKind::Ampersand:
      return UnaryExpr::UnaryOp::Ref;
    case TokenKind::Hash:
      return UnaryExpr::UnaryOp::DeRef;
    default: 
      return UnaryExpr::UnaryOp::Unknown;
  }
}

BinaryExpr::BinaryOp Parser::getBinaryOp() const {
  switch (tok.kind) {
    case TokenKind::Equals: 
      return BinaryExpr::BinaryOp::Assign;
    case TokenKind::PlusEquals:
      return BinaryExpr::BinaryOp::AddAssign;
    case TokenKind::MinusEquals:
      return BinaryExpr::BinaryOp::SubAssign;
    case TokenKind::StarEquals:
      return BinaryExpr::BinaryOp::MultAssign;
    case TokenKind::SlashEquals:
      return BinaryExpr::BinaryOp::DivAssign;
    case TokenKind::EqualsEquals:
      return BinaryExpr::BinaryOp::Equals;
    case TokenKind::BangEquals:
      return BinaryExpr::BinaryOp::NotEquals;
    case TokenKind::Less:
      return BinaryExpr::BinaryOp::LessThan;
    case TokenKind::Greater:
      return BinaryExpr::BinaryOp::GreaterThan;
    case TokenKind::LessEquals:
      return BinaryExpr::BinaryOp::LessEquals;
    case TokenKind::GreaterEquals:
      return BinaryExpr::BinaryOp::GreaterEquals;
    case TokenKind::AndAnd:
      return BinaryExpr::BinaryOp::LogicalAnd;
    case TokenKind::OrOr:
      return BinaryExpr::BinaryOp::LogicalOr;
    case TokenKind::XorXor:
      return BinaryExpr::BinaryOp::LogicalXor;
    case TokenKind::Plus:
      return BinaryExpr::BinaryOp::Add;
    case TokenKind::Minus:
      return BinaryExpr::BinaryOp::Sub;
    case TokenKind::Star:
      return BinaryExpr::BinaryOp::Mult;
    case TokenKind::Slash:
      return BinaryExpr::BinaryOp::Div;
    default:
      return BinaryExpr::BinaryOp::Unknown;
  }
}

/// Parses a defined type reference. For example, `#int` or `char[5]`.
///
/// This function will parse a type reference, which can be a basic type or a
/// user-defined type. Implicitly defined types such as FunctionTypes cannot be 
/// parsed here.
const Type *Parser::ParseType() {
  // Parse ptr reference levels.
  string typeIdentifier;
  while (tok.is(TokenKind::Hash)) {
    typeIdentifier.append("#");
    nextToken(); // Consume the '*' token.
  }

  assert(tok.is(TokenKind::Identifier) && "expected identifier");

  const Token identToken = tok;
  nextToken(); // Consume the identifier token.

  typeIdentifier.append(identToken.value);

  if (!tok.is(TokenKind::OpenBracket))
    return ctx->getType(typeIdentifier);

  nextToken(); // Consume the '[' token.

  if (!tok.is(LiteralKind::Integer)) {
    fatal("expected constant integer to define array size", lastLoc);
  }

  const string size = tok.value;
  nextToken(); // Consume the integer token.

  if (!tok.is(TokenKind::CloseBracket)) {
    fatal("expected ']' after array type", lastLoc);
  }
  nextToken(); // Consume the ']' token.

  return ctx->getType(typeIdentifier + "[" + size + "]");
}
