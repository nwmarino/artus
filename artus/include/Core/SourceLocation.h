#ifndef ARTUS_CORE_SOURCELOCATION_H
#define ARTUS_CORE_SOURCELOCATION_H

#include <string>

using std::size_t;
using std::string;

namespace artus {

/// Represents a singular location in the source code.
/// Used for error reporting and span maintenance.
struct SourceLocation {
  string file;
  size_t line;
  size_t col;
};

} // namespace artus

#endif // ARTUS_CORE_SOURCELOCATION_H
