//>==- SourcePath.h --------------------------------------------------------<//
//
// This header file declares the SourcePath struct, which is used to represent
// a source path. Source paths are used by import declarations to resolve the
// path of a target source file import.
//
//>------------------------------------------------------------------------<//

#ifndef ARTUS_CORE_SOURCEPATH_H
#define ARTUS_CORE_SOURCEPATH_H

#include <string>

namespace artus {

/// Represents a source path. Used by import declarations to resolve the path of
/// a target source file import.
struct SourcePath {
  /// The current path piece.
  std::string curr;

  /// The next part of the path.
  SourcePath *next;

  SourcePath(const std::string &curr, SourcePath *next) 
      : curr(curr), next(next) {}

  ~SourcePath() = default;

  SourcePath &operator=(const SourcePath &other) {
    curr = other.curr;
    next = other.next;
    return *this;
  }

  /// Compares two source paths for equality.
  bool compare(const SourcePath &other) const {
    if (curr != other.curr)
      return false;

    if (next && other.next)
      return next->compare(*other.next);

    return !next && !other.next;
  }

  /// \returns A string representation of the source path, divided by `_`.
  const std::string toString() const {
    if (!this->next)
      return this->curr;

    return this->next->toString() + '_' + this->curr;
  }
};

} // end namespace artus

#endif // ARTUS_CORE_SOURCEPATH_H
