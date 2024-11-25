#include "Decl.h"
#include "../../include/AST/Expr.h"

using std::string;
using std::unique_ptr;
using std::vector;

using namespace artus;

/* Decl Implementation ----------------------------------------------------===*/

Decl::Decl(const Span &span) : span(span) {}

const Span &Decl::getSpan() const { return span; }

/* NamedDecl Implementation -----------------------------------------------===*/

NamedDecl::NamedDecl(const string &name, const Span &span, bool priv) 
    : Decl(span), name(name), priv(priv) {}

const string &NamedDecl::getName() const { return name; }

bool NamedDecl::isPrivate() const { return priv; }

void NamedDecl::setPrivate() { priv = true; }

void NamedDecl::setPublic() { priv = false; }

/* ScopedDecl Implementation ----------------------------------------------===*/

ScopedDecl::ScopedDecl(const string &name, Scope *scope, const Span &span,
                       bool priv)
    : NamedDecl(name, span, priv), scope(scope) {}

Scope *ScopedDecl::getScope() const { return scope; }

/* PackageUnitDecl Implementation -----------------------------------------===*/

PackageUnitDecl::PackageUnitDecl(const string &id, vector<string> imports, 
                                 Scope *scope, vector<unique_ptr<Decl>> decls)
    : identifier(id), imports(std::move(imports)), decls(std::move(decls)),
      scope(scope) {}

void PackageUnitDecl::pass(ASTVisitor *visitor) { visitor->visit(this); }

const string &PackageUnitDecl::getIdentifier() const { return identifier; }

const vector<string> &PackageUnitDecl::getImports() const { return imports; }

void PackageUnitDecl::addDecl(unique_ptr<Decl> decl) 
{ decls.push_back(std::move(decl)); }

void PackageUnitDecl::addImport(const string &import)
{ imports.push_back(import); }

/* VarDecl Implementation -------------------------------------------------===*/

VarDecl::VarDecl(const string &name, const Type *T, unique_ptr<Expr> init, 
                 const bool mut, const Span &span) 
    : NamedDecl(name, span), T(T), init(std::move(init)), mut(mut) {}
      
void VarDecl::pass(ASTVisitor *visitor) { visitor->visit(this); }

const Type *VarDecl::getType() const { return T; }

bool VarDecl::isParam() const { return !init; }

unsigned VarDecl::isMutable() const { return mut; }

bool VarDecl::canImport() const { return false; }

/* ParamVarDecl Implementation --------------------------------------------===*/

ParamVarDecl::ParamVarDecl(const string &name, const Type *T, const bool mut,
                           const Span &span)
    : VarDecl(name, T, nullptr, mut, span) {}

void ParamVarDecl::pass(ASTVisitor *visitor) { visitor->visit(this); }

/* FunctionDecl Implementation --------------------------------------------===*/

FunctionDecl::FunctionDecl(const string &name, const Type *T,
                           vector<unique_ptr<ParamVarDecl>> params,
                           unique_ptr<Stmt> body, Scope *scope, 
                           const Span &span, bool priv)
    : ScopedDecl(name, scope, span, priv), T(T), params(std::move(params)),
      body(std::move(body)) {}

void FunctionDecl::pass(ASTVisitor *visitor) { visitor->visit(this); }

const Type *FunctionDecl::getType() const { return T; }

size_t FunctionDecl::getNumParams() const { return params.size(); }

const ParamVarDecl *FunctionDecl::getParam(size_t i) const 
{ return i < params.size() ? params[i].get() : nullptr; }

bool FunctionDecl::canImport() const { return true; }

/* EnumDecl Implementation ------------------------------------------------===*/

EnumDecl::EnumDecl(const string &name, vector<string> variants, const Type *T,
                   const Span &span, bool priv)
    : NamedDecl(name, span, priv), variants(std::move(variants)), T(T) {}

void EnumDecl::pass(ASTVisitor *visitor) { visitor->visit(this); }

const Type *EnumDecl::getType() const { return T; }

size_t EnumDecl::getNumVariants() const { return variants.size(); }

int EnumDecl::getVariantIndex(const string &variant) const {
  for (size_t i = 0; i < variants.size(); ++i) {
    if (variants[i] == variant) 
      return i;
  }
  return -1;
}

bool EnumDecl::canImport() const { return true; }

/* FieldDecl Implementation -----------------------------------------------===*/

FieldDecl::FieldDecl(const string &name, const Type *T, const bool mut,
                     const Span &span)
    : NamedDecl(name, span), T(T), mut(mut) {}

void FieldDecl::pass(ASTVisitor *visitor) { visitor->visit(this); }

const Type *FieldDecl::getType() const { return T; }

bool FieldDecl::isMutable() const { return mut; }

bool FieldDecl::canImport() const { return false; }

/* StructDecl Implementation ----------------------------------------------===*/

StructDecl::StructDecl(const string &name, vector<unique_ptr<FieldDecl>> fields,
                       Scope *scope, const Type *T, const Span &span, bool priv)
    : ScopedDecl(name, scope, span, priv), fields(std::move(fields)), traits(),
    T(T) {}

void StructDecl::pass(ASTVisitor *visitor) { visitor->visit(this); }

size_t StructDecl::getNumFields() const { return fields.size(); }

const FieldDecl *StructDecl::getField(size_t i) const 
{ return i < fields.size() ? fields[i].get() : nullptr; }

int StructDecl::getFieldIndex(const string &field) const {
  for (size_t i = 0; i < fields.size(); ++i) {
    if (fields[i]->getName() == field) 
      return i;
  }
  return -1;
}

const Type *StructDecl::getFieldType(const string &name) const {
  for (const unique_ptr<FieldDecl> &field : this->fields) {
    if (field->getName() == name) 
      return field->getType();
  }
  return nullptr;
}

bool StructDecl::isFieldMutable(const string &name) const {
  for (const unique_ptr<FieldDecl> &field : this->fields) {
    if (field->getName() == name) {
      return field->isMutable();
    }
  }
  return false;
}

const Type *StructDecl::getType() const { return T; }

void StructDecl::addTrait(const string &trait) { traits.push_back(trait); }

bool StructDecl::canImport() const { return true; }
