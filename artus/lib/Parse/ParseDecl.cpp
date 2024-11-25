#include <cassert>

#include "../../include/Core/Logger.h"
#include "../../include/Parse/Parser.h"
#include "../../include/Sema/Type.h"

using std::string;
using std::vector;

using namespace artus;

/// Parse a declaration.
std::unique_ptr<Decl> Parser::ParseDeclaration() {
  if (tok.isKeyword("priv")) {
    this->makePriv = true;
    nextToken(); // Consume the 'priv' keyword.
  }

  if (tok.isKeyword("fn"))
    return ParseFunctionDeclaration();
  else if (tok.isKeyword("struct"))
    return ParseStructDeclaration();
  else if (tok.isKeyword("enum"))
    return ParseEnumDeclaration();

  return nullptr;
}

/// Parse a function declaration.
///
/// fn-decl:
///   'fn' '@' <identifier> '(' [params] ')' '->' [return-type] <stmt>
///
/// params:
///   [type] <param> [',' [type] <param>]*
///
/// type:
///   <identfier>
///
/// return-type:
///   <identifier>
///
/// Expects the current token to be a 'fn' keyword.
std::unique_ptr<Decl> Parser::ParseFunctionDeclaration() {
  assert(tok.isKeyword("fn") && "expected 'fn' keyword");

  bool isPriv = this->makePriv;
  this->makePriv = false;

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

  // Enter into a new function scope.
  this->inFunction = 1;
  enterScope({ .isFunctionScope = 1 });

  // Parse the function parameters.
  vector<std::unique_ptr<ParamVarDecl>> params = ParseFunctionParams();

  // Unrecoverable error if the next token is not a '->' symbol.
  if (!tok.is(TokenKind::Arrow)) {
    fatal("expected '->' symbol after function parameters", lastLoc);
  }
  nextToken(); // Consume the '->' token.

  const Type *returnType = ParseType();

  // Create the function type.
  vector<const Type *> paramTypes;
  for (const auto &param : params)
    paramTypes.push_back(param->getType());

  const FunctionType *FT = new FunctionType(returnType, paramTypes);
  this->parentFunctionType = FT;

  // Parse the function body.
  std::unique_ptr<Stmt> body = ParseStatement();

  /// UNRECOVERABLE: No function body.
  if (!body) {
    fatal("expected statement after function declaration", lastLoc);
  }

  // Exit from the function scope.
  Scope *scope = this->scope;
  this->inFunction = 0;
  this->parentFunctionType = nullptr;
  exitScope();

  std::unique_ptr<FunctionDecl> fnDecl = std::make_unique<FunctionDecl>(
      functionName, FT, std::move(params), std::move(body), 
      scope, createSpan(fnToken.loc, lastLoc), isPriv);

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
  while (!tok.is(TokenKind::CloseParen)) {
    if (!tok.is(TokenKind::Identifier)) {
      trace("expected identifier after '(' symbol", lastLoc);
      return {};
    }

    bool isMutable = false;
    if (tok.isKeyword("mut")) {
      isMutable = true;
      nextToken(); // Consume the 'mut' keyword.

      if (!tok.is(TokenKind::Identifier)) {
        trace("expected identifier after 'mut' keyword", lastLoc);
        return {};
      }
    }

    const Token idToken = tok;
    nextToken(); // Consume the identifier token.

    if (!tok.is(TokenKind::Colon)) {
      trace("expected ':' symbol after parameter identifier", lastLoc);
      return {};
    }

    nextToken(); // Consume the ':' token.

    const Type *paramType = ParseType();

    // Create the parameter declaration and add it to the current scope.
    std::unique_ptr<ParamVarDecl> param = std::make_unique<ParamVarDecl>(
        idToken.value, paramType, isMutable, createSpan(idToken.loc));

    scope->addDecl(param.get());
    params.push_back(std::move(param));

    if (tok.is(TokenKind::Comma))
      nextToken(); // Consume the ',' token.
  }

  nextToken(); // Consume the ')' token.
  return params;
}

