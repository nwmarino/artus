#ifndef ARTUS_CORE_UNITCACHE_H
#define ARTUS_CORE_UNITCACHE_H

#include <cassert>

#include "../AST/DeclBase.h"

using std::vector;

namespace artus {

/// A cache to store units for the compilation lifecycle. This class allows
/// easier management of the compilation units during passes.
class UnitCache {
  vector<std::unique_ptr<PackageUnitDecl>> units = {};

  /// The index of the active unit in `units`.
  unsigned idx = -1;

public:
  UnitCache() {}

  /// Adds a unit to the cache.
  void addUnit(std::unique_ptr<PackageUnitDecl> unit) { 
    units.push_back(std::move(unit)); 
  }

  /// Returns the units in the cache.
  PackageUnitDecl *getActive() const {
    assert(idx >= 0 && idx < units.size() && \
        "A package is not currently active.");
    return units[idx].get(); 
  }

  /// Mark the active unit as compiled, and set the next unit as active. Returns
  /// true if a new unit was set as active, false otherwise.
  bool nextUnit() { return ++idx < units.size(); }

  /// Returns all units in the cache.
  vector<PackageUnitDecl *> getUnits() {
    vector<PackageUnitDecl *> units = {};
    for (std::unique_ptr<PackageUnitDecl> &unit : this->units) {
      units.push_back(unit.get());
    }
    return units;
  }
};

} // namespace artus

#endif // ARTUS_CORE_UNITCACHE_H
