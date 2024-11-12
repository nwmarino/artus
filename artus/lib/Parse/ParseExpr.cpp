#include <cassert>

#include "../../include/Core/Logger.h"
#include "../../include/Parse/Parser.h"

using namespace artus;

/// Parse an expression.
///
/// expr:
///   <primary-expr>
///   <expr> binary-op <expr>
///   unary-op <expr>
std::unique_ptr<Expr> Parser::ParseExpression() {
  std::unique_ptr<Expr> base = ParsePrimaryExpression();
  if (!base) {
    return nullptr;
  }

  return ParseBinaryExpression(std::move(base));
}

/// Parse a primary expression.
std::unique_ptr<Expr> Parser::ParsePrimaryExpression() {
  if (tok.is(TokenKind::Identifier))
    return ParseCastExpression();

  if (tok.is(LiteralKind::Integer))
    return ParseIntegerExpression();
  
  return nullptr;
}

/// Parse a cast expression.
std::unique_ptr<Expr> Parser::ParseCastExpression() {
  assert(tok.is(TokenKind::Identifier) && "expected identifier");

  /// UNRECOVERABLE: Cannot cast an expression that is already under a cast.
  if (isUnderCast) {
    fatal("cannot cast an expression already under a cast", tok.loc);
  }

  Token idToken = tok; // Save the identifier token.
  nextToken();

  // Resolve the cast type.
  const Type *castType = ctx->getType(idToken.value);
  
  // Resolve the base expression.
  isUnderCast = 1;
  std::unique_ptr<Expr> baseExpr = ParseExpression();
  if (!baseExpr) {
    fatal("expected expression after cast: " + castType->toString(), 
        idToken.loc);
  }

  isUnderCast = 0;
  return std::make_unique<ExplicitCastExpr>(std::move(baseExpr), 
      castType, createSpan(idToken.loc));
}

/// Parse a binary expression.
///
/// expr:
///   <expr> binary-op <expr>
std::unique_ptr<Expr> Parser::ParseBinaryExpression(std::unique_ptr<Expr> base,
                                                    int precedence) {
  while (true) {
    int tokenPrecedence = getPrecedence();
    if (tokenPrecedence < precedence)
      return base;

    BinaryExpr::BinaryOp op = getBinaryOp();
    if (op == BinaryExpr::BinaryOp::Unknown) {
      fatal("unresolved binary operator", tok.loc);
    }

    nextToken(); // Eat the operator token.

    std::unique_ptr<Expr> rhs = ParsePrimaryExpression();
    if (!rhs) {
      fatal("expected expression after binary operator", tok.loc);
    }

    int nextPrecedence = getPrecedence();
    if (tokenPrecedence < nextPrecedence) {
      rhs = ParseBinaryExpression(std::move(rhs), tokenPrecedence + 1);
      if (!rhs) {
        fatal("expected expression after binary operator", tok.loc);
      }
    }
    
    base = std::make_unique<BinaryExpr>(std::move(base), std::move(rhs), 
        op, createSpan({ base->getSpan().file, base->getSpan().line,
        base->getSpan().col }, lastLoc));
  }
}

/// Parse a numerical literal expression.
///
/// Expects the current token to be an integer literal.
std::unique_ptr<Expr> Parser::ParseIntegerExpression() {
  assert(tok.is(LiteralKind::Integer) && "expected integer literal");

  Token intToken = tok; // Save the integer token.
  nextToken();

  // Determine the type of the integer literal.
  const Type *T = parentFunctionType && parentFunctionType->isIntegerType() ? \
      parentFunctionType->getReturnType() : ctx->getType("i32");

  return std::make_unique<IntegerLiteral>(
    std::stoi(intToken.value, 0, 10), T, false,
    createSpan(intToken.loc, intToken.loc));
}
