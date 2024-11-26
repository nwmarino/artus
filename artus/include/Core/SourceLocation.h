//>==- SourceLocation.h ---------------------------------------------------==<//
//
// This header file defines two important structs for representing locations
// in the source code: SourceLocation and Span.
//
//==>----------------------------------------------------------------------==<//

#ifndef ARTUS_CORE_SOURCELOCATION_H
#define ARTUS_CORE_SOURCELOCATION_H

#include <string>

namespace artus {

/// Represents a singular location in the source code.
struct SourceLocation {
  std::string file;
  std::size_t line;
  std::size_t col;
};

/// Represents a span of text in the source code.
struct Span {
  /// The beginning point of the span.
  SourceLocation begin;

  /// The ending point of the span.
  SourceLocation end;
};

} // end namespace artus

#endif // ARTUS_CORE_SOURCELOCATION_H
