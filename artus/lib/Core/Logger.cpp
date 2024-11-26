//>==- Logger.cpp ----------------------------------------------------------==<//
//
// The following source implements the logging functions declared in Logger.h.
//
//>==----------------------------------------------------------------------==<//

#include "../../include/Core/Logger.h"

using namespace artus;

static const std::string CLEAR = "\033[0m";
static const std::string RED = "\033[31m";
static const std::string YELLOW = "\033[33m";
static const std::string ORANGE = "\033[38;5;208m";
static const std::string BOLD = "\033[1m";

static const std::string FATAL = BOLD + RED + "fatal: " + CLEAR;
static const std::string WARN = BOLD + YELLOW + "warn: " + CLEAR;
static const std::string TRACE = BOLD + ORANGE + "trace: " + CLEAR;

[[noreturn]] void artus::fatal(const std::string &msg) {
  printf("artus: %s%s\n", FATAL.c_str(), msg.c_str()); 
  exit(EXIT_FAILURE); 
}

[[noreturn]] void artus::fatal(const std::string &msg, const SourceLocation &loc) {
  printf("%s:%zu:%zu: %s%s\n", loc.file.c_str(), loc.line, loc.col,
      FATAL.c_str(), msg.c_str());
  exit(EXIT_FAILURE);
}

void artus::warn(const std::string &msg) 
{ printf("artus: %s%s\n", WARN.c_str(), msg.c_str()); }

void artus::warn(const std::string &msg, const SourceLocation &loc) {
  printf("%s:%zu:%zu: %s%s\n", loc.file.c_str(), loc.line, loc.col,
      WARN.c_str(), msg.c_str());
}

void artus::trace(const std::string &msg, const SourceLocation &loc) {
  printf("%s:%zu:%zu: %s%s\n", loc.file.c_str(), loc.line, loc.col,
      TRACE.c_str(), msg.c_str());
}

void artus::info(const std::string &msg) 
{ printf("artus: %s\n", msg.c_str()); }
