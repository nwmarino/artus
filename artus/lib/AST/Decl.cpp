#include "../../include/AST/Expr.h"

using std::string;
using std::unique_ptr;
using std::vector;

using namespace artus;

/* Decl Implementation ----------------------------------------------------===*/

Decl::Decl(const Span &span) : span(span) {}

const Span &Decl::getSpan() const { return span; }

/* NamedDecl Implementation -----------------------------------------------===*/

NamedDecl::NamedDecl(const string &name, const Span &span) 
    : Decl(span), name(name) {}

const string &NamedDecl::getName() const { return name; }

/* ScopedDecl Implementation ----------------------------------------------===*/

ScopedDecl::ScopedDecl(const string &name, Scope *scope, const Span &span)
    : NamedDecl(name, span), scope(scope) {}

Scope *ScopedDecl::getScope() const { return scope; }

/* PackageUnitDecl Implementation -----------------------------------------===*/

PackageUnitDecl::PackageUnitDecl(const string &id, vector<string> imports, 
                                 Scope *scope, vector<unique_ptr<Decl>> decls)
    : identifier(id), imports(std::move(imports)), decls(std::move(decls)),
      scope(scope) {}

void PackageUnitDecl::pass(ASTVisitor *visitor) { visitor->visit(this); }

const string &PackageUnitDecl::getIdentifier() const { return identifier; }

const vector<string> &PackageUnitDecl::getImports() const { return imports; }

void PackageUnitDecl::addDecl(unique_ptr<Decl> decl) { 
  decls.push_back(std::move(decl)); 
}

void PackageUnitDecl::addImport(const string &import) { 
  imports.push_back(import); 
}

/* LabelDecl Implementation -----------------------------------------------===*/

LabelDecl::LabelDecl(const string &name, const Span &span)
    : NamedDecl(name, span), stmt(nullptr) {}

void LabelDecl::pass(ASTVisitor *visitor) { visitor->visit(this); }

const Stmt *LabelDecl::getStmt() const { return stmt; }

void LabelDecl::setStmt(const Stmt *stmt) { this->stmt = stmt; }

/* ParamVarDecl Implementation --------------------------------------------===*/

ParamVarDecl::ParamVarDecl(const string &name, const Type *T, const bool mut,
                           const Span &span)
    : NamedDecl(name, span), T(T), mut(mut) {}

void ParamVarDecl::pass(ASTVisitor *visitor) { visitor->visit(this); }

const Type *ParamVarDecl::getType() const { return T; }

unsigned ParamVarDecl::isMutable() const { return mut; }

/* FunctionDecl Implementation --------------------------------------------===*/

FunctionDecl::FunctionDecl(const string &name, const Type *T,
                           vector<unique_ptr<ParamVarDecl>> params,
                           unique_ptr<Stmt> body, Scope *scope, const Span &span)
    : ScopedDecl(name, scope, span), T(T), params(std::move(params)),
      body(std::move(body)) {}

void FunctionDecl::pass(ASTVisitor *visitor) { visitor->visit(this); }

const Type *FunctionDecl::getType() const { return T; }

size_t FunctionDecl::getNumParams() const { return params.size(); }

const ParamVarDecl *FunctionDecl::getParam(size_t i) const {
  return i < params.size() ? params[i].get() : nullptr;
}

/* VarDecl Implementation -------------------------------------------------===*/

VarDecl::VarDecl(const string &name, const Type *T, unique_ptr<Expr> init, 
                 const bool mut, const Span &span) 
    : NamedDecl(name, span), T(T), init(std::move(init)), mut(mut) {}
      
void VarDecl::pass(ASTVisitor *visitor) { visitor->visit(this); }

const Type *VarDecl::getType() const { return T; }

unsigned VarDecl::isMutable() const { return mut; }
