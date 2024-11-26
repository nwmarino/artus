//>==- UnitCache.h --------------------------------------------------------<==//
//
// This header file defines the `UnitCache` class, which is used to store
// packages during the compilation lifetime.
//
//>------------------------------------------------------------------------<==//

#ifndef ARTUS_CORE_UNITCACHE_H
#define ARTUS_CORE_UNITCACHE_H

#include <cassert>
#include <memory>
#include <vector>

#include "../AST/DeclBase.h"

namespace artus {

/// A cache to store units for the compilation lifetime.
class UnitCache {
  /// The units contained in this cache.
  std::vector<std::unique_ptr<PackageUnitDecl>> units = {};

  /// The index of the active unit in `units`.
  std::size_t idx = -1;

public:
  UnitCache() = default;

  /// Adds the \p unit to this cache.
  void addUnit(std::unique_ptr<PackageUnitDecl> unit) 
  { units.push_back(std::move(unit)); }

  /// \returns The units in the cache.
  PackageUnitDecl *getActive() const {
    assert(idx >= 0 && idx < units.size() && \
        "A package is not currently active.");

    return units.at(idx).get(); 
  }

  /// Mark the active unit as compiled, and set the next unit as active. 
  /// \returns `true` if a new unit could be set as active.
  bool nextUnit() { return ++idx < units.size(); }

  /// \returns All units in the cache.
  std::vector<PackageUnitDecl *> getUnits() {
    std::vector<PackageUnitDecl *> units = {};
    for (std::unique_ptr<PackageUnitDecl> &unit : this->units)
      units.push_back(unit.get());

    return units;
  }
};

} // end namespace artus

#endif // ARTUS_CORE_UNITCACHE_H
