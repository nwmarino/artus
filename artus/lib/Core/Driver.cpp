//>==- Driver.cpp ---------------------------------------------------------==<//
//
// The following source implements a driver that takes over control flow for
// the compilation process from command line input.
//
//>==----------------------------------------------------------------------==<//

#include <cassert>
#include <string>
#include <system_error>
#include <unordered_map>

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/CodeGen/CommandFlags.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Host.h"

#include "../../include/AST/DeclBase.h"
#include "../../include/Codegen/Codegen.h"
#include "../../include/Core/Driver.h"
#include "../../include/Core/Logger.h"
#include "../../include/Core/PackageManager.h"
#include "../../include/Parse/Parser.h"
#include "../../include/Sema/ReferenceAnalysis.h"
#include "../../include/Sema/Sema.h"

using namespace artus;

void Driver::createTM() {
  llvm::Triple triple = llvm::Triple(llvm::sys::getDefaultTargetTriple());

  // Make basic options for the target machine.
  llvm::TargetOptions options;
  const std::string cpu = "generic";
  const std::string features = "";

  // Generate the target.
  std::string errata;
  const llvm::Target *target = llvm::TargetRegistry::lookupTarget(
    llvm::sys::getDefaultTargetTriple(), errata);
  
  if (!target)
    fatal(errata);

  this->TM = target->createTargetMachine(triple.getTriple(), cpu, 
      features, options, llvm::Reloc::PIC_);
}

int Driver::emitFile(llvm::Module *module) {
  /// Instantiate all LLVM passes over the IR.
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

  // Get the package name without any extension.
  const std::string pkgName = module->getSourceFileName().substr(0, \
      module->getSourceFileName().find_last_of('.'));

  // Create the output file name for this package dependent on emit flags.
  std::string outputFile = pkgName + ".o";
  if (flags.emitLLVM)
    outputFile = pkgName + ".ll";
  else if (flags.emitASM)
    outputFile = pkgName + ".s";

  /// Configure the output file.
  std::error_code errorCode;
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
  if (fileType == llvm::CodeGenFileType::AssemblyFile && flags.emitLLVM)
    CPM.add(llvm::createPrintModulePass(output->os()));
  else {
    if (TM->addPassesToEmitFile(CPM, output->os(), nullptr, fileType))
      fatal("no support for file type");
  }

  /// Debugging for the module.
  if (llvm::verifyModule(*module, &llvm::errs()) && flags.debug) {
    module->print(llvm::errs(), nullptr);
    fatal("bad codegen");
  }

  /// Run remaining passes on the module.
  MPM.run(*module, MAM);
  CPM.run(*module);
  output->keep();

  if (flags.compile)
    objectFiles.push_back(outputFile);

  return 1;
}

Driver::Driver(const InputContainer &input) : flags(input.flags) {
  ctx = new Context(input.files);

  // Create the target machine.
  createTM();
  if (!this->TM)
    fatal("could not resolve a target machine");

  const std::string instance = llvm::sys::getHostCPUName().str();

  // Parse the source program.
  Parser parser = Parser(this->ctx);
  parser.buildAST();

  std::string pkgCycle = ctx->PM->checkCyclicalImports();
  if (!pkgCycle.empty())
    fatal("cyclical imports detected in package: " + pkgCycle);

  // Run reference analysis on each AST.
  ReferenceAnalysis refAnalysis = ReferenceAnalysis(this->ctx);

  // Run semantic analysis on each AST.
  Sema sema = Sema(this->ctx);

  // Check that a single entry point exists.
  if (ctx->foundEntry < 1)
    fatal("no entry point found");
  else if (ctx->foundEntry != 1)
    fatal("multiple entry points found");

  if (flags.printAST)
    ctx->printAST();

  // Stop here if not lowering any further.
  if (flags.skipCGN)
    return;
  
  Codegen cgn = Codegen(ctx, instance, this->TM);
  if (!emitFile(cgn.getModule()))
    fatal("failed to emit file output");

  // Link the object files.
  if (flags.compile) {
    // Setup a shellout command to link the object files.
    std::string cmd = "clang -o " + input.target + ' ' + instance + ".o";

    // Run the command.
    if (system(cmd.c_str()))
      fatal("failed to link object files");
    
    // Remove the object files.
    for (const std::string &obj : objectFiles) {
      if (remove((instance + ".o").c_str()))
        warn("failed to remove object file: " + obj);
    }
  }
}

Driver::~Driver() {
  delete this->ctx;
  //delete this->TM;
}
