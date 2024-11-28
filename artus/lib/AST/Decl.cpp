//>==- Decl.cpp -----------------------------------------------------------<==//
//
// The following source contains implementations for classes defined in Decl.h
//
//>==----------------------------------------------------------------------==<//

#include "../../include/AST/Decl.h"
#include "../../include/AST/Expr.h"
#include "../../include/AST/Stmt.h"
#include "../../include/Sema/Scope.h"

using namespace artus;

//>==- NamedDecl Implementation -------------------------------------------==<//
//
// Named declarations are inline declarations that may define a symbol and
// thereby inhabit a scope.
//
//>==----------------------------------------------------------------------==<//

NamedDecl::NamedDecl(const std::string &name, bool priv, const Span &span)
    : Decl(span), name(name), priv(priv) {}

//>==- ScopedDecl Implementation ------------------------------------------==<//
//
// Scoped declarations contain a link to some local scope quantifier. That link
// may be used to resolve symbols during reference analysis passes.
//
//>==----------------------------------------------------------------------==<//

ScopedDecl::ScopedDecl(const std::string &name, Scope *scope, bool priv, 
                       const Span &span)
    : NamedDecl(name, priv, span), scope(scope) {}

ScopedDecl::~ScopedDecl() { delete scope; }

//>==- VarDecl Implementation ---------------------------------------------==<//
//
// Variable declarations are inline declarations that define a variable symbol.
// They may also contain an initializer expression that is either explicitly
// stated in the source or is derived by the parser.
//
// Variables are named declarations in-scope that may not be imported.
//
// Variable declarations may appear as follows:
//
// var-decl:
//      | 'fix' <identifier> ':' <type> '=' <expr>
//      | 'mut' <identifier> ':' <type>
//      | 'mut' <identifier> ':' <type> '=' <expr>
//
//>==-----------------------------------------------------------------------==<//

VarDecl::VarDecl(const std::string &name, const Type *T, std::unique_ptr<Expr> init,
                 bool mut, const Span &span)
    : NamedDecl(name, false, span), T(T), init(std::move(init)), mut(mut){}

//>==- ParamVarDecl Implementation ----------------------------------------==<//
//
// Parameter variables are special variables that may not possess an initializer
// and are instead special variable references for function signatures. They
// receive similar treatment to regular variables during code generation.
//
// Parameter declarations may appear as follows:
//
// param-decl: 
//      | <identifier> ':' <type>
//      | 'mut' <identifier> ':' <type>
//
//>==----------------------------------------------------------------------==<//

ParamVarDecl::ParamVarDecl(const std::string &name, const Type *T, bool mut,
                           const Span &span)
    : VarDecl(name, T, nullptr, mut, span) {}

//>==- FunctionDecl Implementation ----------------------------------------==<//
//
// Function declarations may appear as follows:
//
// function-decl:
//      | fn '@' <identifier> '(' [param-list] ')' '->' <type> <stmt>
//
// param-list:
//      | <param-decl> (',' <param-decl>)*
//
//>==----------------------------------------------------------------------==<//

FunctionDecl::FunctionDecl(const std::string &name, const Type *T, Scope *scope,
                           std::vector<std::unique_ptr<ParamVarDecl>> params,
                           bool priv, const Span &span)
    : ScopedDecl(name, scope, priv, span), T(T), params(std::move(params)),
      body(nullptr) {}

FunctionDecl::FunctionDecl(const std::string &name, const Type *T, Scope *scope,
                           std::vector<std::unique_ptr<ParamVarDecl>> params,
                           std::unique_ptr<Stmt> body, bool priv, const Span &span)
    : ScopedDecl(name, scope, priv, span), T(T), params(std::move(params)),
      body(std::move(body)) {}
  
FunctionDecl::~FunctionDecl() {
  delete this->scope;
  delete this->T;
}

const ParamVarDecl *FunctionDecl::getParam(size_t i) const 
{ return i < params.size() ? params[i].get() : nullptr; }

bool FunctionDecl::isMain() const { return name == "main"; }

bool FunctionDecl::hasBody() const { return body != nullptr; }

//>==- EnumDecl Implementation --------------------------------------------==<//
//
// Enum declarations may appear as follows:
//
// enum-decl:
//      | 'enum' <identifier> '{' <variant-list> '}'
//
// variant-list:
//      | <identifier> (',' <identifier>)*
//
//>==----------------------------------------------------------------------==<//

EnumDecl::EnumDecl(const std::string &name, std::vector<std::string> variants,
                   const Type *T, bool priv, const Span &span)
    : NamedDecl(name, priv, span), variants(std::move(variants)), T(T) {}

int EnumDecl::getVariantIndex(const std::string &variant) const {
  for (std::size_t i = 0; i < variants.size(); ++i) {
    if (variants[i] == variant)
      return i;
  }

  return -1;
}

//>==- FieldDecl Implementation -------------------------------------------==<//
//
// Field declarations may appear as follows:
//
// field-decl:
//      | <identifier> ':' <type>
//      | 'mut' <identifier> ':' <type>
//      | 'priv' <identifier> ':' <type>
//      | 'priv' 'mut' <identifier> ':' <type>
//
//>==----------------------------------------------------------------------==<//

FieldDecl::FieldDecl(const std::string &name, const Type *T, bool mut, bool priv,
                     const Span &span)
    : NamedDecl(name, priv, span), T(T), mut(mut) {}

//>==- StructDecl Implementation ------------------------------------------==<//
//
// Struct declarations are inline declarations that define a struct type.
//
// struct-decl:
//      | 'struct' <identifier> '{' <field-list> '}'
//
// field-list:
//      | <field-decl> (',' <field-decl>)*
//
//>==----------------------------------------------------------------------==<//

StructDecl::StructDecl(const std::string &name, const Type *T, Scope *scope,
                       std::vector<std::unique_ptr<FieldDecl>> fields, bool priv,
                       const Span &span)
    : ScopedDecl(name, scope, priv, span), fields(std::move(fields)), T(T) {}

const FieldDecl *StructDecl::getField(std::size_t i) const 
{ return i < fields.size() ? fields[i].get() : nullptr; }

int StructDecl::getFieldIndex(const std::string &field) const {
  for (std::size_t i = 0; i < fields.size(); ++i) {
    if (fields[i]->getName() == field) 
      return i;
  }

  return -1;
}

const Type *StructDecl::getFieldType(const std::string &name) const {
  for (const std::unique_ptr<FieldDecl> &field : this->fields) {
    if (field->getName() == name)
      return field->getType();
  }

  return nullptr;
}

bool StructDecl::isFieldMutable(const std::string &name) const {
  for (const std::unique_ptr<FieldDecl> &field : this->fields) {
    if (field->getName() == name)
      return field->isMutable();
  }

  return false;
}
