#ifndef ARTUS_CORE_LOGGER_H
#define ARTUS_CORE_LOGGER_H

#include "../Core/SourceLocation.h"

using std::string;

namespace artus {

#define CLEAR "\033[0m"
#define RED "\033[31m"
#define YELLOW "\033[33m"
#define ORANGE "\033[38;5;208m"
#define BOLD "\033[1m"

#define FATAL (BOLD RED "fatal: " CLEAR)
#define WARN (BOLD YELLOW "warn: " CLEAR)
#define TRACE (BOLD ORANGE "trace: " CLEAR)

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

} // namespace artus

#endif // ARTUS_CORE_LOGGER_H
