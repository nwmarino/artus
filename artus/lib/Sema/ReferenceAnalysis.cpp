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

#include <cassert>

#include "../../include/AST/Decl.h"
#include "../../include/AST/DeclBase.h"
#include "../../include/AST/Expr.h"
#include "../../include/AST/Stmt.h"
#include "../../include/Core/Logger.h"
#include "../../include/Core/SourcePath.h"
#include "../../include/Sema/ReferenceAnalysis.h"
#include "../../include/Sema/Scope.h"
#include "../../include/Sema/Type.h"

using namespace artus;

ReferenceAnalysis::ReferenceAnalysis(Context *ctx)
    : ctx(ctx), globalScope(nullptr), localScope(nullptr) {
  for (PackageUnitDecl *pkg : ctx->PM->getPackages())
    pkg->pass(this);
}

const Type *ReferenceAnalysis::resolveType(const std::string &ident, 
                                           const SourceLocation &loc) const {
  const Type *resolvedType = ctx->getType(ident);
  if (resolvedType && resolvedType->isAbsolute())
    return resolvedType;

  // Before crashing, try to resolve a local type declaration.
  if (const Decl *decl = resolveReference(ident, loc)) {
    if (const StructDecl *SD = dynamic_cast<const StructDecl *>(decl))
      return SD->getType();
    else if (const EnumDecl *ED = dynamic_cast<const EnumDecl *>(decl))
      return ED->getType();
  }

  fatal("unresolved type: " + ident, loc);
}

const Decl *ReferenceAnalysis::resolveReference(const std::string &ident, 
                                                const SourceLocation &loc) const {
  if (const Decl *decl = this->localScope->getDecl(ident))
    return decl;

  fatal("unresolved reference: " + ident, loc);
}

void ReferenceAnalysis::importDependencies(PackageUnitDecl *pkg) {
  for (ImportDecl *imp : pkg->getImports()) {
    PackageUnitDecl *depPkg = ctx->resolvePackage(imp->getPath().toString(), imp->getStartLoc());

    importDependencies(depPkg);

    for (Decl *decl : depPkg->decls) {
      NamedDecl *ND = dynamic_cast<NamedDecl *>(decl);

      if (ND && ND->canImport() && !ND->isPrivate())
        pkg->scope->addDecl(ND);
    }
  }
}

void ReferenceAnalysis::checkParent(const Decl *decl, 
                                    const SourceLocation &loc) const {
  if (decl->getParent() != currPkg) {
    if (const NamedDecl *ND = dynamic_cast<const NamedDecl *>(decl))
      fatal("unset declaration parent: " + ND->getName(), loc);
    else
      fatal("unset declaration parent", loc);
  }
}
    

void ReferenceAnalysis::visit(PackageUnitDecl *decl) {
  this->currPkg = decl;
  this->globalScope = decl->scope;
  this->localScope = this->globalScope;

  // Check for duplicate imports.
  for (std::size_t i = 0; i < decl->imports.size(); i++) {
    for (std::size_t j = i + 1; j < decl->imports.size(); j++) {
      if (decl->imports[i]->getPath().curr == decl->imports[j]->getPath().curr)
        fatal("duplicate import: " + decl->imports[i]->getPath().curr,
            decl->imports.at(j)->getStartLoc());
    }
  }

  importDependencies(decl);

  // Reconstruct function types.
  for (Decl *d : decl->decls) {
    if (FunctionDecl *FD = dynamic_cast<FunctionDecl *>(d)) {
      if (!FD->T->isAbsolute()) {
        const FunctionType *FT = dynamic_cast<const FunctionType *>(FD->T);
        assert(FT && "expected function type");

        const Type *RT = FT->getReturnType();
        std::vector<const Type *> PT = FT->getParamTypes();

        // Resolve the absolute return type.
        if (!RT->isAbsolute())
          RT = resolveType(RT->toString(), FD->getStartLoc());

        // Check that each parameter type is absolute.
        unsigned idx = 0;
        for (const Type *T : PT) {
          // Resolve the absolute parameter type.
          if (!T->isAbsolute())
            PT[idx] = resolveType(T->toString(), FD->getStartLoc());

          idx++;
        }

        // Reconstruct the function type.
        delete FD->T;
        FD->T = new FunctionType(RT, PT);
      }
    }
  }

  // Pass on each declaration.
  for (Decl *d : decl->decls)
    d->pass(this);

  this->currPkg = nullptr;
  this->globalScope = nullptr;
  this->localScope = nullptr;
}

