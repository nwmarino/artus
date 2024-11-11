#include <cstdlib>
#include <fstream>

#include "llvm/Support/TargetSelect.h"

#include "Driver.h"
#include "Input.h"
#include "include/AST/ASTPrinter.h"
#include "include/Codegen/Codegen.h"
#include "include/Core/Context.h"
#include "include/Core/Logger.h"
#include "include/Lex/Lexer.h"

using std::ofstream;
using std::fstream;
using std::string;

using namespace artus;

[[noreturn]] void printHelp(bool basic = false) {
  printf("usage: artus [options] <input files>\n");
  if (basic == true) {
    exit(EXIT_SUCCESS);
  }

  printf("Options:\n");
  printf("  -d: Enable debug mode\n");
  printf("  -ll: Emit LLVM IR\n");
  printf("  -S: Emit assembly\n");
  printf("  --print-ast: Print the AST\n");
  exit(EXIT_FAILURE);
}

SourceFile static parseInputFile(const string &path) {
  fstream file(path);

  if (!file.is_open()) {
    fatal(string("file not found: ") + path);
  }

  const string name = path.substr(path.find_last_of('/') + 1);
  char *SrcBuffer;
  int len;
  
  file.seekg(0, std::ios::end);
  SrcBuffer = new char[file.tellg()];
  len = file.tellg();

  file.seekg(0, std::ios::beg);
  file.read(SrcBuffer, len);
  file.close();


  return { .name = name.substr(0, name.find_last_of('.')), 
      .path = path, .BufferStart = SrcBuffer };
}

InputContainer parseCommandArgs(int argc, char **argv) {
  CompilerFlags flags = CompilerFlags();
  vector<SourceFile> files;

  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "-h") == 0) {
      printHelp();
    }
    
    if (strcmp(argv[i], "-d") == 0)
      flags.debug = 1;
    else if (strcmp(argv[i], "-S") == 0)
      flags.emitASM = 1;
    else if (strcmp(argv[i], "-ll") == 0)
      flags.emitLLVM = 1;
    else if (strcmp(argv[i], "--print-ast") == 0)
      flags.printAST = 1;

    if (argv[i][0] != '-') {
      files.push_back(parseInputFile(argv[i]));
    }
  }

  if (files.empty()) {
    fatal("no input files");
  }

  return { .flags = flags, .files = files };
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printHelp(true);
  }

  InputContainer input = parseCommandArgs(argc, argv);

  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();

  Driver driver = Driver(input);

  return EXIT_SUCCESS;
}
