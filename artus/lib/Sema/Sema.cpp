#include <cassert>

#include "../../include/Core/Logger.h"
#include "../../include/Sema/Sema.h"

using namespace artus;

Sema::Sema(Context *ctx) : ctx(ctx) {
  for (PackageUnitDecl *pkg : ctx->cache->getUnits()) {
    pkg->pass(this); // Sema on each package unit.
  }
}
/// Semantic Analysis over a PackageUnitDecl.
///
/// PackageUnitDecls are valid if and only if they have valid declarations.
void Sema::visit(PackageUnitDecl *decl) {
  globalScope = decl->scope;

  for (const std::unique_ptr<Decl> &d : decl->decls) {
    d->pass(this); // Sema on each declaration.
  }

  if (!hasMain)
    fatal("'main' function not found: " + decl->identifier);

  globalScope = nullptr;
}

/// Semantic Analysis over a FunctionDecl.
///
/// FunctionDecls are valid if and only if they have valid parameters, a valid
/// type designation, and a semantically valid body.
void Sema::visit(FunctionDecl *decl) {
  inFunction = 1;
  localScope = decl->scope;

  // Sema on each parameter.
  for (const std::unique_ptr<ParamVarDecl> &param : decl->params) {
    param->pass(this);
    paramIndex++;
  }
  paramIndex = 0;
  
  // Resolve the function type for later type checking.
  parentFunctionType = dynamic_cast<const FunctionType *>(decl->T);
  if (!parentFunctionType) {
    fatal("expected function type: " + decl->name, 
    { decl->span.file, decl->span.line, decl->span.col });
  }

  // Check if the function is the main function.
  if (decl->name == "main") {
    hasMain = 1;

    // Check if main function returns 'i64'.
    if (parentFunctionType->getReturnType()->compare(ctx->getType("i64")) != 1) {
      fatal("main function must return 'i64'", { decl->span.file, 
          decl->span.line, decl->span.col });
    }
  }
  
  decl->body->pass(this); // Sema on the body of the function.

  inFunction = 0;
  localScope = localScope->getParent();
  parentFunctionType = nullptr;
}

/// Semantic Analysis over a ParamVarDecl.
///
/// ParamVarDecls are valid if and only if they match their designated type in
/// the parent function's parameter list.
void Sema::visit(ParamVarDecl *decl) {
  const Type *targetType = parentFunctionType->getParamType(paramIndex);
  // Check that the type of the parameter is valid.
  if (decl->T->compare(targetType) == 0) {
    fatal("parameter type mismatch: " + decl->name, 
    { decl->span.file, decl->span.line, decl->span.col });
  }
}

/// Semantic Analysis over a LabelDecl.
void Sema::visit(LabelDecl *decl) { /* unused */ }

/// Semantic Analysis over an ImplicitCastExpr.
///
/// ImplicitCastExprs are valid if and only if they are of the same type as the
/// expression they are casting.
void Sema::visit(ImplicitCastExpr *expr) {
  expr->expr->pass(this); // Sema on the expression.

  if (expr->T->compare(expr->expr->T) == 0) {
    fatal("implicit cast type mismatch", { expr->span.file, 
        expr->span.line, expr->span.col });
  }

  // Propagate the type of the expression.
  expr->expr->T = expr->T;
}

/// Semantic Analysis over an ExplicitCastExpr.
///
/// ExplicitCastExprs are valid if and only if they are of the same type as the
/// expression they are casting.
void Sema::visit(ExplicitCastExpr *expr) {
  expr->expr->pass(this); // Sema on the expression.

  if (expr->T->compare(expr->expr->T) == 0) {
    fatal("explicit cast type mismatch", { expr->span.file, 
        expr->span.line, expr->span.col });
  }

  // Propagate the type of the expression.
  expr->expr->T = expr->T;
}

/// Semantic Analysis over an IntegerLiteral.
///
/// IntegerLiterals are valid if and only if they are of an integer type.
void Sema::visit(IntegerLiteral *expr) {
  if (!expr->T->isIntegerType()) {
    fatal("expected integer type", { expr->span.file, 
        expr->span.line, expr->span.col });
  }
}

/// Semantic Analysis over a CompoundStmt.
///
/// CompoundStmts are valid if and only if all of their statements are valid.
void Sema::visit(CompoundStmt *stmt) {
  localScope = stmt->scope;
  for (const std::unique_ptr<Stmt> &s : stmt->stmts) {
    s->pass(this); // Sema on each statement.
  }

  localScope = localScope->getParent();
}

/// Semantic Analysis over a LabelStmt.
///
/// LabelStmts are valid if and only if the label is declared and is named.
void Sema::visit(LabelStmt *stmt) {
  // Check that the label has a name.
  if (stmt->name.empty()) {
    fatal("unnamed label", { stmt->span.file, 
        stmt->span.line, stmt->span.col });
  }

  // Check that the label is not already declared.
  const LabelDecl *decl = dynamic_cast<const LabelDecl *>(
      localScope->getDecl(stmt->name));

  if (!decl) {
    fatal("label not declared: " + stmt->name, { stmt->span.file,
        stmt->span.line, stmt->span.col });
  }
}

/// Semantic Analysis over a RetStmt.
///
/// RetStmts are valid if and only if the statement matches the function's
/// return type.
void Sema::visit(RetStmt *stmt) {
  /// UNRECOVERABLE: Return statement outside of function.
  if (!inFunction) {
    fatal("return statement outside of function");
  }

  stmt->expr->pass(this); // Sema on the expression.

  // Check that the return type matches the function's return type.
  if (stmt->T->compare(parentFunctionType->getReturnType()) == 0) {
    fatal("return type mismatch", { stmt->span.file, 
        stmt->span.line, stmt->span.col });
  }
}
