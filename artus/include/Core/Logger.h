#ifndef ARTUS_CORE_LOGGER_H
#define ARTUS_CORE_LOGGER_H

#include "../Core/SourceLocation.h"

using std::string;

namespace artus {

/// Crash the compiler with a fatal error message.
[[noreturn]] void fatal(const string &msg);

/// Crash the compiler with a positional cause.
[[noreturn]] void fatal(const string &msg, const SourceLocation &loc);

/// Warn the user of potentially bad input. This function is not marked
/// `[[noreturn]]` as it may not signify a fatal error.
void warn(const string &msg);

/// Warn the user of potentially bad input with a positional cause. This 
/// function is not marked `[[noreturn]]` as it may not signify a fatal error.
void warn(const string &msg, const SourceLocation &loc);

/// Begin a possible error trace with a positional cause. This function is not
/// marked `[[noreturn]]` as certain phases of compilation may recover.
void trace(const string &msg, const SourceLocation &loc);

/// Display a neutral info message.
void info(const string &msg);

} // namespace artus

#endif // ARTUS_CORE_LOGGER_H
