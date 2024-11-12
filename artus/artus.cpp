#include <cstdlib>
#include <fstream>

#include "llvm/Support/TargetSelect.h"

#include "include/Core/Driver.h"
#include "include/Core/Logger.h"

using std::ofstream;
using std::fstream;
using std::string;

using namespace artus;

/// Print the help message for the compiler.
[[noreturn]] void printHelp(bool basic = false) {
  printf("artus: usage: artus [options] <input files>\n");
  if (basic == true) {
    exit(EXIT_SUCCESS);
  }

  printf("Options:\n");
  printf("  -o: Specify the output file\n");
  printf("  -d: Enable debug mode\n");
  printf("  -nc: Skip code generation\n");
  printf("  -ll: Emit LLVM IR\n");
  printf("  -S: Emit assembly\n");
  printf("  --print-ast: Print the AST\n");
  exit(EXIT_FAILURE);
}

/// Parse an input file path into a SourceFile struct.
SourceFile static parseInputFile(const string &path) {
  fstream file(path);

  if (!file.is_open()) {
    fatal("file not found: " + path);
  }

  const string name = path.substr(path.find_last_of('/') + 1);
  char *SrcBuffer;
  int len;
  
  file.seekg(0, std::ios::end);
  len = file.tellg();
  SrcBuffer = new char[len + 1];

  file.seekg(0, std::ios::beg);
  file.read(SrcBuffer, len);
  file.close();

  return { .name = name, .path = path, .BufferStart = SrcBuffer };
}

/// Parse the command line arguments into a struct.
InputContainer parseCommandArgs(int argc, char **argv) {
  CompilerFlags flags = CompilerFlags();
  vector<SourceFile> files;
  string target = "main";

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
    else if (strcmp(argv[i], "-nc") == 0)
      flags.skipCGN = 1;
    else if (strcmp(argv[i], "--print-ast") == 0)
      flags.printAST = 1;
    else if (strcmp(argv[i], "-o") == 0) {
      if (i + 1 < argc) {
        target = argv[++i];
      } else {
        fatal("no output file specified");
      }
    } else if (argv[i][0] != '-') {
      files.push_back(parseInputFile(argv[i]));
    }
  }

  if (files.empty()) {
    fatal("no input files");
  }

  if (target.substr(0, target.find_last_of('.')) != target) {
    fatal("target output should not have an extension");
  }

  flags.compile = !flags.emitASM && !flags.emitLLVM;

  return { .flags = flags, .files = files, .target = target };
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
