#include "include/AST/ASTPrinter.h"
#include "include/Core/Context.h"
#include "include/Core/Logger.h"
#include "include/Lex/Lexer.h"
#include "include/Parse/Parser.h"

#include <cstdlib>
#include <fstream>

using std::ofstream;
using std::fstream;
using std::string;

using namespace artus;

int main(int argc, char **argv) {
  const string path = "../sample/RetZero.s";
  fstream file(path);

  if (!file.is_open()) {
    fatal(string("file not found: ") + path);
  }

  char *SrcBuffer;
  int len;
  
  file.seekg(0, std::ios::end);
  SrcBuffer = new char[file.tellg()];
  len = file.tellg();

  // read the whole file to bufstart
  file.seekg(0, std::ios::beg);
  file.read(SrcBuffer, len);
  file.close();

  /*
  ofstream buf("dump.txt");
  buf << lexer.dump();
  buf.close();
  */

  SourceFile src = { .name = "main.s", .path = path, .BufferStart = SrcBuffer };
  Context *ctx = new Context(vector({ src }));

  Parser parser = Parser(ctx);
  parser.buildAST();

  ctx->printAST();

  /*
  parser.buildAST();

  ctx->printAST();
  */
 
  delete ctx;
  delete SrcBuffer;
  return EXIT_SUCCESS;
}
