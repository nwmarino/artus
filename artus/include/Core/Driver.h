#ifndef ARTUS_CORE_DRIVER_H
#define ARTUS_CORE_DRIVER_H

#include "llvm/Target/TargetMachine.h"

namespace artus {


struct CompilerFlags {

};

/// Logic driver for the compilation process.
class Driver {
  /// The target machine for the current host.
  llvm::TargetMachine *TM;

public:
  Driver(const CompilerFlags &flags);
};

} // namespace artus

#endif // ARTUS_CORE_DRIVER_H
