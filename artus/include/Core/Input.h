//>==- Input.h ------------------------------------------------------------==<//
//
// This header file defines important structs to organize input to the compiler
// on execution.
//
//>==----------------------------------------------------------------------==<//

#ifndef ARTUS_CORE_INPUT_H
#define ARTUS_CORE_INPUT_H

#include <string>
#include <vector>

namespace artus {

/// Represents an input source file to the compiler.
struct SourceFile {
  /// Name of the source file.
  std::string name;

  /// Full path to the source file.
  std::string path;

  /// Pointer to the file source code.
  const char *BufferStart;
};

/// Represents the possible compilation flags given at execution.
struct CompilerFlags {
  /// To include all debug logging.
  unsigned int debug : 1;

  /// To skip code generation.
  unsigned int skipCGN : 1;

  /// To emit LLVM IR.
  unsigned int emitLLVM : 1;

  /// To emit assembly code.
  unsigned int emitASM : 1;

  /// To compile to an executable.
  unsigned int compile : 1;

  /// To print the AST post-sema.
  unsigned int printAST : 1;

  /// To print the time compilation took.
  unsigned int printTime : 1;
};

/// Wrapper object for compiler flags and input.
struct InputContainer {
  /// Flags used during compilation.
  const CompilerFlags flags;

  /// Input source files to compile.
  const std::vector<SourceFile> files;

  /// The name of the executable to compile to.
  const std::string target;
};

} // end namespace artus

#endif // ARTUS_CORE_INPUT_H
