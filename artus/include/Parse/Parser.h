//>==- Parser.h -----------------------------------------------------------==<//
//
// This header files declares the interface used to parse token streams into an
// abstract syntax tree.
//
//>==----------------------------------------------------------------------==<//

#ifndef ARTUS_PARSE_PARSER_H
#define ARTUS_PARSE_PARSER_H

#include <memory>

#include "../AST/DeclBase.h"
#include "../AST/Decl.h"
#include "../AST/Expr.h"
#include "../AST/Stmt.h"
#include "../Core/Context.h"
#include "../Sema/Scope.h"
#include "../Sema/Type.h"

namespace artus {

/// A parser interface for parsing token streams.
class Parser final {
  friend class Context;

  /// Non-owning context used during parsing.
  Context *ctx;

  /// The current token being parsed.
  Token tok;

  /// The token being currently peeked at, if it exists.
  Token peek;

  /// Flag used to indicate if the parser is currently peeking at a token.
  /// Signifies that the `tok` field will not be overriden by the next token.
  unsigned peeking : 1 = 0;

  /// Basic parsing flags.
  unsigned inLoop : 1 = 0;
  unsigned inFunction : 1 = 0;

  /// Flag used to indicate if the current declaration should be made private.
  unsigned makePriv : 1 = 0;

  /// Flag used to indicate if the current expession is being casted.
  unsigned isUnderCast : 1 = 0;

  /// The last recorded location in the source code.
  SourceLocation lastLoc;

  /// The current scope of the parser.
  Scope *scope;

  /// The type of the parent function declaration, if it exists.
  const FunctionType *parentFunctionType;

  /// The type of the parent variable declaration, if it exists.
  const Type *parentVarType;

  /// Consumes the current token and moves to the next token in the stream.
  /// \returns `true` if the parser has begun lexing a new unit, and `false`
  /// otherwise.
  bool nextToken();

  /// Peeks at the next token in the stream. Does not consume the current token.
  /// This function is idempotent, in that it will not peek at the next token
  /// if it has already been peeked at. \returns `true` if there was a token to
  /// peek at, and `false` otherwise.
  bool peekToken();

  /// \returns A span object from the \p firstLoc to the current location.
  const Span createSpan(const SourceLocation &firstLoc) const;

  /// \returns A span object between \p firstLoc and \p lastLoc.
  const Span createSpan(const SourceLocation &firstLoc, 
                        const SourceLocation &lastLoc) const;

  /// Moves up the current scope to its parent, if it exists.
  void exitScope();

  /// Pushes a new scope onto the scope stack with context \p ctx.
  void enterScope(const ScopeContext &ctx);

  /// \returns The precedence for the current token.
  int getPrecedence() const;

  /// Returns the unary operator equivelant of the current token.
  UnaryExpr::UnaryOp getUnaryOp() const;

  /// Returns the binary operator equivelant of the current token.
  BinaryExpr::BinaryOp getBinaryOp() const;

  std::unique_ptr<Expr> ParseExpression();
  std::unique_ptr<Expr> ParsePrimaryExpression();
  std::unique_ptr<Expr> ParseIdentifierExpression();
  std::unique_ptr<Expr> ParseCallExpression();
  std::unique_ptr<Expr> ParseReferenceExpression();
  std::unique_ptr<Expr> ParseCastExpression();
  std::unique_ptr<Expr> ParseUnaryExpression();
  std::unique_ptr<Expr> ParseBinaryExpression(std::unique_ptr<Expr> base, 
                                              int precedence = 0);
  std::unique_ptr<Expr> ParseBooleanExpression();
  std::unique_ptr<Expr> ParseIntegerExpression();
  std::unique_ptr<Expr> ParseFPExpression();
  std::unique_ptr<Expr> ParseCharacterExpression();
  std::unique_ptr<Expr> ParseStringExpression();
  std::unique_ptr<Expr> ParseNullExpression();
  std::unique_ptr<Expr> ParseArrayExpression(const ArrayType *T);
  std::unique_ptr<Expr> ParseArrayAccessExpression();
  std::unique_ptr<Expr> ParseStructInitExpression();
  std::unique_ptr<Expr> ParseMemberExpression(std::unique_ptr<Expr> base = nullptr);
  std::unique_ptr<Expr> ParseEnumReferenceExpression();

  std::unique_ptr<Stmt> ParseStatement();
  std::unique_ptr<Stmt> ParseBreakStatement();
  std::unique_ptr<Stmt> ParseContinueStatement();
  std::unique_ptr<Stmt> ParseCompoundStatement();
  std::unique_ptr<Stmt> ParseDeclStatement();
  std::unique_ptr<Stmt> ParseIfStatement();
  std::unique_ptr<Stmt> ParseWhileStatement();
  std::unique_ptr<Stmt> ParseUntilStatement();
  std::unique_ptr<Stmt> ParseMatchStatement();
  std::unique_ptr<Stmt> ParseRetStatement();

  std::unique_ptr<Decl> ParseDeclaration();
  std::unique_ptr<Decl> ParseImportDeclaration();
  std::unique_ptr<Decl> ParseFunctionDeclaration();
  std::unique_ptr<Decl> ParseVarDeclaration(bool isMut = 0);
  std::unique_ptr<Decl> ParseStructDeclaration();
  std::unique_ptr<Decl> ParseEnumDeclaration();

  std::unique_ptr<MatchCase> ParseMatchCaseStatement();
  std::vector<std::unique_ptr<FieldDecl>> ParseFieldDeclarations();
  std::vector<std::unique_ptr<ParamVarDecl>> ParseFunctionParams();
  std::unique_ptr<PackageUnitDecl> ParsePackageUnit();

  std::unique_ptr<Expr> ParseDefaultInitExpression(const Type *T);
  const Type *ParseType();

public:
  Parser(Context *ctx) : ctx(ctx), scope(nullptr), parentFunctionType(nullptr),
                         lastLoc({ ctx->getActiveFileName(), 0, 0 }) {}
  ~Parser() = default;

  /// Creates an AST from the token stream and embeds the package units into
  /// the context attached to this parser interface.
  void buildAST() { 
    while (ctx->nextFile())
      ctx->addPackage(ParsePackageUnit()); 
  }
};

} // end namespace artus

#endif // ARTUS_PARSE_PARSER_H
