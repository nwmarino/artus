#include <cstdlib>
#include <fstream>

#include "llvm/CodeGen/CommandFlags.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/LegacyPassManager.h"
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/IR/PassManager.h>
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include <llvm/IR/Verifier.h>
#include <llvm/Support/CodeGen.h>
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Host.h"

#include "include/AST/ASTPrinter.h"
#include "include/Codegen/Codegen.h"
#include "include/Core/Context.h"
#include "include/Core/Logger.h"
#include "include/Lex/Lexer.h"
#include "include/Parse/Parser.h"
#include "include/Sema/Sema.h"

using std::ofstream;
using std::fstream;
using std::string;

using namespace artus;

/// Creates a target machine for the current host.
[[nodiscard]] llvm::TargetMachine *createTM() {
  llvm::Triple triple = llvm::Triple(llvm::sys::getDefaultTargetTriple());

  llvm::TargetOptions opts;
  std::string cpu_str = "generic";
  std::string feat_str = "";

  std::string err;
  const llvm::Target *target = llvm::TargetRegistry::lookupTarget(
      llvm::sys::getDefaultTargetTriple(), err);

  if (!target) {
    fatal(err);
  }

  return target->createTargetMachine(triple.getTriple(), cpu_str, 
      feat_str, opts, llvm::Reloc::PIC_);
}

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

  llvm::TargetMachine *TM = createTM();

  SourceFile src = { .name = "main.s", .path = path, .BufferStart = SrcBuffer };
  Context *ctx = new Context(vector({ src }), TM);

  Parser parser = Parser(ctx);
  parser.buildAST();
  
  Sema sema = Sema(ctx);
  
  ctx->printAST();

 
  delete ctx;
  delete SrcBuffer;
  return EXIT_SUCCESS;
}
