//>==- Context.cpp --------------------------------------------------------==<//
//
// The following source implements a context interface for the compilation
// process. The most notable feature of the context is the type table, which
// holds all types declared in an active package.
//
// Not all types are treated equally. Basic types are added to the context by
// default, similarily with shallow pointers to basic types. Source-defined
// types like structs are added as they are parsed. Array types are dependent
// upon both an element type and size, and so are only added when needed.
// Function types are not added to the type table, as they are not first-class
// types in the language, nor may they be explicitly declared. If a type is not
// declared in the type table when it is needed, then it is provided as a
// reference. If that type is not declared by type checking, then it is deemed
// unresolved.
//
// During type resolution, it is assumed all types are absolute, that is, not a
// reference. Since function types are not stored in the type table, if they are
// not absolute, for example, returning a reference, then they are reconstructed
// from scratch.
//
//>==----------------------------------------------------------------------==<//

#include <utility>

#include "../../include/AST/DeclBase.h"
#include "../../include/Core/Context.h"
#include "../../include/Core/Logger.h"
#include "../../include/Core/PackageManager.h"

using namespace artus;

Context::Context(std::vector<SourceFile> files) : files(std::move(files)) {
  this->PM = std::make_unique<PackageManager>();

  // Add basic types to the context.
  this->bTyTable["bool"] = new BasicType(BasicType::BasicTypeKind::INT1);
  this->bTyTable["char"] = new BasicType(BasicType::BasicTypeKind::INT8);
  this->bTyTable["i32"] = new BasicType(BasicType::BasicTypeKind::INT32);
  this->bTyTable["i64"] = new BasicType(BasicType::BasicTypeKind::INT64);
  this->bTyTable["u8"] = new BasicType(BasicType::BasicTypeKind::UINT8);
  this->bTyTable["u32"] = new BasicType(BasicType::BasicTypeKind::UINT32);
  this->bTyTable["u64"] = new BasicType(BasicType::BasicTypeKind::UINT64);
  this->bTyTable["f64"] = new BasicType(BasicType::BasicTypeKind::FP64);
  this->bTyTable["str"] = new BasicType(BasicType::BasicTypeKind::STR);

  this->resetTypes();
}

Context::~Context() {
  for (auto &type : bTyTable)
    delete type.second;

  for (SourceFile &file : files)
    delete []file.BufferStart;
}

void Context::resetTypes() {
  // Clears all types. Defined types are owned by their declarations.
  this->tyTable.clear();
}

void Context::addDefinedType(const std::string &name, const Type *T,
                             const SourceLocation &loc) {
  if (tyTable.find(name) == tyTable.end() || !tyTable[name]->isAbsolute())
    tyTable[name] = T;
  else
    fatal("redefinition of type: " + name, loc);
}

bool Context::nextFile() {
  if (files.empty())
    return false;

  // Get the next source file.
  const SourceFile nextFile = files.back();
  files.pop_back();

  // Instantiate a new lexer process and assign the new active file data.
  lexer = std::make_unique<Lexer>(nextFile.name, 
      nextFile.BufferStart);
  eof = 0;
  active.BufferStart = nextFile.BufferStart;
  active.name = nextFile.name;
  active.path = nextFile.path;
  resetTypes();
  return true;
}

void Context::addPackage(std::unique_ptr<PackageUnitDecl> pkg) 
{ PM->addPackage(std::move(pkg)); }

PackageUnitDecl *Context::resolvePackage(const std::string &id, 
                                         const SourceLocation &loc) const {
  const std::string cutId = id.substr(0, id.find_last_of('.'));
  if (PackageUnitDecl *PUD = PM->getPackage(cutId))
    return PUD;

  fatal("unresolved package: " + id, loc);
}

const Type *Context::getType(const std::string &name) {
  // Check if the type already exists.
  if (bTyTable.find(name) != bTyTable.end())
    return this->bTyTable[name];
  else if (tyTable.find(name) != tyTable.end())
    return this->tyTable[name];

  // Handle array types, i.e. i64[3]
  if (name.find('[') != std::string::npos) {
    // Extract the element type and size.
    const std::size_t idx = name.find('[');
    const std::string elemType = name.substr(0, idx);
    const std::string size = name.substr(idx + 1, name.size() - idx - 2);

    // Resolve the element type.
    const Type *T = getType(elemType);
    assert(T && "invalid array element type");

    // Instantiate and return the new array type.
    const ArrayType *AT = new ArrayType(T, std::stoi(size));
    this->bTyTable[name] = AT;
    return AT;
  }

  // Handle pointers not defined in the type table.
  if (name[0] == '#') {
    const std::string nestedType = name.substr(1);
    this->bTyTable[name] = new PointerType(getType(nestedType));
    return this->bTyTable[name];
  }

  // Return a type reference since the type could not be found.
  this->tyTable[name] = new TypeRef(name);
  return this->tyTable[name];
}

void Context::printAST() {
  ASTPrinter printer;
  for (PackageUnitDecl *pkg : PM->getPackages())
    printer.visit(pkg);
}
