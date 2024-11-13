#include <cassert>

#include "../../include/Core/Logger.h"
#include "../../include/Parse/Parser.h"

using namespace artus;

/// Parse a default initialization expression dependent on the given type.
///
/// Given the type, this function will return an expression that may be used
/// to initialize an otherwised undefined variable.
std::unique_ptr<Expr> Parser::ParseDefaultInitExpression(const Type *T) {
  if (T->isIntegerType()) {
    return std::make_unique<IntegerLiteral>(0, T, false, 
        createSpan(lastLoc));
  }

  return nullptr;
}

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
    return ParseIdentifierExpression();

  if (tok.is(LiteralKind::Integer))
    return ParseIntegerExpression();
  
  return nullptr;
}

/// Parse an identifier expression.
std::unique_ptr<Expr> Parser::ParseIdentifierExpression() {
  assert(tok.is(TokenKind::Identifier) && "expected identifier");

  if (Decl *refDecl = scope->getDecl(tok.value))
    return ParseReferenceExpression();

  // Variable (re)assignments.
  peekToken();
  if (peek.is(TokenKind::Equals))
    return ParseReferenceExpression();

  return ParseCastExpression();
}

/// Parse a declaration reference expression.
std::unique_ptr<Expr> Parser::ParseReferenceExpression() {
  Token identToken = tok; // Save the identifier token.
  nextToken();

  // Resolve the identifier reference.
  Decl *refDecl = scope->getDecl(identToken.value);
  if (!refDecl) {
    fatal("unresolved symbol: " + identToken.value, identToken.loc);
  }

  // Resolve the declaration type, if it exists.
  const Type *refType = nullptr;
  if (VarDecl *varDecl = dynamic_cast<VarDecl *>(refDecl)) {
    refType = varDecl->getType();
  } else if (ParamVarDecl *paramDecl = dynamic_cast<ParamVarDecl *>(refDecl)) {
    refType = paramDecl->getType();
  }

  return std::make_unique<DeclRefExpr>(identToken.value, refDecl, 
      refType, createSpan(identToken.loc));
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
