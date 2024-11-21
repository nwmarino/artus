#include "../../include/AST/Stmt.h"
#include "../../include/Core/Context.h"
#include "../../include/Sema/Type.h"

using std::string;

using namespace artus;

Context::Context(vector<SourceFile> files) : files(std::move(files)), eof(0) {
  this->cache = std::make_unique<UnitCache>();

  // Add basic types to the context.
  this->types["bool"] = new BasicType(BasicType::BasicTypeKind::INT1);
  this->types["char"] = new BasicType(BasicType::BasicTypeKind::INT8);
  this->types["i32"] = new BasicType(BasicType::BasicTypeKind::INT32);
  this->types["i64"] = new BasicType(BasicType::BasicTypeKind::INT64);
  this->types["u8"] = new BasicType(BasicType::BasicTypeKind::UINT8);
  this->types["u32"] = new BasicType(BasicType::BasicTypeKind::UINT32);
  this->types["u64"] = new BasicType(BasicType::BasicTypeKind::UINT64);
  this->types["fp64"] = new BasicType(BasicType::BasicTypeKind::FP64);
  this->types["#bool"] = new PointerType(this->types["bool"]);
  this->types["#char"] = new PointerType(this->types["char"]);
  this->types["#i32"] = new PointerType(this->types["i32"]);
  this->types["#i64"] = new PointerType(this->types["i64"]);
  this->types["#u8"] = new PointerType(this->types["u8"]);
  this->types["#u32"] = new PointerType(this->types["u32"]);
  this->types["#u64"] = new PointerType(this->types["u64"]);
  this->types["#fp64"] = new PointerType(this->types["fp64"]);
}

Context::~Context() {
  for (auto &type : types) {
    delete type.second;
  }

  for (SourceFile &file : files) {
    delete []file.BufferStart;
  }
}

bool Context::nextFile() {
  if (files.empty())
    return false;

  const SourceFile next_file = files.back();
  files.pop_back();

  lexer = std::make_unique<Lexer>(next_file.name, 
      next_file.BufferStart);
  eof = 0;
  active.BufferStart = next_file.BufferStart;
  active.name = next_file.name;
  active.path = next_file.path;
  return true;
}

void Context::addPackage(std::unique_ptr<PackageUnitDecl> pkg) {
  cache->addUnit(std::move(pkg));
}

const Type *Context::getType(const string &name) {
  if (types.find(name) != types.end())
    return types[name];

  // Handle ptr reference levels deeper than 1.
  if (name[0] == '#') {
    const string nestedType = name.substr(1);
    types[name] = new PointerType(getType(nestedType));
    return types[name];
  }

  return nullptr;
}

void Context::printAST() {
  ASTPrinter printer;
  for (PackageUnitDecl *pkg : cache->getUnits()) {
    printer.visit(pkg);
  }
}
