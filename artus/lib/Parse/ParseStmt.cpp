#include <cassert>

#include "../../include/Core/Logger.h"
#include "../../include/Parse/Parser.h"

using namespace artus;

/// Parse a statement.
std::unique_ptr<Stmt> Parser::ParseStatement() {
  if (tok.is(TokenKind::OpenBrace))
    return ParseCompoundStatement();

  peekToken();
  if (peek.is(TokenKind::Colon))
    return ParseLabelStatement();

  if (tok.isKeyword("ret"))
    return ParseRetStatement();
  else if (tok.isKeyword("jmp"))
    return ParseJmpStatement();
  else if (tok.isKeyword("fix") || tok.isKeyword("mut"))
    return ParseDeclStatement();

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

/// Parse a label statement.
///
/// label:
///   <identifier> ':'
///
/// Expects current token to be an identifier, followed by a peeked colon.
std::unique_ptr<Stmt> Parser::ParseLabelStatement() {
  assert(tok.is(TokenKind::Identifier) && \
      "expected identifier to define label");

  Token idToken = tok;
  nextToken(); // Consume the identifier token.

  assert(tok.is(TokenKind::Colon) && "expected colon after label identifier");

  const Span span = createSpan(idToken.loc);
  nextToken(); // Consume the colon token.

  // Check if a label with the same name already exists in the current scope.
  if (scope->getDecl(idToken.value)) {
    fatal("label '" + idToken.value + "' already defined in scope", 
        lastLoc);
  }
  
  // Store a new label declaration in scope.
  LabelDecl *labelDecl = new LabelDecl(idToken.value, span);
  scope->addDecl(labelDecl);

  // Initialize the statement, and store it in the associated declaration.
  std::unique_ptr<LabelStmt> labelStmt = std::make_unique<LabelStmt>(
      idToken.value, labelDecl, span);
  labelDecl->setStmt(labelStmt.get());

  return labelStmt;
}

/// Parse a jmp statement.
///
/// jmp:
///   'jmp' <identifier>
///
/// Expects the current token to be a 'jmp' keyword.
std::unique_ptr<Stmt> Parser::ParseJmpStatement() {
  assert(tok.isKeyword("jmp") && "expected 'jmp' keyword");

  const SourceLocation firstLoc = lastLoc;
  nextToken(); // Consume the 'jmp' token.

  if (!tok.is(TokenKind::Identifier)) {
    trace("expected identifier after 'jmp' statement", lastLoc);
    return nullptr;
  }

  Token idToken = tok; // Store the identifier token.
  nextToken();

  return std::make_unique<JmpStmt>(idToken.value, nullptr, 
      createSpan(firstLoc));
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
