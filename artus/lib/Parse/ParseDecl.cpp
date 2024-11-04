#include <cassert>

#include "../../include/Core/Logger.h"
#include "../../include/Parse/Parser.h"

using std::string;
using std::vector;

using namespace artus;

/// Parse a declaration.
std::unique_ptr<Decl> Parser::ParseDeclaration() {
  if (tok.isKeyword("fn"))
    return ParseFunctionDeclaration();

  return nullptr;
}

/// Parse a function declaration.
///
/// fn-decl:
///   'fn' '@' <identifier> '(' [params] ')' '->' [return-type] <stmt>
///
/// params:
///   <type> <param> [',' <type> <param>]*
///
/// return-type:
///   <type>
///
/// Expects the current token to be a 'fn' keyword.
std::unique_ptr<Decl> Parser::ParseFunctionDeclaration() {
  assert(tok.isKeyword("fn") && "expected 'fn' keyword");

  Token fnToken = tok; // Save the 'fn' token.
  nextToken(); // Consume the 'fn' token.

  if (!tok.is(TokenKind::At)) {
    trace("expected call '@' symbol after 'fn' keyword", lastLoc);
    return nullptr;
  }

  nextToken(); // Consume the '@' token.

  if (!tok.is(TokenKind::Identifier)) {
    trace("expected identifier after call '@' symbol", lastLoc);
    return nullptr;
  }

  const string functionName = tok.value;
  nextToken(); // Consume the identifier token.

  // Unrecoverable error if the next token is not a '(' symbol. (Unsupported)
  if (!tok.is(TokenKind::OpenParen)) {
    fatal("expected '(' symbol after function identifier", lastLoc);
  }

  enterScope({ .isFunctionScope = 1 });
  vector<std::unique_ptr<ParamVarDecl>> params = ParseFunctionParams();

  /* Redo until line x when void return type is supported. */

  // Unrecoverable error if the next token is not a '->' symbol. (No void types)
  if (!tok.is(TokenKind::Arrow)) {
    fatal("expected '->' symbol after function parameters", lastLoc);
  }

  nextToken(); // Consume the '->' token.

  // Unrecoverable error if the next token is not an identifier. (No void types)
  if (!tok.is(TokenKind::Identifier)) {
    fatal("expected type after '->' symbol", lastLoc);
  }

  const Type *T = ctx.getType(tok.value);
  nextToken(); // Consume the type token.

  // Parse the function body.
  std::unique_ptr<Stmt> body = ParseStatement();

  // Unrecoverable error if there is no statement after the function prototype.
  // (No empty function bodies)
  if (!body) {
    fatal("expected statement after function declaration", lastLoc);
  }

  // Create the function declaration.
  Scope *scope = this->scope;
  exitScope();

  std::unique_ptr<FunctionDecl> fnDecl = std::make_unique<FunctionDecl>(
      functionName, T, std::move(params), std::move(body), 
      scope, createSpan(fnToken.loc, lastLoc));

  // Add the function declaration to parent scope.
  this->scope->addDecl(fnDecl.get());
  return fnDecl;
}

/// Parse a list of function parameters.
///
/// Expects the current token to be a '(' symbol.
std::vector<std::unique_ptr<ParamVarDecl>> Parser::ParseFunctionParams() {
  assert(tok.is(TokenKind::OpenParen) && "expected '(' symbol");
  nextToken(); // Consume the '(' token.

  // Parse the list of parameters.
  vector<std::unique_ptr<ParamVarDecl>> params;
  while (!tok.is(TokenKind::CloseParen)) { /* no params yet */ }

  nextToken(); // Consume the ')' token.
  return params;
}

/// Parse a package unit.
std::unique_ptr<PackageUnitDecl> Parser::ParsePackageUnit() {
  const string id = ctx.getActiveFilePath();

  // Declare a new scope for the package.
  enterScope({ .isUnitScope = 1 });
  nextToken(); // Begin lexing tokens of the package unit.

  // Parse the imports of the package. (unsupported)
  vector<string> imports = {};

  // Parse the declarations of the package.
  vector<std::unique_ptr<Decl>> decls;
  while (!tok.is(TokenKind::Eof)) {
    std::unique_ptr<Decl> decl = ParseDeclaration();
    if (!decl)
      fatal("expected declaration", lastLoc);

    decls.push_back(std::move(decl));
  }

  // Exit the scope of the package.
  Scope *scope = this->scope;
  exitScope();

  return std::make_unique<PackageUnitDecl>(id, std::move(imports), 
      scope, std::move(decls));
}
