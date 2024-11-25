#ifndef ARTUS_CORE_SOURCEPATH_H
#define ARTUS_CORE_SOURCEPATH_H

#include <string>

using std::string;

namespace artus {

/// Represents a source path. Used by import declarations to resolve the path of
/// a target source file import.
struct SourcePath {
  /// The current path piece.
  string curr;

  /// The next part of the path.
  SourcePath *next;

  SourcePath(const string &curr, SourcePath *next) : curr(curr), next(next) {}

  ~SourcePath() {
    /*
    SourcePath *currPath = next;
    while (currPath) {
      SourcePath *tmp = currPath->next;
      delete currPath;
      currPath = tmp;
    }
    */
    delete next;
  }

  SourcePath &operator=(const SourcePath &other) {
    curr = other.curr;
    next = other.next;
    return *this;
  }

  /// Compares two source paths for equality.
  bool compare(const SourcePath &other) const {
    if (curr != other.curr) {
      return false;
    }

    if (next && other.next) {
      return next->compare(*other.next);
    }

    return !next && !other.next;
  }
};

} // namespace artus

#endif // ARTUS_CORE_SOURCEPATH_H
