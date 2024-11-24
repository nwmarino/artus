//>==- ReferenceAnalysis.cpp ----------------------------------------------==<//
// 
// The following source implements a reference analysis pass on a parsed AST.
// The purpose of the pass is to resolve potentially declared types and names.
//
// During type resolution, declared types are expected to already be held in the
// AST context type table. If an absolute type by some identifier is not found,
// then the pass will deem the type as unresolved.
//
//>==----------------------------------------------------------------------==<//

#include "../../include/AST/Expr.h"
#include "../../include/Core/Logger.h"
#include "../../include/Sema/ReferenceAnalysis.h"
#include "../../include/Sema/Scope.h"
#include "../../include/Sema/Type.h"

using namespace artus;

ReferenceAnalysis::ReferenceAnalysis(Context *ctx)
    : ctx(ctx), globalScope(nullptr), localScope(nullptr) {
  for (PackageUnitDecl *pkg : ctx->cache->getUnits()) {
    pkg->pass(this); // Pass on each package unit.
  }
}

const Type *ReferenceAnalysis::resolveType(const string &ident, 
                                           const SourceLocation &loc) const {
  const Type *resolvedType = ctx->getType(ident);
  if (!resolvedType || !resolvedType->isAbsolute()) {
    fatal("unresolved type: " + ident, loc);
  }

  return resolvedType;
}

const Decl *ReferenceAnalysis::resolveReference(const string &ident, 
                                                const SourceLocation &loc) const {
  if (const Decl *decl = this->localScope->getDecl(ident)) {
    return decl;
  }

  fatal("unresolved reference: " + ident, loc);
}

void ReferenceAnalysis::visit(PackageUnitDecl *decl) {
  this->globalScope = decl->scope;

  // Reconstruct function types.
  for (const std::unique_ptr<Decl> &d : decl->decls) {
    if (FunctionDecl *FD = dynamic_cast<FunctionDecl *>(d.get())) {
      if (!FD->T->isAbsolute()) {
        const FunctionType *FT = dynamic_cast<const FunctionType *>(FD->T);
        assert(FT && "expected function type");

        const Type *RT = FT->getReturnType();
        vector<const Type *> PT = FT->getParamTypes();

        // Resolve the absolute return type.
        if (!RT->isAbsolute()) {
          RT = resolveType(RT->toString(),
              { FD->getSpan().file, FD->getSpan().line, FD->getSpan().col });
        }

        // Check that each parameter type is absolute.
        for (const Type *T : PT) {
          // Resolve the absolute parameter type.
          if (!T->isAbsolute()) {
            T = resolveType(T->toString(),
                { FD->getSpan().file, FD->getSpan().line, FD->getSpan().col });
          }
        }

        // Reconstruct the function type.
        delete FD->T;
        FD->T = new FunctionType(RT, PT);
      }
    }
  }

  // Pass on each declaration.
  for (const std::unique_ptr<Decl> &d : decl->decls) {
    d->pass(this);
  }

  this->globalScope = nullptr;
}

void ReferenceAnalysis::visit(FunctionDecl *decl) {
  assert(decl->T->isAbsolute() && 
      ("invalid function type: " + decl->getName()).c_str());
  this->localScope = decl->scope;
  for (const std::unique_ptr<ParamVarDecl> &p : decl->params) {
    p->pass(this);
  }
  decl->body->pass(this);
  this->localScope = localScope->getParent();
}

void ReferenceAnalysis::visit(ParamVarDecl *decl) {
  // Resolve the absolute type.
  if (!decl->T->isAbsolute()) {
    decl->T = resolveType(decl->T->toString(),
        { decl->span.file, decl->span.line, decl->span.col });
  }

  assert(decl->T->isAbsolute() && 
      ("invalid parameter type: " + decl->T->toString()).c_str());
}

void ReferenceAnalysis::visit(VarDecl *decl) {
  // Resolve the absolute type.
  if (!decl->T->isAbsolute()) {
    decl->T = resolveType(decl->T->toString(),
        { decl->span.file, decl->span.line, decl->span.col });
  }

  // Pass after type resolution, as expressions may depend on the type.
  if (decl->init) {
    decl->init->pass(this);
  }

  assert(decl->T->isAbsolute() && 
      ("invalid variable type: " + decl->T->toString()).c_str());
}

void ReferenceAnalysis::visit(FieldDecl *decl) {
  // Resolve the absolute type.
  if (!decl->T->isAbsolute()) {
    decl->T = resolveType(decl->T->toString(),
        { decl->span.file, decl->span.line, decl->span.col });
  }

  assert(decl->T->isAbsolute() && 
      ("invalid field type: " + decl->T->toString()).c_str());
}

