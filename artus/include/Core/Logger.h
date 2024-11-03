#ifndef ARTUS_CORE_LOGGER_H
#define ARTUS_CORE_LOGGER_H

#include "../Core/SourceLocation.h"

using std::string;

namespace artus {

#define CLEAR "\033[0m"
#define RED "\033[31m"
#define YELLOW "\033[33m"
#define BOLD "\033[1m"

#define FATAL (BOLD RED "fatal: " CLEAR)
#define WARN (BOLD YELLOW "warn: " CLEAR)

/// Crash the compiler with a fatal error message.
[[noreturn]] void fatal(const string &msg);

/// Crash the compiler with a positional cause.
[[noreturn]] void fatal(const string &msg, const SourceLocation &loc);

/// Begin a possible error trace.
void warn(const string &msg);

} // namespace artus

#endif // ARTUS_CORE_LOGGER_H
