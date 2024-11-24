#include <cassert>

#include "../../include/Core/Logger.h"
#include "../../include/Parse/Parser.h"

using namespace artus;

/// Parse a statement.
std::unique_ptr<Stmt> Parser::ParseStatement() {
  if (tok.is(TokenKind::OpenBrace))
    return ParseCompoundStatement();

  if (tok.isKeyword("ret"))
    return ParseRetStatement();
  else if (tok.isKeyword("fix") || tok.isKeyword("mut"))
    return ParseDeclStatement();
  else if (tok.isKeyword("if"))
    return ParseIfStatement();
  else if (tok.isKeyword("while"))
    return ParseWhileStatement();
  else if (tok.isKeyword("until"))
    return ParseUntilStatement();
  else if (tok.isKeyword("match"))
    return ParseMatchStatement();

  return ParseExpression();
}

/// Parse a compound statement.
///
/// compound:
///  '{' <statement>* '}'
///
/// Expects the current token to be an open brace.
std::unique_ptr<Stmt> Parser::ParseCompoundStatement() {
  const SourceLocation firstLoc = this->lastLoc;
  nextToken(); // Consume the '{' token.

  // Declare a new scope for this compound statement.
  enterScope({ .isCompoundScope = 1 });

  // Parse the compound statement body.
  vector<std::unique_ptr<Stmt>> stmts;
  while (!tok.is(TokenKind::CloseBrace)) {
    std::unique_ptr<Stmt> stmt = ParseStatement();
    if (!stmt) {
      trace("expected statement", lastLoc);
      return nullptr;
    }

    stmts.push_back(std::move(stmt));
  }

  const SourceLocation lastLoc = this->lastLoc;
  nextToken(); // Consume the '}' token.

  // Exit the scope of this compound statement.
  Scope *scope = this->scope;
  exitScope();

  return std::make_unique<CompoundStmt>(std::move(stmts), scope, 
                                        createSpan(firstLoc, lastLoc));
}

/// Parse a decl statement.
///
/// decl:
///   <identifier> ':' <type> ['=' <expression>]
///
/// Expects the current token to be a 'fix' or 'mut' keyword.
std::unique_ptr<Stmt> Parser::ParseDeclStatement() {
  assert((tok.isKeyword("fix") || tok.isKeyword("mut")) && \
      "expected 'fix' or 'mut' keyword");

  // Parse the nested variable declaration.
  std::unique_ptr<Decl> decl = nullptr;
  if (tok.isKeyword("fix"))
    decl = ParseVarDeclaration();
  else if (tok.isKeyword("mut"))
    decl = ParseVarDeclaration(1);

  if (!decl) {
    fatal("expected variable declaration after '" + tok.value + '\'', 
        lastLoc);
  }

  return std::make_unique<DeclStmt>(std::move(decl), decl->getSpan());
}

/// Parse an if statement.
///
/// if:
///   'if' <expression> <statement> ['else' <statement>]
///
/// Expects the current token to be an 'if' keyword.
std::unique_ptr<Stmt> Parser::ParseIfStatement() {
  assert(tok.isKeyword("if") && "expected 'if' keyword");

  const SourceLocation firstLoc = lastLoc;
  nextToken(); // Consume the 'if' token.

  std::unique_ptr<Expr> cond = ParseExpression();
  if (!cond) {
    trace("expected expression after 'if' statement", lastLoc);
    return nullptr;
  }

  std::unique_ptr<Stmt> thenStmt = ParseStatement();
  if (!thenStmt) {
    trace("expected statement after 'if' condition", lastLoc);
    return nullptr;
  }

  std::unique_ptr<Stmt> elseStmt = nullptr;
  if (tok.isKeyword("else")) {
    nextToken(); // Consume the 'else' token.

    elseStmt = ParseStatement();
    if (!elseStmt) {
      trace("expected statement after 'else' keyword", lastLoc);
      return nullptr;
    }
  }

  return std::make_unique<IfStmt>(std::move(cond), std::move(thenStmt), 
                                  std::move(elseStmt), createSpan(firstLoc));
}

/// Parse a while statement.
///
/// while:
///   'while' <expression> <statement>
///
/// Expects the current token to be a 'while' keyword.
std::unique_ptr<Stmt> Parser::ParseWhileStatement() {
  assert(tok.isKeyword("while") && "expected 'while' keyword");

  const SourceLocation firstLoc = lastLoc;
  nextToken(); // Consume the 'while' token.

  std::unique_ptr<Expr> cond = ParseExpression();
  if (!cond) {
    trace("expected expression after 'while' statement", lastLoc);
    return nullptr;
  }

  std::unique_ptr<Stmt> body = ParseStatement();
  if (!body) {
    trace("expected statement after 'while' condition", lastLoc);
    return nullptr;
  }

  return std::make_unique<WhileStmt>(std::move(cond), std::move(body), 
                                     createSpan(firstLoc));
}