void ReferenceAnalysis::visit(ImportDecl *decl) { /* no work to be done */ }

void ReferenceAnalysis::visit(FunctionDecl *decl) {
  checkParent(decl, decl->getStartLoc());
  if (!decl->getType()->isAbsolute())
    fatal("invalid function type: " + decl->getName());

  this->localScope = decl->scope;
  for (const std::unique_ptr<ParamVarDecl> &p : decl->params)
    p->pass(this);

  decl->body->pass(this);
  this->localScope = localScope->getParent();
}

void ReferenceAnalysis::visit(ParamVarDecl *decl) {
  checkParent(decl, decl->getStartLoc());

  // Resolve the absolute type.
  if (!decl->T->isAbsolute())
    decl->T = resolveType(decl->T->toString(), decl->getStartLoc());

  assert(decl->T->isAbsolute() &&
      ("invalid parameter type: " + decl->T->toString()).c_str());
}

void ReferenceAnalysis::visit(VarDecl *decl) {
  if (!decl->getParent())
    decl->setParent(currPkg);

  // Resolve the absolute type.
  if (!decl->T->isAbsolute())
    decl->T = resolveType(decl->T->toString(), decl->getStartLoc());

  // Pass after type resolution, as expressions may depend on the type.
  if (decl->init)
    decl->init->pass(this);

  assert(decl->T->isAbsolute() && 
      ("invalid variable type: " + decl->T->toString()).c_str());
}

void ReferenceAnalysis::visit(EnumDecl *decl) 
{ checkParent(decl, decl->getStartLoc()); }

void ReferenceAnalysis::visit(FieldDecl *decl) {
  checkParent(decl, decl->getStartLoc());

  // Resolve the absolute type.
  if (!decl->T->isAbsolute())
    decl->T = resolveType(decl->T->toString(), decl->getStartLoc());

  assert(decl->T->isAbsolute() &&
      ("invalid field type: " + decl->T->toString()).c_str());
}

void ReferenceAnalysis::visit(StructDecl *decl) {
  checkParent(decl, decl->getStartLoc());
  this->localScope = decl->scope;

  for (const std::unique_ptr<FieldDecl> &f : decl->fields)
    f->pass(this);

  this->localScope = localScope->getParent();
}

void ReferenceAnalysis::visit(ImplicitCastExpr *expr) {
  expr->expr->pass(this);

  // Resolve the absolute type.
  if (!expr->T->isAbsolute())
    expr->T = resolveType(expr->T->toString(), expr->getStartLoc());

  assert(expr->T->isAbsolute() &&
      ("invalid cast type: " + expr->T->toString()).c_str());
}

void ReferenceAnalysis::visit(ExplicitCastExpr *expr) {
  expr->expr->pass(this);

  // Resolve the absolute type.
  if (!expr->T->isAbsolute())
    expr->T = resolveType(expr->T->toString(), expr->getStartLoc());

  assert(expr->T->isAbsolute() &&
      ("invalid cast type: " + expr->T->toString()).c_str());
}

void ReferenceAnalysis::visit(DeclRefExpr *expr) {
  // Handle possible enum reference, which already have type references.
  if (expr->hasSpecifier()) {
    const EnumType *ET = dynamic_cast<const EnumType *>(ctx->getType(
        expr->getType()->toString()));
    if (!ET) {
      fatal("expected enum variant reference: " + expr->getIdent(), 
          expr->getStartLoc());
    }

    if (ET->getVariant(expr->getSpecifier()) < 0)
      fatal("unresolved variant: " + expr->getIdent(), expr->getStartLoc());

    // Propogate the type of the expression.
    expr->T = ET;

    // Resolve the enum declaration.
    expr->decl = resolveReference(expr->getType()->toString(),
        expr->getStartLoc());
  } else {
    const Decl *decl = resolveReference(expr->ident, expr->getStartLoc());

    if (const VarDecl *VarDecl = dynamic_cast<const class VarDecl *>(decl)) {
      expr->T = VarDecl->T; // Propogate the type.
      expr->decl = VarDecl;
    } else
      fatal("expected variable: " + expr->ident, expr->getStartLoc());
  }

  expr->setLValue();
  if (!expr->getType()->isAbsolute())
    fatal("unresolved reference type: " + expr->getType()->toString(), 
        expr->getStartLoc());
}

