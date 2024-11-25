#include <cassert>
#include <memory>
#include <string>

#include "../../include/AST/Expr.h"
#include "../../include/Core/Logger.h"
#include "../../include/Sema/Type.h"
#include "../../include/Sema/Sema.h"

using namespace artus;

Sema::Sema(Context *ctx) : ctx(ctx), parentFunctionType(nullptr), 
    lvalueType(nullptr) {
  for (PackageUnitDecl *pkg : ctx->cache->getUnits()) {
    pkg->pass(this); // Sema on each package unit.
  }
}

const VarDecl *Sema::resolveReference(Expr *lvalue) {
  if (DeclRefExpr *ref = dynamic_cast<DeclRefExpr *>(lvalue)) {
    Decl *decl = localScope->getDecl(ref->ident);
    if (!decl) {
      fatal("unresolved reference: " + ref->ident,
          { ref->span.file, ref->span.line, ref->span.col });
    }

    lastReference = ref->ident;
    return dynamic_cast<const VarDecl *>(decl);
  }

  if (ArraySubscriptExpr *arr = dynamic_cast<ArraySubscriptExpr *>(lvalue)) {
    return resolveReference(arr->base.get());
  }

  if (UnaryExpr *unary = dynamic_cast<UnaryExpr *>(lvalue)) {
    return resolveReference(unary->base.get());
  }

  if (MemberExpr *member = dynamic_cast<MemberExpr *>(lvalue)) {
    return resolveReference(member->base.get());
  }

  return nullptr;
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

  // Resolve the function type for later type checking.
  parentFunctionType = dynamic_cast<const FunctionType *>(decl->T);
  if (!parentFunctionType) {
    fatal("expected function type: " + decl->name, 
    { decl->span.file, decl->span.line, decl->span.col });
  }

  // Sema on each parameter.
  for (const std::unique_ptr<ParamVarDecl> &param : decl->params) {
    param->pass(this);
    paramIndex++;
  }
  paramIndex = 0;

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

/// Semantic Analysis over a VarDecl.
///
/// VarDecls are valid if and only if they are of the same type as their
/// initializer.
void Sema::visit(VarDecl *decl) {
  lvalueType = decl->T; // Assign the variable type for the initializer pass.
  decl->init->pass(this);

  // Check that the assigned variable type and the initializer type match.
  if (decl->T->compare(decl->init->T) == 0) {
    fatal("variable type mismatch: " + decl->name + ": expected " + 
        decl->T->toString() + ", got " + decl->init->T->toString(),
    { decl->span.file, decl->span.line, decl->span.col });
  }

  lvalueType = nullptr;
}

void Sema::visit(EnumDecl *decl) { /* no work to be done */ }
void Sema::visit(FieldDecl *decl) { /* no work to be done */ }

/// Semantic Analysis over a StructDecl.
///
/// StructDecls are valid if and only if they have valid, unique fields.
void Sema::visit(StructDecl *decl) {
  localScope = decl->scope;

  // Check that there are no duplicate fields.
  for (size_t i = 0; i < decl->fields.size(); i++) {
    for (size_t j = i + 1; j < decl->fields.size(); j++) {
      if (decl->fields[i]->name == decl->fields[j]->name) {
        fatal("duplicate field: " + decl->fields[i]->name, 
        { decl->span.file, decl->span.line, decl->span.col });
      }
    }
  }

  for (const std::unique_ptr<FieldDecl> &field : decl->fields) {
    field->pass(this); // Sema on each field.
  }

  localScope = localScope->getParent();
}

/// Semantic Analysis over an ImplicitCastExpr.
///
/// ImplicitCastExprs are valid if and only if they are of the same type as the
/// expression they are casting.
void Sema::visit(ImplicitCastExpr *expr) {
  expr->expr->pass(this); // Sema on the expression.

  // Assert that the cast is valid, as it was injected by the compiler.
  assert(expr->expr->T->canCastTo(expr->T) && 
      "implicit cast type mismatch");
}

/// Semantic Analysis over an ExplicitCastExpr.
///
/// ExplicitCastExprs are valid if and only if they are of the same type as the
/// expression they are casting.
void Sema::visit(ExplicitCastExpr *expr) {
  expr->expr->pass(this); // Sema on the expression.

  // Type check the cast.
  if (!expr->expr->T->canCastTo(expr->T)) {
    fatal("explicit cast type mismatch: " + expr->expr->T->toString() + 
          " to " + expr->ident, { expr->span.file, 
        expr->span.line, expr->span.col });
  }
}

void Sema::visit(DeclRefExpr *expr) { /* no work to be done */ }

/// Semantic Analysis over a CallExpr.
///
/// CallExprs are valid if and only if the callee is a function in scope and the
/// arguments match both in count and type to the function.
void Sema::visit(CallExpr *expr) {
  // By reference analysis, this expression refers to a function already.
  const FunctionDecl *callee = dynamic_cast<const FunctionDecl *>(
      localScope->getDecl(expr->ident));
  assert(callee && "expected function declaration");

  // Check that the number of arguments matches the number of parameters.
  if (expr->getNumArgs() != callee->getNumParams()) {
    fatal(expr->ident + " argument count mismatch: " + 
        std::to_string(callee->getNumParams()) + " required, but " + 
        std::to_string(expr->getNumArgs()) + " were provided", 
        { expr->span.file, expr->span.line, expr->span.col });
  }

  // Check that the types of the arguments match the types of the parameters.
  for (size_t i = 0; i < expr->getNumArgs(); i++) {
    expr->getArg(i)->pass(this); // Sema on each argument.

    // Type check the function call argument with the parameter.
    if (expr->getArg(i)->T->compare(callee->params[i]->T) == 0) {
      fatal("argument type mismatch: " + expr->ident + ": expected " 
          + callee->params[i]->T->toString() + ", got " 
          + expr->getArg(i)->T->toString(), { expr->span.file, 
          expr->span.line, expr->span.col });
    }

    // Check that an immutable reference is not passed to a mutable parameter.
    if (DeclRefExpr *ref = dynamic_cast<DeclRefExpr *>(expr->getArg(i))) {
      const VarDecl *decl = resolveReference(ref);
      if (!decl) {
        fatal("unresolved reference: " + ref->ident, { expr->span.file,
            expr->span.line, expr->span.col });
      }

      if (callee->params[i]->isMutable() && !decl->isMutable()) {
        fatal("attempted to pass immutable reference to mutable parameter: "
            + lastReference, { expr->span.file, expr->span.line,
            expr->span.col });
      }
    }
  }

  // Propagate the type of the expression.
  const FunctionType *FT = dynamic_cast<const FunctionType *>(callee->T);
  assert(FT && "expected function type");
  expr->T = FT->getReturnType();
}

/// Semantic Analysis over a UnaryExpr.
///
/// UnaryExprs are valid if and only if they are of the same type as their
/// operand.
void Sema::visit(UnaryExpr *expr) {
  expr->base->pass(this); // Sema on the expression.

  if (expr->T->compare(expr->base->T) == 0) {
    fatal("unary expression type mismatch", { expr->span.file, 
        expr->span.line, expr->span.col });
  }

  // Propagate the type of the expression.
  expr->T = expr->base->T;

  // Check that reference operators '&' are only done to lvalues.
  if (expr->op == UnaryExpr::UnaryOp::Ref) {
    // Check that the operand is an lvalue.
    if (!dynamic_cast<const DeclRefExpr *>(expr->base.get())) {
      fatal("expected lvalue for unary operator: &", { expr->span.file, 
          expr->span.line, expr->span.col });
    }

    // Nest the base type in a pointer type.
    expr->T = ctx->getType('#' + expr->T->toString());
  }

  // Check that dereference operators '#' are only done to pointers.
  if (expr->op == UnaryExpr::UnaryOp::DeRef) {
    // Check that the operand is a pointer type.
    if (!expr->base->T->isPointerType()) {
      fatal("expected pointer type for unary operator: #", { expr->span.file, 
          expr->span.line, expr->span.col });
    }

    // Dereference the base type.
    const PointerType *ptrType = dynamic_cast<const PointerType *>(expr->base->T);
    assert (ptrType && "expected pointer type");
    expr->T = ptrType->getPointeeType();
  }
}

/// Semantic Analysis over a BinaryExpr.
///
/// BinaryExprs are valid if and only if they are of the same type as their
/// operands.
void Sema::visit(BinaryExpr *expr) {
  expr->lhs->pass(this); // Sema on the left-hand side.
  expr->rhs->pass(this); // Sema on the right-hand side.

  // Check that the types of the operands match, and if not, attempt to cast.
  int typeComp = expr->lhs->T->compare(expr->rhs->T);
  if (typeComp == 0) {
    fatal("binary expression type mismatch: " + expr->lhs->T->toString()
        + " and " + expr->rhs->T->toString(), { expr->span.file, 
        expr->span.line, expr->span.col });
  } else if (typeComp == 2) {
    if (expr->rhs->T->canCastTo(expr->lhs->T)) {
      // Inject implicit cast on rhs to lhs type.
      expr->rhs = std::make_unique<ImplicitCastExpr>(std::move(expr->rhs),
          expr->lhs->T->toString(), expr->lhs->T, expr->rhs->span);
    } else {
      fatal("unable to type cast " + expr->rhs->T->toString() + " to "
          + expr->lhs->T->toString(), { expr->span.file, 
          expr->span.line, expr->span.col });
    }
  }

  // If the operands are strings, check that the operator is a concatenation.
  if (expr->lhs->T->isStringType() && expr->rhs->T->isStringType()) {
    if (expr->op != BinaryExpr::BinaryOp::Add) {
      fatal("expected string concatenation operator: +", { expr->span.file,
          expr->span.line, expr->span.col });
    }
  }

  // Propagate the type of the expression as a boolean if it is a comparison.
  if (expr->isComparison()) {
    expr->T = ctx->getType("bool");
    return;
  }

  // Propagate the type of the expression otherwise.
  expr->T = expr->lhs->T;

  // Check that assignment is only done to mutable lvalues.
  if (!expr->isAssignment()) {
    return;
  }

  if (!expr->lhs->isLValue()) {
    fatal("expected lvalue to variable assignment", { expr->span.file,
        expr->span.line, expr->span.col });
  }

  // Check that immutable references are not reassigned.
  const VarDecl *decl = resolveReference(expr->lhs.get());
  if (!decl) {
    fatal("unresolved reference: " + lastReference, { expr->span.file,
        expr->span.line, expr->span.col });
  }

  if (!decl->isMutable()) {
    fatal("attempted to reassign immutable variable: " + lastReference,
        { expr->span.file, expr->span.line, expr->span.col });
  }

  // Check that `str` types are not mutated.
  if (ArraySubscriptExpr *arr = dynamic_cast<ArraySubscriptExpr *>(expr->lhs.get())) {
    if (arr->base->T->isStringType()) {
      fatal("attempted to mutate string type", { expr->span.file,
          expr->span.line, expr->span.col });
    }
  }

  // Check that immutable fields are not mutated.
  if (MemberExpr *member = dynamic_cast<MemberExpr *>(expr->lhs.get())) {
    // By reference analysis, the member expression base is of a struct type.
    StructDecl *SD = dynamic_cast<StructDecl *>(globalScope->getDecl(
        member->base->T->toString()));
    assert(SD && "invalid struct declaration");

    if (!SD->isFieldMutable(member->getMember())) {
      fatal("attempted to reassign immutable field: " + member->getMember(),
          { expr->span.file, expr->span.line, expr->span.col });
    }
  }
}

void Sema::visit(BooleanLiteral *expr) { /* no work to be done */ }
void Sema::visit(IntegerLiteral *expr) { /* no work to be done */ }
void Sema::visit(FPLiteral *expr) { /* no work to be done */ }
void Sema::visit(CharLiteral *expr) { /* no work to be done */ }
void Sema::visit(StringLiteral *expr) { /* no work to be done */ }
void Sema::visit(NullExpr *expr) { /* no work to be done */ }

/// Semantic Analysis over an ArrayExpr.
///
/// ArrayInitExprs are valid if and only if all of their expressions are of the
/// same type as the array.
void Sema::visit(ArrayExpr *expr) {
  const ArrayType *AT = dynamic_cast<const ArrayType *>(expr->T);
  assert(AT && "expected array type");

  for (const std::unique_ptr<Expr> &e : expr->exprs) {
    e->pass(this); // Sema on the expression.

    // Check that each element type matches the array type.
    if (e->T->compare(AT->getElementType()) == 0) {
      fatal("array expression type mismatch: " + e->T->toString() + " for " 
          + AT->getElementType()->toString(), { expr->span.file, 
          expr->span.line, expr->span.col });
    }

    // Propagate the type of the expression.
    e->T = AT->getElementType();
  }
}

/// Semantic Analysis over a ArraySubscriptExpr.
///
/// ArrayAccessExprs are valid if and only if they are of the same type as their
/// array. The index is also checked to be of an integer type.
void Sema::visit(ArraySubscriptExpr *expr) {
  expr->base->pass(this); // Sema on the base expression.
  expr->index->pass(this); // Sema on the index expression.

  // Enforce only certain types of array-like accessing.
  if (!expr->base->T->canSubscript()) {
    fatal("expected array type for array access", { expr->span.file,
        expr->span.line, expr->span.col });
  }

  // Enforce only integer literals for indexing.
  if (!expr->index->T->isIntegerType()) {
    fatal("expected integer index type", { expr->span.file, 
        expr->span.line, expr->span.col });
  }
}

/// Semantic Analysis over a StructInitExpr.
///
/// StructInitExprs are valid if and only if all of their fields are valid.
void Sema::visit(StructInitExpr *expr) {
  // By reference analysis, this expression refers to a struct already.
  const StructDecl *SD = dynamic_cast<const StructDecl *>(
      localScope->getDecl(expr->name));
  assert(SD && "expected struct declaration");

  // Type check each field.
  for (const auto &field : expr->fields) {
    field.second->pass(this); // Sema on the field expression.

    // Check that the field exists in the struct.
    const Type *FT = SD->getFieldType(field.first);
    if (!FT) {
      fatal("undeclared field: " + field.first, { expr->span.file,
          expr->span.line, expr->span.col });
    }

    // Type check the field.
    if (field.second->T->compare(FT) == 0) {
      fatal("field type mismatch: " + field.first + ": expected " + 
          FT->toString() + ", got " + field.second->T->toString(), 
          { expr->span.file, expr->span.line, expr->span.col });
    }
  }
}

/// Semantic Analysis over a MemberExpr.
///
/// MemberExprs are valid if and only if the base is valid.
void Sema::visit(MemberExpr *expr) {
  expr->base->pass(this);
}

/// Semantic Analysis over a BreakStmt.
///
/// BreakStmts are valid if and only if they are within a loop.
void Sema::visit(BreakStmt *stmt) {
  if (!this->inLoop) {
    fatal("break statement outside of loop", 
        { stmt->span.file, stmt->span.line, stmt->span.col });
  }
}

/// Semantic Analysis over a ContinueStmt.
///
/// ContinueStmts are valid if and only if they are within a loop.
void Sema::visit(ContinueStmt *stmt) {
  if (!this->inLoop) {
    fatal("continue statement outside of loop", 
        { stmt->span.file, stmt->span.line, stmt->span.col });
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

/// Semantic Analysis over a DeclStmt.
///
/// DeclStmts are valid if and only if the declaration is valid.
void Sema::visit(DeclStmt *stmt) {
  stmt->decl->pass(this); // Sema on the declaration.
}

/// Semantic Analysis over an IfStmt.
///
/// IfStmts are valid if and only if the condition is of a boolean type.
void Sema::visit(IfStmt *stmt) {
  stmt->cond->pass(this); // Sema on the condition.

  if (!stmt->cond->T->isBooleanType()) {
    fatal("expected boolean type", { stmt->span.file, 
        stmt->span.line, stmt->span.col });
  }

  stmt->thenStmt->pass(this); // Sema on the then statement.
  if (stmt->hasElse()) {
    stmt->elseStmt->pass(this); // Sema on the else statement.
  }
}

/// Semantic Analysis over a WhileStmt.
///
/// WhileStmts are valid if and only if the condition is of a boolean type.
void Sema::visit(WhileStmt *stmt) {
  stmt->cond->pass(this); // Sema on the condition.

  if (!stmt->cond->T->isBooleanType()) {
    fatal("expected boolean type", { stmt->span.file, 
        stmt->span.line, stmt->span.col });
  }

  // Setup new loop flags.
  unsigned prevInLoop = this->inLoop;
  LoopKind prevLoopKind = this->loopKind;
  this->inLoop = 1;
  this->loopKind = LoopKind::WHILE;

  stmt->body->pass(this); // Sema on the body of the loop.
  this->inLoop = prevInLoop;
  this->loopKind = prevLoopKind;
}

/// Semantic Analysis over an UntilStmt.
///
/// UntilStmts are valid if and only if the condition is of a boolean type.
void Sema::visit(UntilStmt *stmt) {
  stmt->cond->pass(this); // Sema on the condition.

  if (!stmt->cond->T->isBooleanType()) {
    fatal("expected boolean type", { stmt->span.file, 
        stmt->span.line, stmt->span.col });
  }

  // Setup new loop flags.
  unsigned prevInLoop = this->inLoop;
  LoopKind prevLoopKind = this->loopKind;
  this->inLoop = 1;
  this->loopKind = LoopKind::UNTIL;

  stmt->body->pass(this); // Sema on the body of the loop.
  this->inLoop = prevInLoop;
  this->loopKind = prevLoopKind;
}

/// Semantic Analysis over a CaseStmt.
///
/// CaseStmts are valid if and only if the condition and body are valid.
void Sema::visit(CaseStmt *stmt) {
  stmt->expr->pass(this); // Sema on the condition.
  stmt->body->pass(this); // Sema on the body of the case.
}

/// Semantic Analysis over a DefaultStmt.
///
/// DefaultStmts are valid if and only if the body is valid.
void Sema::visit(DefaultStmt *stmt) {
  stmt->body->pass(this); // Sema on the body of the default case.
}

/// Semantic Analysis over a MatchStmt.
///
/// MatchStmts are valid if and only if the condition and cases are valid.
/// It also checks if the expression is boolean, both cases exist.
void Sema::visit(MatchStmt *stmt) {
  stmt->expr->pass(this); // Sema on the condition.

  for (std::unique_ptr<MatchCase> &s : stmt->cases) {
    s->pass(this); // Sema on each case.
  }

  // Check that boolean matches have a true and false case.
  if (stmt->expr->T->isBooleanType() && stmt->cases.size() != 2) {
    fatal("boolean match statement must have exactly two cases", 
        { stmt->span.file, stmt->span.line, stmt->span.col });
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
  switch (stmt->T->compare(parentFunctionType->getReturnType())) {
    case 2: // Attempt to implicitly cast the return type.
      if (stmt->expr->T->canCastTo(parentFunctionType->getReturnType())) {
        stmt->expr = std::make_unique<ImplicitCastExpr>(
            std::move(stmt->expr),
            parentFunctionType->getReturnType()->toString(), 
            parentFunctionType->getReturnType(),
            stmt->span
        );
        return;
      } else {
        warn("cannot cast downwards: " + stmt->expr->T->toString() + 
            " to " + parentFunctionType->getReturnType()->toString(), 
            { stmt->span.file, stmt->span.line, stmt->span.col });
        break;
      }
    case 1: return;
    default: break;
  }

  /// UNRECOVERABLE: If the type was not exact or could not be casted.
  fatal("return type mismatch", { stmt->span.file, 
          stmt->span.line, stmt->span.col });
}