/// Parse an until statement.
///
/// until:
///   'until' <expression> <statement>
///
/// Expects the current token to be an 'until' keyword.
std::unique_ptr<Stmt> Parser::ParseUntilStatement() {
  assert(tok.isKeyword("until") && "expected 'until' keyword");

  const SourceLocation firstLoc = lastLoc;
  nextToken(); // Consume the 'until' token.

  std::unique_ptr<Expr> cond = ParseExpression();
  if (!cond) {
    trace("expected expression after 'until' statement", lastLoc);
    return nullptr;
  }

  std::unique_ptr<Stmt> body = ParseStatement();
  if (!body) {
    trace("expected statement after 'until' condition", lastLoc);
    return nullptr;
  }

  return std::make_unique<UntilStmt>(std::move(cond), std::move(body), 
                                     createSpan(firstLoc));
}

/// Parse a match case statement.
///
/// match_case:
///   <expression> => <statement>
std::unique_ptr<MatchCase> Parser::ParseMatchCaseStatement() {
  // Handle default cases.
  bool isDefault = false;
  if (tok.is(TokenKind::Identifier) && tok.value == "_") {
    isDefault = true;
    nextToken(); // Consume the '_' token.
  }

  // Parse the expression to match.
  std::unique_ptr<Expr> expr = nullptr;
  if (!isDefault) {
    expr = ParseExpression();
    if (!expr) {
      fatal("expected expression in match case statement", lastLoc);
    }
  }

  if (!tok.is(TokenKind::FatArrow)) {
    trace("expected '=>' after match case expression", lastLoc);
    return nullptr;
  }
  nextToken(); // Consume the '=>' token.

  std::unique_ptr<Stmt> body = ParseStatement();
  if (!body) {
    trace("expected statement after match case expression", lastLoc);
    return nullptr;
  }

  if (isDefault) {
    return std::make_unique<DefaultStmt>(std::move(body), body->getSpan());
  }

  return std::make_unique<CaseStmt>(std::move(expr), std::move(body), 
                                    body->getSpan());
}

/// Parse a match statement.
///
/// match:
///   'match' <expression> '{' <match_case>* '}'
///
/// Expects the current token to be a 'match' keyword.
std::unique_ptr<Stmt> Parser::ParseMatchStatement() {
  assert(tok.isKeyword("match") && "expected 'match' keyword");

  const SourceLocation firstLoc = lastLoc;
  nextToken(); // Consume the 'match' token.

  std::unique_ptr<Expr> expr = ParseExpression();
  if (!expr) {
    trace("expected expression after 'match' statement", lastLoc);
    return nullptr;
  }

  if (!tok.is(TokenKind::OpenBrace)) {
    trace("expected '{' after 'match' expression", lastLoc);
    return nullptr;
  }
  nextToken(); // Consume the '{' token.

  vector<std::unique_ptr<MatchCase>> cases;
  while (!tok.is(TokenKind::CloseBrace)) {
    std::unique_ptr<MatchCase> stmt = ParseMatchCaseStatement();
    if (!stmt) {
      trace("expected match case statement", lastLoc);
      return nullptr;
    }

    cases.push_back(std::move(stmt));

    if (tok.is(TokenKind::Comma)) {
      nextToken(); // Consume the ',' token.
      continue;
    }

    if (tok.is(TokenKind::CloseBrace)) {
      break;
    }

    fatal("expected ',' or '}' after match case statement", lastLoc);
  }

  const SourceLocation lastLoc = this->lastLoc;
  nextToken(); // Consume the '}' token.

  return std::make_unique<MatchStmt>(std::move(expr), std::move(cases),
                                     createSpan(firstLoc, lastLoc));
}

/// Parse a ret statement.
///
/// ret:
///   'ret' <expression>
///
/// Expects the current token to be a 'ret' keyword.
std::unique_ptr<Stmt> Parser::ParseRetStatement() {
  assert(tok.isKeyword("ret") && "expected 'ret' keyword");

  const SourceLocation firstLoc = lastLoc;
  nextToken(); // Consume the 'ret' token.

  std::unique_ptr<Expr> expr = ParseExpression();
  
  // Unrecoverable error if there is no expression after 'ret'.
  if (!expr)
    fatal("expected value after 'ret' statement", lastLoc);

  return std::make_unique<RetStmt>(std::move(expr), 
                                   createSpan(firstLoc, lastLoc));
}
