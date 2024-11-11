#ifndef ARTUS_CORE_DRIVER_H
#define ARTUS_CORE_DRIVER_H

#include <vector>

#include "llvm/IR/Module.h"
#include "llvm/Target/TargetMachine.h"

#include "Context.h"
#include "Input.h"

using std::vector;

namespace artus {

/// Logic driver for the compilation process.
class Driver {
  /// Flags used during compilation.
  const CompilerFlags flags;

  /// The target machine for the current host.
  llvm::TargetMachine *TM;

  /// Context for the compiler process.
  Context *ctx;

  /// Creates the target machine.
  void createTM();

  /// Emit an object or assembly file from a given module.
  int emitFile(llvm::Module *module);

public:
  Driver(const InputContainer &input);
  ~Driver() = default;

  /// Returns the compiler flags being used by this driver.
  const CompilerFlags getFlags() { return flags; }

  /// Returns the target machine being used by this driver.
  llvm::TargetMachine *getTM() { return TM; }
};

} // namespace artus

#endif // ARTUS_CORE_DRIVER_H
