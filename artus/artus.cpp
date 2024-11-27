//>==- artus.cpp ----------------------------------------------------------==<//
//
// The following source contains the entry point `main` to the compiler. The
// function hands off control flow to the driver to manage the compilation
// process and the lifespan of necessary contexts and passes.
//
//>==--------------------------------------------------------------------==<//

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#include "llvm/Support/TargetSelect.h"

#include "include/Core/Driver.h"
#include "include/Core/Logger.h"

using namespace artus;

/// Print the help message for the compiler, with options if \p basic.
[[noreturn]] void printHelp(bool basic = false) {
  printf("artus: usage: artus [options] <input files>\n");
  if (basic)
    exit(EXIT_SUCCESS);

  printf("Options:\n");
  printf("  -o: Specify the output file\n");
  printf("  -d: Enable debug mode\n");
  printf("  -t: Print compilation time\n");
  printf("  -nc: Skip code generation\n");
  printf("  -ll: Emit LLVM IR\n");
  printf("  -S: Emit assembly\n");
  printf("  --print-ast: Print the AST\n");

  exit(EXIT_FAILURE);
}

/// \returns A SourceFile struct containing data about the input file \p path.
SourceFile static parseInputFile(const std::string &path) {
  std::fstream file(path);
  if (!file.is_open())
    fatal("file not found: " + path);

  const std::string name = path.substr(path.find_last_of('/') + 1);
  char *SrcBuffer;
  int len;
  
  // Determine the source length.
  file.seekg(0, std::ios::end);
  len = file.tellg();
  file.seekg(0, std::ios::beg);

  // Instantiate a source buffer.
  SrcBuffer = new char[len + 1];
  file.read(SrcBuffer, len);
  SrcBuffer[len] = '\0';

  file.close();
  return { .name=name, .path=path, .BufferStart=SrcBuffer };
}

/// \returns A wrapper object containing flags and input source files parsed
/// from the command line input \p argv.
InputContainer parseCommandArgs(int argc, char **argv) {
  CompilerFlags flags = CompilerFlags();
  std::vector<SourceFile> files;
  std::string target = "main";

  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "-h") == 0)
      printHelp();
    else if (strcmp(argv[i], "-t") == 0)
      flags.printTime = 1;
    else if (strcmp(argv[i], "-d") == 0)
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
      if (i + 1 < argc)
        target = argv[++i];
      else
        fatal("no output file specified");
    } else if (argv[i][0] != '-')
      files.push_back(parseInputFile(argv[i]));
  }

  if (files.empty())
    fatal("no input files");

  if (target.substr(0, target.find_last_of('.')) != target)
    fatal("target output should not have an extension");

  flags.compile = !flags.emitASM && !flags.emitLLVM;

  return { .flags=flags, .files=files, .target=target };
}

int main(int argc, char **argv) {
  if (argc < 2)
    printHelp(true);

  // Start timer for compilation.
  std::chrono::time_point start = std::chrono::high_resolution_clock::now();

  InputContainer input = parseCommandArgs(argc, argv);

  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();

  Driver driver = Driver(input);

  // Stop timer for compilation, and calculate duration.
  std::chrono::time_point stop = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      stop - start
  );

  if (input.flags.printTime)
    printf("artus: compilation took: %ldms\n", duration.count());

  return EXIT_SUCCESS;
}