void ReferenceAnalysis::visit(StructDecl *decl) {
  this->localScope = decl->scope;
  for (const std::unique_ptr<FieldDecl> &f : decl->fields) {
    f->pass(this);
  }
  this->localScope = localScope->getParent();
}

void ReferenceAnalysis::visit(ImplicitCastExpr *expr) {
  expr->expr->pass(this);

  // Resolve the absolute type.
  if (!expr->T->isAbsolute()) {
    expr->T = resolveType(expr->T->toString(),
        { expr->span.file, expr->span.line, expr->span.col });
  }

  assert(expr->T->isAbsolute() && 
      ("invalid cast type: " + expr->T->toString()).c_str());
}

void ReferenceAnalysis::visit(ExplicitCastExpr *expr) {
  expr->expr->pass(this);

  // Resolve the absolute type.
  if (!expr->T->isAbsolute()) {
    expr->T = resolveType(expr->T->toString(),
        { expr->span.file, expr->span.line, expr->span.col });
  }

  assert(expr->T->isAbsolute() && 
      ("invalid cast type: " + expr->T->toString()).c_str());
}

void ReferenceAnalysis::visit(DeclRefExpr *expr) {
  const Decl *decl = resolveReference(expr->ident,
      { expr->span.file, expr->span.line, expr->span.col });

  // Propagate the type of the expression.
  if (const VarDecl *VarDecl = dynamic_cast<const class VarDecl *>(decl)) {
    expr->T = VarDecl->T;
  } else {
    fatal("expected variable reference: " + expr->ident,
        { expr->span.file, expr->span.line, expr->span.col });
  }

  assert(expr->T->isAbsolute() && 
      ("invalid reference type: " + expr->T->toString()).c_str());
}

void ReferenceAnalysis::visit(CallExpr *expr) {
  // Resolve the function reference.
  const Decl *decl = resolveReference(expr->ident,
      { expr->span.file, expr->span.line, expr->span.col });

  // Propagate the type of the expression based on the function return type.
  if (const FunctionDecl *callee = dynamic_cast<const FunctionDecl *>(decl)) {
    const FunctionType *FT = dynamic_cast<const FunctionType *>(callee->T);
    assert(FT && ("expected function type: " + expr->ident).c_str());
    expr->T = FT->getReturnType();
  } else {
    fatal("expected function: " + expr->ident, 
        { expr->span.file, expr->span.line, expr->span.col });
  }

  // Pass on each of the arguments.
  for (size_t i = 0; i < expr->getNumArgs(); i++) {
    expr->getArg(i)->pass(this);
  }

  assert(expr->T->isAbsolute() && 
      ("invalid call expression type: " + expr->T->toString()).c_str());
}

void ReferenceAnalysis::visit(UnaryExpr *expr) { 
  expr->base->pass(this); 
}

void ReferenceAnalysis::visit(BinaryExpr *expr) { 
  expr->lhs->pass(this); 
  expr->rhs->pass(this);
}

void ReferenceAnalysis::visit(BooleanLiteral *expr) {
  // Check that a boolean literal is of type 'bool'.
  if (expr->T->toString() != "bool") {
    fatal("expected boolean type", { expr->span.file, 
        expr->span.line, expr->span.col });
  }
}

void ReferenceAnalysis::visit(IntegerLiteral *expr) {
  // Check that an integer literal is of an integer type.
  if (!expr->T->isIntegerType()) {
    fatal("expected integer type", { expr->span.file, 
        expr->span.line, expr->span.col });
  }
}

void ReferenceAnalysis::visit(FPLiteral *expr) {
  // Check that a floating point literal of a floating point type.
  if (!expr->T->isFloatingPointType()) {
    fatal("expected floating point type", { expr->span.file, 
        expr->span.line, expr->span.col });
  }
}

void ReferenceAnalysis::visit(CharLiteral *expr) {
  // Check that a character literal is of type 'char'.
  if (expr->T->toString() != "char") {
    fatal("expected character type", { expr->span.file, 
        expr->span.line, expr->span.col });
  }
}

void ReferenceAnalysis::visit(StringLiteral *expr) {
  // Check that a string literal is of a string type.
  if (!expr->T->isStringType()) {
    fatal("expected string type", { expr->span.file, 
        expr->span.line, expr->span.col });
  }
}

void ReferenceAnalysis::visit(NullExpr *expr) {
  // Check that a 'null' type is a pointer.
  if (!expr->T || !expr->T->isPointerType()) {
    fatal("'null' type must be a pointer", 
        { expr->span.file, expr->span.line, expr->span.col });
  }

  // Resolve the absolute type.
  if (!expr->T->isAbsolute()) {
    expr->T = resolveType(expr->T->toString(),
        { expr->span.file, expr->span.line, expr->span.col });
  }

  assert(expr->T->isAbsolute() && 
      ("invalid null pointer type: " + expr->T->toString()).c_str());
}