void ReferenceAnalysis::visit(CallExpr *expr) {
  // Resolve the function reference.
  const Decl *decl = resolveReference(expr->ident, expr->getStartLoc());

  // Propagate the type of the expression based on the function return type.
  if (const FunctionDecl *callee = dynamic_cast<const FunctionDecl *>(decl)) {
    const FunctionType *FT = dynamic_cast<const FunctionType *>(callee->T);
    assert(FT && ("expected function type: " + expr->ident).c_str());

    expr->T = FT->getReturnType();
    expr->decl = callee;
  } else
    fatal("expected function: " + expr->ident, expr->getStartLoc());

  // Pass on each of the arguments.
  for (std::size_t i = 0; i < expr->getNumArgs(); i++)
    expr->getArg(i)->pass(this);
  
  if (!expr->getType()->isAbsolute())
    fatal("unresolved call expression type: " + expr->getType()->toString(), 
        expr->getStartLoc());

  if (expr->getType()->isStructType())
    expr->setLValue();
}

void ReferenceAnalysis::visit(UnaryExpr *expr) 
{ expr->base->pass(this); }

void ReferenceAnalysis::visit(BinaryExpr *expr) { 
  expr->lhs->pass(this); 
  expr->rhs->pass(this);
}

void ReferenceAnalysis::visit(BooleanLiteral *expr) {
  // Check that a boolean literal is of a boolean type.
  if (!expr->T->isBooleanType())
    fatal("expected boolean type", expr->getStartLoc());
}

void ReferenceAnalysis::visit(IntegerLiteral *expr) {
  // Check that an integer literal is of an integer type.
  if (!expr->T->isIntegerType())
    fatal("expected integer type", expr->getStartLoc());
}

void ReferenceAnalysis::visit(FPLiteral *expr) {
  // Check that a floating point literal of a floating point type.
  if (!expr->T->isFloatingPointType())
    fatal("expected floating point type", expr->getStartLoc());
}

void ReferenceAnalysis::visit(CharLiteral *expr) {
  // Check that a character literal is of type 'char'.
  if (expr->T->toString() != "char")
    fatal("expected character type", expr->getStartLoc());
}

void ReferenceAnalysis::visit(StringLiteral *expr) {
  // Check that a string literal is of a string type.
  if (!expr->T->isStringType())
    fatal("expected string type", expr->getStartLoc());
}

void ReferenceAnalysis::visit(NullExpr *expr) {
  // Check that a 'null' type is a pointer.
  if (!expr->T || !expr->T->isPointerType())
    fatal("'null' type must be a pointer", expr->getStartLoc());

  // Resolve the absolute type.
  if (!expr->T->isAbsolute())
    expr->T = resolveType(expr->T->toString(), expr->getStartLoc());

  assert(expr->T->isAbsolute() &&
      ("invalid null pointer type: " + expr->T->toString()).c_str());
}

void ReferenceAnalysis::visit(ArrayExpr *expr) {
  // Pass on each of the expressions.
  for (std::unique_ptr<Expr> &e : expr->exprs)
    e->pass(this);

  // Resolve the absolute type.
  if (!expr->T->isAbsolute())
    expr->T = resolveType(expr->T->toString(), expr->getStartLoc());

  assert(expr->T->isAbsolute() &&
      ("invalid array list type: " + expr->T->toString()).c_str());
}

