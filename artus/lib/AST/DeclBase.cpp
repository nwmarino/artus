//>==- DeclBase.cpp -------------------------------------------------------<==//
//
// The following source contains implementations for classes defined in
// DeclBase.h
//
//>==----------------------------------------------------------------------==<//

#include "../../include/AST/DeclBase.h"
#include "../../include/Sema/Scope.h"

using namespace artus;

//>==- DeclContext Implementation -----------------------------------------==<//
//
// A DeclContext is an important class in the model for declaration ownership 
// during compilation. In particular, the class holds the owning references to
// declarations for the lifetime of a package.
//
// The declarations contained in a DeclContext are a subset of those found in
// its parent package unit.
//
//>==----------------------------------------------------------------------==<//

Decl *DeclContext::addDeclaration(std::unique_ptr<Decl> decl) {
  Decl *declPtr = decl.get();
  decls.push_back(std::move(decl));

  if (NamedDecl *namedDeclPtr = dynamic_cast<NamedDecl *>(declPtr))
    map[namedDeclPtr->getName()] = namedDeclPtr;

  return declPtr;
}

NamedDecl *DeclContext::getDeclaration(const std::string &name) const {
  std::map<std::string, NamedDecl*>::const_iterator iter = map.find(name);
  if (iter != map.end())
    return iter->second;

  return nullptr;
}

std::vector<Decl *> DeclContext::getDeclarations() const {
  std::vector<Decl *> declarations = {};
  for (const std::unique_ptr<Decl> &decl : this->decls)
    declarations.push_back(decl.get());

  return declarations;
}

//>==- ImportDecl Implementation ------------------------------------------==<//
//
// Import Declarations may appear as follows:
//
// import-decl:
//      | 'import' <identifier>
//      | 'import' [SourcePath] '::' <identifier>
//
// SourcePath:
//      | [SourcePath '::'] <identifier>
//
// The SourcePath is a sequence of identifiers separated by the path '::' token.
//
//>==----------------------------------------------------------------------==<//

ImportDecl::ImportDecl(const SourcePath &path, bool local, const Span &span)
    : Decl(span), path(path), local(local) {}

//>==- PackageUnitDecl Implementation --------------------------------------==<//
//
// A PackageUnitDecl represents a source file in a local source tree.
// Declarations explicitly defined in the package are considered owned by the
// package, that is, they are contained in its respective DeclContext.
//
// Such declarations are also held as borrowed in the package itself. These
// declarations are not all explicitly owned by the package, as they may be
// imported declarations. Compiler passes operate on these declarations.
//
//>==----------------------------------------------------------------------==<//

PackageUnitDecl::PackageUnitDecl(const std::string &id, DeclContext *ctx, 
                                 Scope *scope, 
                                 std::vector<std::unique_ptr<ImportDecl>> imports)
    : DeclBase(), identifier(id), path(SourcePath(this->identifier, nullptr)),
    ctx(ctx), scope(scope) {
  for (Decl *decl : this->ctx->getDeclarations())
    this->decls.push_back(decl);
}

PackageUnitDecl::~PackageUnitDecl() {
  delete this->ctx;
  delete this->scope;
}

std::vector<ImportDecl *> PackageUnitDecl::getImports() const {
  std::vector<ImportDecl *> imports = {};
  for (const std::unique_ptr<ImportDecl> &import : this->imports)
    imports.push_back(import.get());

  return imports;
}

void PackageUnitDecl::addDecl(std::unique_ptr<Decl> decl) 
{ this->addDecl(this->ctx->addDeclaration(std::move(decl))); }

void PackageUnitDecl::addDecl(Decl *decl) {
  if (NamedDecl *namedDecl = dynamic_cast<NamedDecl *>(decl))
    this->scope->addDecl(namedDecl);

  this->decls.push_back(decl); 
}
