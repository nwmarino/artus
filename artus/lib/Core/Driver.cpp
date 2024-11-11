#include <cassert>
#include <llvm/IR/LLVMContext.h>
#include <string>
#include <system_error>

#include <llvm/Analysis/AliasAnalysis.h>
#include "llvm/CodeGen/CommandFlags.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/LegacyPassManager.h"
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include <llvm/Support/CodeGen.h>
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Host.h"

#include "Driver.h"
#include "Context.h"
#include "Input.h"
#include "Logger.h"
#include "../../include/Codegen/Codegen.h"
#include "../../include/Parse/Parser.h"
#include "../../include/Sema/Sema.h"

using std::error_code;
using std::string;

using namespace artus;

void Driver::createTM() {
  llvm::Triple triple = llvm::Triple(llvm::sys::getDefaultTargetTriple());

  llvm::TargetOptions options;
  const string cpu = "generic";
  const string features = "";

  std::string errata;
  const llvm::Target *target = llvm::TargetRegistry::lookupTarget(
    llvm::sys::getDefaultTargetTriple(), errata);
  
  if (!target)
    fatal(errata);

  this->TM = target->createTargetMachine(triple.getTriple(), cpu, 
      features, options, llvm::Reloc::PIC_);
}

int Driver::emitFile(llvm::Module *module) {
  /// Instantiate all passes over the IR.
  llvm::PassBuilder passBuilder = llvm::PassBuilder(this->TM);

  llvm::LoopAnalysisManager LAM;
  llvm::FunctionAnalysisManager FAM;
  llvm::CGSCCAnalysisManager CAM;
  llvm::ModuleAnalysisManager MAM;

  /// Register each pass on the pass builder.
  FAM.registerPass([&] { 
    return passBuilder.buildDefaultAAPipeline(); 
  });

  passBuilder.registerModuleAnalyses(MAM);
  passBuilder.registerCGSCCAnalyses(CAM);
  passBuilder.registerFunctionAnalyses(FAM);
  passBuilder.registerLoopAnalyses(LAM);
  passBuilder.crossRegisterProxies(LAM, FAM, CAM, MAM);

  llvm::ModulePassManager MPM;

  /// Configure the output file extension dependent on compiler flags.
  llvm::CodeGenFileType fileType = flags.emitLLVM || flags.emitASM ? \
      llvm::CodeGenFileType::AssemblyFile : llvm::CodeGenFileType::ObjectFile;

  const std::string pkgName = module->getSourceFileName();
  string outputFile = pkgName + ".o";
  if (flags.emitLLVM) {
    outputFile = pkgName + ".ll";
  } else if (flags.emitASM) {
    outputFile = pkgName + ".s";
  }

  /// Configure the output file.
  error_code errorCode;
  llvm::sys::fs::OpenFlags openFlags = llvm::sys::fs::OF_None;
  if (fileType == llvm::CodeGenFileType::AssemblyFile)
    openFlags |= llvm::sys::fs::OF_Text;

  std::unique_ptr<llvm::ToolOutputFile>
      output = std::make_unique<llvm::ToolOutputFile>(
          outputFile, errorCode, openFlags);

  if (errorCode)
    fatal(errorCode.message());

  /// Configure the pass manager and add a print pass for emitting IR.
  llvm::legacy::PassManager CPM;
  if (fileType == llvm::CodeGenFileType::AssemblyFile && flags.emitLLVM) {
    CPM.add(llvm::createPrintModulePass(output->os()));
  } else {
    if (TM->addPassesToEmitFile(CPM, output->os(), nullptr, fileType))
      fatal("no support for file type");
  }

  /// Debugging for the module.
  module->print(llvm::errs(), nullptr);
  if (llvm::verifyModule(*module, &llvm::errs())) {
    fatal("bad codegen");
  }

  /// Run remaining passes on the module.
  MPM.run(*module, MAM);
  CPM.run(*module);

  if (flags.emitASM || flags.emitLLVM) 
    output->keep();

  return 1;
}

Driver::Driver(const InputContainer &input) : flags(input.flags) {
  createTM();
  assert(this->TM && "Target machine does not exist.");

  this->ctx = new Context(input.files);

  Parser parser = Parser(this->ctx);
  parser.buildAST();
  
  Sema sema = Sema(this->ctx);

  if (flags.printAST) {
    ctx->printAST();
  }
  
  while (ctx->cache->nextUnit()) {
    
    Codegen codegen = Codegen(ctx, this->TM);
    
    llvm::Module *module = codegen.getModule();
    
    if (!emitFile(module)) {
      fatal("Failed to emit file: " + \
          ctx->cache->getActive()->getIdentifier());
    }
  }
  
}
