#include "include/Core/Logger.h"
#include "include/Lex/Lexer.h"

#include <cstdlib>
#include <fstream>
#include <fstream>

using std::ofstream;

using namespace artus;

int main(int argc, char **argv) {
  std::fstream file("./sample/main.s");

  if (!file.is_open()) {
    fatal(std::string("file not found: ") + "./sample/main.s");
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

  Lexer lexer("main.s", SrcBuffer);

  ofstream buf("dump.txt");
  buf << lexer.dump();
  buf.close();

  free(SrcBuffer);
  return EXIT_SUCCESS;
}