void ReferenceAnalysis::visit(ArraySubscriptExpr *expr) {
  expr->base->pass(this);
  expr->index->pass(this);

  // Assign the access expression type.
  if (expr->base->T->isStringType())
    // Handle string subscripting, i.e. 'str[0]'.
    expr->T = ctx->getType("char");
  else if (expr->base->T->isPointerType()) {
    // Handle pointer subscripting, i.e. 'ptr[0]'.
    const PointerType *PT = dynamic_cast<const PointerType *>(expr->base->T);
    assert(PT && "expected pointer type for array access");

    expr->T = PT->getPointeeType();
  } else {
    // Handle generic array subscripting, i.e. 'arr[0]'.
    const ArrayType *AT = dynamic_cast<const ArrayType *>(expr->base->T);
    assert(AT && "expected array type");

    expr->T = AT->getElementType();
  }

  // Resolve the absolute type.
  if (!expr->T->isAbsolute())
    expr->T = resolveType(expr->T->toString(), expr->getStartLoc());

  assert(expr->T->isAbsolute() &&
      ("invalid array access type: " + expr->T->toString()).c_str());
}

void ReferenceAnalysis::visit(StructInitExpr *expr) {
  // Resolve the struct.
  const Decl *decl = resolveReference(expr->name, expr->getStartLoc());

  const StructDecl *SD = dynamic_cast<const StructDecl *>(decl);
  if (!SD)
    fatal("expected struct: " + expr->name, expr->getStartLoc());

  // Check that the struct has the correct number of fields.
  if (expr->fields.size() != SD->fields.size()) {
    fatal("expected " + std::to_string(SD->fields.size()) + " fields, got "
        + std::to_string(expr->fields.size()), expr->getStartLoc());
  }
  
  // Pass on each of the field initializers.
  for (const std::pair<std::string, std::unique_ptr<Expr>> &f : expr->fields)
    f.second->pass(this);

  // Type is null by parser, resolve it.
  expr->T = resolveType(expr->name, expr->getStartLoc());
  expr->setDecl(SD);

  assert(expr->T->isAbsolute() &&
      ("invalid struct type: " + expr->T->toString()).c_str());
}

void ReferenceAnalysis::visit(MemberExpr *expr) {
  expr->base->pass(this);

  // Resolve the base as a struct reference.
  if (!expr->base->T->isStructType())
    fatal("expected struct reference", expr->getStartLoc());

  // Get the struct type of the base expression.
  const Decl *decl = resolveReference(expr->base->T->toString(), 
      expr->getStartLoc());

  const StructDecl *SD = dynamic_cast<const StructDecl *>(decl);
  if (!SD) {
    fatal("expected struct reference for member access: " + expr->getMember(),
        expr->getStartLoc());
  }

  // Set the type of the member expression according to the struct field type.
  const Type *fieldType = SD->getFieldType(expr->getMember());
  if (!fieldType) {
    fatal("field: " + expr->getMember() + " does not exist for struct " 
        + expr->base->T->toString(), expr->getStartLoc());
  }

  // Propogate the type.
  expr->T = fieldType;

  // Assign the field index.
  expr->index = SD->getFieldIndex(expr->getMember());

  if (!expr->getType()->isAbsolute())
    fatal("unresolved member expression type: " + expr->getType()->toString(), 
        expr->getStartLoc());

  expr->setLValue();
}

void ReferenceAnalysis::visit(CompoundStmt *stmt) {
  this->localScope = stmt->scope;

  for (const std::unique_ptr<Stmt> &s : stmt->stmts)
    s->pass(this);

  this->localScope = localScope->getParent();
}

void ReferenceAnalysis::visit(BreakStmt *stmt) { /* no work to be done */ }
void ReferenceAnalysis::visit(ContinueStmt *stmt) { /* no work to be done */ }

void ReferenceAnalysis::visit(DeclStmt *stmt) 
{ stmt->decl->pass(this); }

void ReferenceAnalysis::visit(IfStmt *stmt) {
  stmt->cond->pass(this);
  stmt->thenStmt->pass(this);
  if (stmt->hasElse())
    stmt->elseStmt->pass(this);
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

void ReferenceAnalysis::visit(DefaultStmt *stmt) 
{ stmt->body->pass(this); }

void ReferenceAnalysis::visit(MatchStmt *stmt) {
  stmt->expr->pass(this);
  for (std::unique_ptr<MatchCase> &s : stmt->cases)
    s->pass(this);
}

void ReferenceAnalysis::visit(RetStmt *stmt) { 
  stmt->expr->pass(this); 

  // Forward the expression type.
  stmt->T = stmt->expr->T;
}
