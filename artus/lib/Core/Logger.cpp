#include "../../include/Core/Logger.h"

using std::string;

using namespace artus;

[[noreturn]] void artus::fatal(const string &msg) {
  printf("artus: %s%s\n", FATAL, msg.c_str()); 
  exit(EXIT_FAILURE); 
}

[[noreturn]] void artus::fatal(const string &msg, const Span &sp) {
  printf("%s:%zu:%zu: %s%s\n", sp.file.c_str(), sp.line, sp.col, 
      FATAL, msg.c_str());
  exit(EXIT_FAILURE);
}

void artus::warn(const string &msg) {
  printf("artus: %s%s\n", WARN, msg.c_str());
}
