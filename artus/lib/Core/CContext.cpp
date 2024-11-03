#include <utility>

#include "../../include/Core/CContext.h"
#include "../../include/Sema/Type.h"

using std::string;
using std::vector;

using namespace artus;

CContext::CContext(vector<SrcFile> &files) : files(std::move(files)), 
    previousToken(Token()), currentToken(Token()), eof(0) {
  InitTypes();
  InitLexer();
}

void CContext::InitTypes() const {
  types["bool"] = new BasicType(BasicType::BasicTypeKind::INT1);
  types["char"] = new BasicType(BasicType::BasicTypeKind::INT8);
  types["i32"] = new BasicType(BasicType::BasicTypeKind::INT32);
  types["i64"] = new BasicType(BasicType::BasicTypeKind::INT64);
  types["u8"] = new BasicType(BasicType::BasicTypeKind::UINT8);
  types["u32"] = new BasicType(BasicType::BasicTypeKind::UINT32);
  types["u64"] = new BasicType(BasicType::BasicTypeKind::UINT64);
  types["fp64"] = new BasicType(BasicType::BasicTypeKind::FP64);
}

void CContext::InitLexer() {
  if (files.empty())
    return;

  const SrcFile file = files.back();
  files.pop_back();

  lexer = new Lexer(file.name, file.BufferStart);
}

const Type *CContext::getType(const string &name) const {
  if (types.find(name) != types.end())
    return types[name];

  return nullptr;
}

const Token &CContext::next() {
  previousToken = currentToken;
  if (lexer->Lex(currentToken))
    return currentToken;

  eof = 1;
  return currentToken;
}
