//>==- Logger.h -----------------------------------------------------------==<//
//
// This header files declares important logging functions used during the
// compilation process to log errors and other information.
//
//>------------------------------------------------------------------------<//

#ifndef ARTUS_CORE_LOGGER_H
#define ARTUS_CORE_LOGGER_H

#include <string>

#include "../Core/SourceLocation.h"

namespace artus {

/// Crash the compiler with an error \p msg.
[[noreturn]] void fatal(const std::string &msg);

/// Crash the compiler with an error \p msg and positional cause \p loc.
[[noreturn]] void fatal(const std::string &msg, const SourceLocation &loc);

/// Log a non-fatal error \p msg.
void warn(const std::string &msg);

/// Log a non-fatal error \p msg with a positional cause \p loc.
void warn(const std::string &msg, const SourceLocation &loc);

/// Begin a possible error trace with message \p msg and a positional cause 
/// \p loc.
void trace(const std::string &msg, const SourceLocation &loc);

/// Displays \p msg in a neutral manner.
void info(const std::string &msg);

} // end namespace artus

#endif // ARTUS_CORE_LOGGER_H
