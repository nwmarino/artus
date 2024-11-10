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
    trace("expected value", lastLoc);
    return nullptr;
  }

  return base;
}

/// Parse a primary expression.
std::unique_ptr<Expr> Parser::ParsePrimaryExpression() {
  if (tok.is(LiteralKind::Integer))
    return ParseIntegerExpression();
  
  return nullptr;
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
      parentFunctionType : ctx->getType("i32");

  return std::make_unique<IntegerLiteral>(
    std::stoi(intToken.value, 0, 10), T, false,
    createSpan(intToken.loc, intToken.loc));
}
