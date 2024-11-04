#include "../../include/Core/Context.h"

using std::string;

using namespace artus;

Context::Context(vector<SourceFile> files) : files(std::move(files)), eof(0) {
  types["bool"] = new BasicType(BasicType::BasicTypeKind::INT1);
  types["char"] = new BasicType(BasicType::BasicTypeKind::INT8);
  types["i32"] = new BasicType(BasicType::BasicTypeKind::INT32);
  types["i64"] = new BasicType(BasicType::BasicTypeKind::INT64);
  types["u8"] = new BasicType(BasicType::BasicTypeKind::UINT8);
  types["u32"] = new BasicType(BasicType::BasicTypeKind::UINT32);
  types["u64"] = new BasicType(BasicType::BasicTypeKind::UINT64);
  types["fp64"] = new BasicType(BasicType::BasicTypeKind::FP64);
}

bool Context::nextFile() {
  if (files.empty())
    return false;

  const SourceFile next_file = files.back();
  files.pop_back();

  lexer = std::make_unique<Lexer>(next_file.name, 
      next_file.BufferStart);
  eof = 0;
  return true;
}

const Type *Context::getType(const string &name) {
  if (types.find(name) != types.end())
    return types[name];

  return nullptr;
}
