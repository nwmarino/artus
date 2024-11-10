#ifndef ARTUS_PARSE_PARSER_H
#define ARTUS_PARSE_PARSER_H

#include "../AST/Expr.h"
#include "../Core/Context.h"
#include "../Sema/Scope.h"

namespace artus {

/// A parser interface for parsing token streams.
class Parser final {
  friend class Context;

  /// Relevant context used during parsing.
  Context *ctx;

  /// The current token being parsed.
  Token tok;

  /// The token being currently peeked at, if it exists.
  Token peek;

  /// Flag used to indicate if the parser is currently peeking at a token.
  /// Signifies that the `tok` field will not be overriden by the next token.
  unsigned peeking : 1;

  /// Basic parsing flags.
  unsigned inLoop : 1;
  unsigned inFunction : 1;

  /// The last recorded location in the source code.
  SourceLocation lastLoc = { ctx->getActiveFileName(), 0, 0 };

  /// The current scope of the parser.
  Scope *scope;

  /// The type of the parent function declaration, if it exists.
  const FunctionType *parentFunctionType = nullptr;

  /// Consumes the current token and moves to the next token in the stream.
  /// Returns `true` if the parser has begun lexing a new unit, and `false`
  /// otherwise.
  bool nextToken() {
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

  /// Peeks at the next token in the stream. Does not consume the current token.
  /// This function is idempotent, in that it will not peek at the next token
  /// if it has already been peeked at. Returns `true` if there was a token to
  /// peek at, and `false` otherwise.
  bool peekToken() {
    return !peeking && ctx->lexer->Lex(peek) ? ++peeking : false;
  }

  /// Generate a span from the provided location to the last location seen by
  /// the parser.
  const Span createSpan(const SourceLocation &firstLoc) {
    return { .file = firstLoc.file, .line =  firstLoc.line, 
             .col =  firstLoc.col, .line_nd = lastLoc.line, 
             .col_nd =  lastLoc.col };
  }

  /// Generate a span between the two provided locations.
  const Span createSpan(const SourceLocation &firstLoc, 
                        const SourceLocation &lastLoc) {
    return { .file = firstLoc.file, .line =  firstLoc.line, 
             .col =  firstLoc.col, .line_nd = lastLoc.line, 
             .col_nd =  lastLoc.col };
  }

  /// Moves up the current scope to its parent, if it exists.
  inline void exitScope() { scope = scope->getParent(); }

  /// Pushes a new scope onto the scope stack.
  inline void enterScope(const ScopeContext &ctx) {
    scope = new Scope(scope, {}, ctx); 
  }

  std::unique_ptr<Expr> ParseExpression();
  std::unique_ptr<Expr> ParsePrimaryExpression();
  std::unique_ptr<Expr> ParseIntegerExpression();

  std::unique_ptr<Stmt> ParseStatement();
  std::unique_ptr<Stmt> ParseCompoundStatement();
  std::unique_ptr<Stmt> ParseLabelStatement();
  std::unique_ptr<Stmt> ParseRetStatement();

  std::unique_ptr<Decl> ParseDeclaration();
  std::unique_ptr<Decl> ParseFunctionDeclaration();
  std::vector<std::unique_ptr<ParamVarDecl>> ParseFunctionParams();

  std::unique_ptr<PackageUnitDecl> ParsePackageUnit();

public:
  Parser(Context *ctx) : ctx(ctx) {}

  /// Creates an AST from the token stream and embeds the package units into
  /// the context attached to this parser interface.
  void buildAST() { 
    while (ctx->nextFile()) 
      ctx->addPackage(ParsePackageUnit()); 
  }
};

} // namespace artus

#endif // ARTUS_PARSE_PARSER_H
