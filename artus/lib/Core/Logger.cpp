#include "../../include/Core/Logger.h"

using std::string;

using namespace artus;

[[noreturn]] void artus::fatal(const string &msg) {
  printf("artus: %s%s\n", FATAL, msg.c_str()); 
  exit(EXIT_FAILURE); 
}

[[noreturn]] void artus::fatal(const string &msg, const SourceLocation &loc) {
  printf("%s:%zu:%zu: %s%s\n", loc.file.c_str(), loc.line, loc.col,
      FATAL, msg.c_str());
  exit(EXIT_FAILURE);
}

void artus::warn(const string &msg) {
  printf("artus: %s%s\n", WARN, msg.c_str());
}

void artus::warn(const string &msg, const SourceLocation &loc) {
  printf("%s:%zu:%zu: %s%s\n", loc.file.c_str(), loc.line, loc.col,
      WARN, msg.c_str());
}

void artus::trace(const string &msg, const SourceLocation &loc) {
  printf("%s:%zu:%zu: %s%s\n", loc.file.c_str(), loc.line, loc.col,
      TRACE, msg.c_str());
}