void ReferenceAnalysis::visit(ArrayExpr *expr) {
  // Pass on each of the expressions.
  for (std::unique_ptr<Expr> &e : expr->exprs) {
    e->pass(this);
  }

  // Resolve the absolute type.
  if (!expr->T->isAbsolute()) {
    expr->T = resolveType(expr->T->toString(),
        { expr->span.file, expr->span.line, expr->span.col });
  }

  assert(expr->T->isAbsolute() &&
      ("invalid array list type: " + expr->T->toString()).c_str());
}

void ReferenceAnalysis::visit(ArraySubscriptExpr *expr) {
  expr->base->pass(this);
  expr->index->pass(this);

  // Assign the access expression type.
  if (expr->base->getType()->isStringType()) {
    // Handle string subscripting, i.e. 'str[0]'.
    expr->T = ctx->getType("char");
  } else if (expr->base->getType()->isPointerType()) {
    // Handle pointer subscripting, i.e. 'ptr[0]'.
    const PointerType *PT = dynamic_cast<const PointerType *>(expr->base->getType());
    assert(PT && "expected pointer type for array access");
    expr->T = PT->getPointeeType();
  } else {
    // Handle generic array subscripting, i.e. 'arr[0]'.
    const ArrayType *AT = dynamic_cast<const ArrayType *>(expr->base->getType());
    assert(AT && "expected array type");
    expr->T = AT->getElementType();
  }

  // Resolve the absolute type.
  if (!expr->T->isAbsolute()) {
    expr->T = resolveType(expr->T->toString(),
        { expr->span.file, expr->span.line, expr->span.col });
  }

  assert(expr->T->isAbsolute() && 
      ("invalid array access type: " + expr->T->toString()).c_str());
}

void ReferenceAnalysis::visit(StructInitExpr *expr) {
  // Resolve the struct.
  const Decl *decl = resolveReference(expr->name,
      { expr->span.file, expr->span.line, expr->span.col });

  const StructDecl *SD = dynamic_cast<const StructDecl *>(decl);
  if (!SD) {
    fatal("expected struct: " + expr->name, 
        { expr->span.file, expr->span.line, expr->span.col });
  }

  // Check that the struct has the correct number of fields.
  if (expr->fields.size() != SD->fields.size()) {
    fatal("expected " + std::to_string(SD->fields.size()) + " fields, got " 
        + std::to_string(expr->fields.size()), { expr->span.file,
        expr->span.line, expr->span.col });
  }
  
  // Pass on each of the field initializers.
  for (const pair<string, std::unique_ptr<Expr>> &f : expr->fields) {
    f.second->pass(this);
  }

  // Type is null by parser, resolve it.
  expr->T = resolveType(expr->name,
      { expr->span.file, expr->span.line, expr->span.col });

  assert(expr->T->isAbsolute() && 
      ("invalid struct type: " + expr->T->toString()).c_str());
}

void ReferenceAnalysis::visit(CompoundStmt *stmt) {
  this->localScope = stmt->scope;
  for (const std::unique_ptr<Stmt> &s : stmt->stmts) {
    s->pass(this);
  }
  this->localScope = localScope->getParent();
}

void ReferenceAnalysis::visit(BreakStmt *stmt) { /* no work to be done */ }
void ReferenceAnalysis::visit(ContinueStmt *stmt) { /* no work to be done */ }

void ReferenceAnalysis::visit(DeclStmt *stmt) { 
  stmt->decl->pass(this); 
}

void ReferenceAnalysis::visit(IfStmt *stmt) {
  stmt->cond->pass(this);
  stmt->thenStmt->pass(this);
  if (stmt->hasElse()) {
    stmt->elseStmt->pass(this);
  }
}

void ReferenceAnalysis::visit(WhileStmt *stmt) {
  stmt->cond->pass(this);
  stmt->body->pass(this);
}

void ReferenceAnalysis::visit(UntilStmt *stmt) {
  stmt->cond->pass(this);
  stmt->body->pass(this);
}

void ReferenceAnalysis::visit(CaseStmt *stmt) {
  stmt->expr->pass(this);
  stmt->body->pass(this);
}

void ReferenceAnalysis::visit(DefaultStmt *stmt) {
  stmt->body->pass(this);
}

void ReferenceAnalysis::visit(MatchStmt *stmt) {
  stmt->expr->pass(this);
  for (std::unique_ptr<MatchCase> &s : stmt->cases) {
    s->pass(this);
  }
}

void ReferenceAnalysis::visit(RetStmt *stmt) { 
  stmt->expr->pass(this); 

  // Forward the expression type.
  stmt->T = stmt->expr->T;
}