/// Parse a variable declaration.
///
/// var-decl:
///   'mut' <identifier> ':' <type> '=' <expr>
///   'fix' <identifier> ':' <type> '=' <expr>
std::unique_ptr<Decl> Parser::ParseVarDeclaration(bool isMut) {
  assert ((tok.isKeyword("mut") || tok.isKeyword("fix")) && \
      "expected 'mut' or 'fix' keyword");

  Token varToken = tok; // Save the 'mut' or 'fix' token.
  nextToken();

  if (!tok.is(TokenKind::Identifier)) {
    trace("expected identifier after 'mut' or 'fix' keyword", lastLoc);
    return nullptr;
  }

  const string varName = tok.value;
  nextToken(); // Consume the identifier token.

  if (this->makePriv) {
    trace("variable cannot be declared private: " + varName, lastLoc);
    return nullptr;
  }

  if (!tok.is(TokenKind::Colon)) {
    trace("expected ':' symbol after variable identifier", lastLoc);
    return nullptr;
  }
  nextToken(); // Consume the ':' token.

  const Type *varType = ParseType();
  std::unique_ptr<Expr> initExpr = nullptr;

  /// UNRECOVERABLE: Immutable variables must be initialized.
  if (tok.is(TokenKind::Equals)) {
    nextToken(); // Eat the '=' token.

    // Handle array type initialization.
    if (tok.is(TokenKind::OpenBracket)) {
      const ArrayType *AT = dynamic_cast<const ArrayType *>(varType);
      if (!AT) {
        trace("expected array type for array initialization", lastLoc);
        return nullptr;
      }
      initExpr = ParseArrayExpression(AT);
    } else
      initExpr = ParseExpression();

    if (!initExpr) {
      trace("expected expression after '=' symbol", lastLoc);
      return nullptr;
    }
  } else if (!isMut) {
    trace("expected '=' symbol after variable type", lastLoc);
    return nullptr;
  }

  // If a mutable variable is not initialized, create a default initializer.
  if (!initExpr)
    initExpr = ParseDefaultInitExpression(varType);

  // Instantiate the declaration and add it to the current scope.
  std::unique_ptr<VarDecl> decl = std::make_unique<VarDecl>(varName, varType, 
      std::move(initExpr), isMut, createSpan(varToken.loc));  
  scope->addDecl(decl.get());

  return decl;
}

/// Parse a list of struct field declarations.
///
/// Expects the current token to be a '{' symbol.
std::vector<std::unique_ptr<FieldDecl>> Parser::ParseFieldDeclarations() {
  assert(tok.is(TokenKind::OpenBrace) && "expected '{' symbol");
  nextToken(); // Consume the '{' token.

  // Parse the list of field declarations.
  vector<std::unique_ptr<FieldDecl>> fields;
  while (!tok.is(TokenKind::CloseBrace)) {
    bool isMutable = false;
    if (tok.isKeyword("mut")) {
      isMutable = true;
      nextToken(); // Consume the 'mut' keyword.
    }

    if (!tok.is(TokenKind::Identifier)) {
      fatal("expected field identifier", lastLoc);
    }

    const string fieldName = tok.value;
    nextToken(); // Consume the identifier token.

    if (!tok.is(TokenKind::Colon)) {
      fatal("expected ':' symbol after field identifier", lastLoc);
    }

    nextToken(); // Consume the ':' token.
    const Type *fieldType = ParseType();

    // Create the field declaration and add it to the current scope.
    std::unique_ptr<FieldDecl> field = std::make_unique<FieldDecl>(
        fieldName, fieldType, isMutable, createSpan(tok.loc));
    scope->addDecl(field.get());
    fields.push_back(std::move(field));

    if (tok.is(TokenKind::Comma)) {
      nextToken(); // Consume the ',' token.
      continue;
    }

    if (!tok.is(TokenKind::CloseBrace)) {
      fatal("expected ',' or '}' symbol after struct field declaration: " 
            + fieldName, lastLoc);
    }
  }

  nextToken(); // Consume the '}' token.
  return fields;
}

