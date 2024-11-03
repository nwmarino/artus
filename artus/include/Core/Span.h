#ifndef ARTUS_CORE_SPAN_H
#define ARTUS_CORE_SPAN_H

#include <string>

using std::size_t;
using std::string;

namespace artus {

/// A span of text in the source code. Used for error reporting.
struct Span {
  /// The name of the file the span is in.
  string file;

  /// The start of the span.
  size_t line;
  size_t col;

  /// The end of the span.
  size_t line_nd;
  size_t col_nd;
};

} // namespace artus

#endif // ARTUS_CORE_SPAN_H
