#include <cassert>
#include <memory>

#include "../../include/Core/Logger.h"
#include "../../include/Parse/Parser.h"
#include "../../include/Lex/Token.h"
#include "../../include/Core/SourceLocation.h"

using namespace artus;

/// Parse a default initialization expression dependent on the given type.
///
/// Given the type, this function will return an expression that may be used
/// to initialize an otherwised undefined variable.
std::unique_ptr<Expr> Parser::ParseDefaultInitExpression(const Type *T) {
  // Handle basic types.
  if (const BasicType *BT = dynamic_cast<const BasicType *>(T)) {
    switch (BT->getKind()) {
      case BasicType::INT1:
        return std::make_unique<BooleanLiteral>(0, T, createSpan(lastLoc));
      case BasicType::INT8:
        return std::make_unique<CharLiteral>(0, T, createSpan(lastLoc));
      case BasicType::INT32:
      case BasicType::INT64:
        return std::make_unique<IntegerLiteral>(0, T, true,
            createSpan(lastLoc));
      case BasicType::UINT8:
      case BasicType::UINT32:
      case BasicType::UINT64:
        return std::make_unique<IntegerLiteral>(0, T, false,
            createSpan(lastLoc));
      case BasicType::FP64:
        return nullptr;
    }
  }

  if (const ArrayType *AT = dynamic_cast<const ArrayType *>(T)) {
    vector<std::unique_ptr<Expr>> exprs = {};
    for (unsigned idx = 0; idx < AT->getSize(); ++idx) {
      exprs.push_back(ParseDefaultInitExpression(AT->getElementType()));
    }

    return std::make_unique<ArrayInitExpr>(std::move(exprs), T, 
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
  if (tok.is(TokenKind::At))
    return ParseCallExpression();

  if (tok.is(TokenKind::Identifier)) {
    if (tok.isKeyword("true") || tok.isKeyword("false"))
      return ParseBooleanExpression();

    return ParseIdentifierExpression();
  }

  if (tok.is(LiteralKind::Integer))
    return ParseIntegerExpression();
  else if (tok.is(LiteralKind::Character))
    return ParseCharacterExpression();
  else if (tok.is(LiteralKind::String))
    return ParseStringExpression();
  
  return ParseUnaryExpression();
}

/// Parse an identifier expression.
std::unique_ptr<Expr> Parser::ParseIdentifierExpression() {
  assert(tok.is(TokenKind::Identifier) && "expected identifier");

  peekToken();
  if (peek.is(TokenKind::OpenBracket))
    return ParseArrayAccessExpression();
  else if (peek.is(TokenKind::Equals))
    return ParseReferenceExpression();

  if (Decl *refDecl = scope->getDecl(tok.value))
    return ParseReferenceExpression();

  return ParseCastExpression();
}

/// Parse a function call expression.
///
/// call-expr:
///   '@' <identifier> '(' <expr-list> ')'
std::unique_ptr<Expr> Parser::ParseCallExpression() {
  assert(tok.is(TokenKind::At) && "expected '@'");

  SourceLocation firstLoc = tok.loc;
  nextToken(); // Eat the '@' token.

  if (!tok.is(TokenKind::Identifier)) {
    fatal("expected identifier after '@'", tok.loc);
  }

  const string callee = tok.value; // Save the callee.
  nextToken(); // Eat the identifier token.

  vector<std::unique_ptr<Expr>> args = {};
  bool emptyCall = false;
  // Empty call case: `@foo`; no arguments.
  if (!tok.is(TokenKind::OpenParen)) {
    emptyCall = true;
  }

  if (!emptyCall)
    nextToken(); // Eat the '(' token.

  while (!tok.is(TokenKind::CloseParen) && !emptyCall) {
    std::unique_ptr<Expr> arg = ParseExpression();
    if (!arg) {
      fatal("expected expression in argument list at call: " + callee, tok.loc);
    }

    args.push_back(std::move(arg));

    // Expect a terminator or another argument.
    if (tok.is(TokenKind::Comma)) {
      nextToken(); // Eat the ',' token.
    } else if (!tok.is(TokenKind::CloseParen)) {
      fatal("expected ')' after argument list", tok.loc);
    }
  }

  if (!emptyCall)
    nextToken(); // Eat the ')' token.

  return std::make_unique<CallExpr>(callee, nullptr, nullptr, std::move(args),
      createSpan(firstLoc, lastLoc));
}

/// Parse a declaration reference expression.
std::unique_ptr<Expr> Parser::ParseReferenceExpression() {
  Token identToken = tok; // Save the identifier token.
  nextToken();
NOARGS:

  // Resolve the identifier reference.
  Decl *refDecl = scope->getDecl(identToken.value);
  if (!refDecl) {
    fatal("unresolved symbol: " + identToken.value, identToken.loc);
  }

  // Resolve the declaration type, if it exists.
  const Type *refType = nullptr;
  if (VarDecl *varDecl = dynamic_cast<VarDecl *>(refDecl))
    refType = varDecl->getType();
  else if (ParamVarDecl *paramDecl = dynamic_cast<ParamVarDecl *>(refDecl))
    refType = paramDecl->getType();

  return std::make_unique<DeclRefExpr>(identToken.value, refDecl, 
      refType, createSpan(identToken.loc));
}

/// Parse a cast expression.
std::unique_ptr<Expr> Parser::ParseCastExpression() {
  /// UNRECOVERABLE: Cannot cast an expression that is already under a cast.
  if (isUnderCast) {
    fatal("cannot cast an expression already under a cast", tok.loc);
  }

  assert(tok.is(TokenKind::Identifier) && "expected identifier");

  const SourceLocation idLoc = tok.loc;

  // Resolve the cast type, if it exists yet.
  const Type *castType = ParseType();
  
  // Resolve the base expression.
  isUnderCast = 1;
  std::unique_ptr<Expr> baseExpr = ParseExpression();
  if (!baseExpr) {
    fatal("expected expression after cast: " + castType->toString(), 
        idLoc);
  }

  isUnderCast = 0;
  return std::make_unique<ExplicitCastExpr>(std::move(baseExpr), 
      castType->toString(), castType, createSpan(idLoc));
}

/// Parse a unary expression.
///
/// unary-op:
///   '-' <expr>
///   '!' <expr>
std::unique_ptr<Expr> Parser::ParseUnaryExpression() {
  UnaryExpr::UnaryOp op = getUnaryOp();
  if (op == UnaryExpr::UnaryOp::Unknown) {
    fatal("unresolved unary operator", tok.loc);
  }
  nextToken(); // Eat the operator token.

  std::unique_ptr<Expr> expr = ParsePrimaryExpression();
  if (!expr) {
    fatal("expected expression after unary operator", tok.loc);
  }

  return std::make_unique<UnaryExpr>(std::move(expr), op, 
      createSpan({ expr->getSpan().file, expr->getSpan().line,
      expr->getSpan().col }, lastLoc));
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

/// Parse a boolean literal expression.
///
/// Expects the current token to be a `true` or `false` identifier.
std::unique_ptr<Expr> Parser::ParseBooleanExpression() {
  assert(tok.is(TokenKind::Identifier) && "expected boolean literal");

  Token boolToken = tok; // Save the boolean token.
  nextToken();

  // Determine the type of the boolean literal.
  const Type *T = ctx->getType("bool");

  return std::make_unique<BooleanLiteral>(boolToken.value == "true", T,
    createSpan(boolToken.loc, boolToken.loc));
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

/// Parse a character literal expression.
///
/// Expects the current token to be a character literal.
std::unique_ptr<Expr> Parser::ParseCharacterExpression() {
  assert(tok.is(LiteralKind::Character) && "expected character literal");

  Token charToken = tok; // Save the character token.
  nextToken();

  // Determine the type of the character literal.
  const Type *T = ctx->getType("char");

  return std::make_unique<CharLiteral>(charToken.value[0], T,
    createSpan(charToken.loc));
}

/// Parse a string literal expression.
///
/// Expects the current token to be a string literal.
std::unique_ptr<Expr> Parser::ParseStringExpression() {
  assert(tok.is(LiteralKind::String) && "expected string literal");

  Token strToken = tok; // Save the string token.
  nextToken();

  // Determine the type of the string literal.
  const Type *T = ctx->getType("string");

  return std::make_unique<StringLiteral>(strToken.value, T,
    createSpan(strToken.loc));
}

/// Parse an array initialization expression.
///
/// Expects the current token to be an open bracket.
std::unique_ptr<Expr> Parser::ParseArrayInitExpression(const ArrayType *T) {
  assert(tok.is(TokenKind::OpenBracket) && "expected '['");

  SourceLocation firstLoc = tok.loc;
  nextToken(); // Eat the '[' token.

  size_t idxs = 0;
  vector<std::unique_ptr<Expr>> exprs = {};
  while (!tok.is(TokenKind::CloseBracket)) {
    std::unique_ptr<Expr> expr = ParseExpression();
    if (!expr) {
      fatal("expected expression in array initializer", tok.loc);
    }

    exprs.push_back(std::move(expr));
    idxs++;

    // Expect a terminator or another expression.
    if (tok.is(TokenKind::Comma)) {
      nextToken(); // Eat the ',' token.
    } else if (!tok.is(TokenKind::CloseBracket)) {
      fatal("expected ']' after array initializer", tok.loc);
    }
  }

  nextToken(); // Eat the ']' token.

  if (idxs < T->getSize()) {
    fatal("expected " + std::to_string(T->getSize()) + " expressions in array "
        "initializer, got " + std::to_string(idxs), lastLoc);
  }

  return std::make_unique<ArrayInitExpr>(std::move(exprs), T,
    createSpan(firstLoc, lastLoc));
}

/// Parse an array access expression.
///
/// Expects the current token to be an identifier.
std::unique_ptr<Expr> Parser::ParseArrayAccessExpression() {
  assert(tok.is(TokenKind::Identifier) && "expected identifier");

  // Attempt to resolve the base declaration.
  Decl *decl = scope->getDecl(tok.value);
  if (!decl) {
    fatal("unresolved reference: " + tok.value, tok.loc);
  }

  VarDecl *varDecl = dynamic_cast<VarDecl *>(decl);
  if (!varDecl) {
    fatal("expected variable reference: " + tok.value, tok.loc);
  }

  std::unique_ptr<Expr> base = std::make_unique<DeclRefExpr>(tok.value, 
      varDecl, varDecl->getType(), createSpan(tok.loc));

  const Token baseToken = tok; // Save the whole base token.
  nextToken();

  if (!tok.is(TokenKind::OpenBracket)) {
    fatal("expected '[' after array identifier", tok.loc);
  }
  nextToken(); // Eat the '[' token.

  std::unique_ptr<Expr> index = ParseExpression();
  if (!index) {
    fatal("expected expression after '['", tok.loc);
  }

  if (!tok.is(TokenKind::CloseBracket)) {
    fatal("expected ']' after array index", tok.loc);
  }
  nextToken(); // Eat the ']' token.

  return std::make_unique<ArrayAccessExpr>(baseToken.value, std::move(base),
      std::move(index), nullptr, createSpan(baseToken.loc, lastLoc));
}