/// Parse a struct declaration.
///
/// struct-decl:
///   'struct' <identifier> '{' [fields] '}'
std::unique_ptr<Decl> Parser::ParseStructDeclaration() {
  assert(tok.isKeyword("struct") && "expected 'struct' keyword");

  Token structToken = tok; // Save the 'struct' token.
  nextToken(); // Consume the 'struct' token.

  if (!tok.is(TokenKind::Identifier)) {
    trace("expected identifier after 'struct' keyword", lastLoc);
    return nullptr;
  }

  const string structName = tok.value;
  nextToken(); // Consume the identifier token.

  bool isPrivate = false;
  if (this->makePriv) {
    isPrivate = true;
    this->makePriv = false;
  }

  if (!tok.is(TokenKind::OpenBrace)) {
    trace("expected '{' symbol after struct identifier", lastLoc);
    return nullptr;
  }

  // Enter into a new struct scope.
  enterScope({ .isStructScope = 1 });

  // Parse the struct fields.
  vector<std::unique_ptr<FieldDecl>> fields = ParseFieldDeclarations();

  // Exit from the struct scope.
  Scope *scope = this->scope;
  exitScope();

  // Get the type for each field in the struct.
  vector<const Type *> fieldTypes;
  for (std::unique_ptr<FieldDecl> const &field : fields) {
    fieldTypes.push_back(field->getType());
  }

  // Create a type for this struct definition.
  const StructType *ST = new StructType(structName, fieldTypes);
  this->ctx->addDefinedType(structName, ST);

  // Create the struct declaration.
  std::unique_ptr<StructDecl> structDecl = std::make_unique<StructDecl>(
      structName, std::move(fields), scope, ST, createSpan(structToken.loc),
      isPrivate);

  // Add the struct declaration to parent scope.
  this->scope->addDecl(structDecl.get());
  return structDecl;
}

/// Parse an enum declaration.
///
/// enum-decl:
///   'enum' <identifier> '{' [variants] '}'
///
/// variants:
///   <identifier> [',' <identifier>]*
///
/// Expects the current token to be an 'enum' keyword.
std::unique_ptr<Decl> Parser::ParseEnumDeclaration() {
  assert(tok.isKeyword("enum") && "expected 'enum' keyword");

  Token enumToken = tok; // Save the 'enum' token.
  nextToken(); // Consume the 'enum' token.

  if (!tok.is(TokenKind::Identifier)) {
    trace("expected identifier after 'enum' keyword", lastLoc);
    return nullptr;
  }

  const string enumName = tok.value;
  nextToken(); // Consume the identifier token.

  if (!tok.is(TokenKind::OpenBrace)) {
    trace("expected '{' symbol after enum identifier", lastLoc);
    return nullptr;
  }
  nextToken(); // Consume the '{' token.

  // Parse the list of enum variants.
  vector<string> variants;
  while (!tok.is(TokenKind::CloseBrace)) {
    if (!tok.is(TokenKind::Identifier)) {
      trace("expected identifier after '{' symbol", lastLoc);
      return nullptr;
    }

    // Check if the variant already exists.
    if (std::find(variants.begin(), variants.end(), tok.value) != variants.end()) {
      trace("duplicate enum variant: " + tok.value, lastLoc);
      return nullptr;
    }

    // Unique variant; add it to the list.
    variants.push_back(tok.value);
    nextToken(); // Consume the identifier token.

    if (tok.is(TokenKind::Comma)) {
      nextToken(); // Consume the ',' token.
      continue;
    }

    if (!tok.is(TokenKind::CloseBrace)) {
      fatal("expected ',' or '}' symbol after enum variant", lastLoc);
    }
  }

  nextToken(); // Consume the '}' token.

  // Create the enum type.
  const EnumType *ET = new EnumType(enumName, variants);
  this->ctx->addDefinedType(enumName, ET);

  bool isPrivate = false;
  if (this->makePriv) {
    isPrivate = true;
    this->makePriv = false;
  }

  // Create the enum declaration.
  std::unique_ptr<EnumDecl> enumDecl = std::make_unique<EnumDecl>(enumName,
      std::move(variants), ET, createSpan(enumToken.loc), isPrivate);

  // Add the enum declaration to parent scope.
  this->scope->addDecl(enumDecl.get());
  return enumDecl;
}

/// Parse a package unit.
std::unique_ptr<PackageUnitDecl> Parser::ParsePackageUnit() {
  const string id = ctx->getActiveFileName();

  // Declare a new scope for the package.
  enterScope({ .isUnitScope = 1 });
  nextToken(); // Begin lexing tokens of the package unit.

  /// TODO: Parse the imports of the package.
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
