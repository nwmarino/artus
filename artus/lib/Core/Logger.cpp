#include "../../include/Core/Logger.h"

using std::string;

using namespace artus;

static const string CLEAR = "\033[0m";
static const string RED = "\033[31m";
static const string YELLOW = "\033[33m";
static const string ORANGE = "\033[38;5;208m";
static const string BOLD = "\033[1m";

static const string FATAL = BOLD + RED + "fatal: " + CLEAR;
static const string WARN = BOLD + YELLOW + "warn: " + CLEAR;
static const string TRACE = BOLD + ORANGE + "trace: " + CLEAR;

[[noreturn]] void artus::fatal(const string &msg) {
  printf("artus: %s%s\n", FATAL.c_str(), msg.c_str()); 
  exit(EXIT_FAILURE); 
}

[[noreturn]] void artus::fatal(const string &msg, const SourceLocation &loc) {
  printf("%s:%zu:%zu: %s%s\n", loc.file.c_str(), loc.line, loc.col,
      FATAL.c_str(), msg.c_str());
  exit(EXIT_FAILURE);
}

void artus::warn(const string &msg) {
  printf("artus: %s%s\n", WARN.c_str(), msg.c_str());
}

void artus::warn(const string &msg, const SourceLocation &loc) {
  printf("%s:%zu:%zu: %s%s\n", loc.file.c_str(), loc.line, loc.col,
      WARN.c_str(), msg.c_str());
}

void artus::trace(const string &msg, const SourceLocation &loc) {
  printf("%s:%zu:%zu: %s%s\n", loc.file.c_str(), loc.line, loc.col,
      TRACE.c_str(), msg.c_str());
}

void artus::info(const string &msg) {
  printf("artus: %s\n", msg.c_str());
}
