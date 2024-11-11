#ifndef ARTUS_CORE_UNITCACHE_H
#define ARTUS_CORE_UNITCACHE_H

#include "../AST/DeclBase.h"

using std::vector;

namespace artus {

/// A cache to store units for the compilation lifecycle. This class allows
/// easier management of the compilation units during passes.
class UnitCache {
  vector<std::unique_ptr<PackageUnitDecl>> units = {};

  /// The current unit being compiled.
  PackageUnitDecl *active = nullptr;

  /// The index of the active unit in `units`.
  unsigned idx = -1;

  /// The compiled units.
  vector<std::unique_ptr<PackageUnitDecl>> compiled = {};

public:
  UnitCache() {}

  /// Adds a unit to the cache.
  void addUnit(std::unique_ptr<PackageUnitDecl> unit) { 
    units.push_back(std::move(unit)); 
  }

  /// Returns the units in the cache.
  const PackageUnitDecl *getActive() const { return active; }

  /// Mark the active unit as compiled, and set the next unit as active. Returns
  /// true if a new unit was set as active, false otherwise.
  bool nextUnit() {
    // Mark old active unit as compiled.
    std::unique_ptr<PackageUnitDecl> oldActive = std::move(units[idx++]);
    compiled.push_back(std::move(oldActive));

    // Set the next unit as active.
    active = idx < units.size() ? units[idx].get() : nullptr;
    return active != nullptr;
  }

  /// Returns all units in the cache.
  vector<PackageUnitDecl *> getAllUnits() {
    vector<PackageUnitDecl *> allUnits;
    allUnits.reserve(units.size() + compiled.size());

    allUnits.insert(allUnits.end(), getRawUnits().begin(), 
        getRawUnits().end());
    allUnits.insert(allUnits.end(), getCompiledUnits().begin(), 
        getCompiledUnits().end());

    return allUnits;
  }

  /// Returns all raw, uncompiled units in the cache.
  vector<PackageUnitDecl *> getRawUnits() {
    vector<PackageUnitDecl *> rawUnits;
    for (std::unique_ptr<PackageUnitDecl> &unit : units) {
      rawUnits.push_back(unit.get());
    }
    return rawUnits;
  }

  /// Returns all compiled units in the cache.
  vector<PackageUnitDecl *> getCompiledUnits() {
    vector<PackageUnitDecl *> compiledUnits;
    for (std::unique_ptr<PackageUnitDecl> &unit : compiled) {
      compiledUnits.push_back(unit.get());
    }
    return compiledUnits;
  }
};

} // namespace artus

#endif // ARTUS_CORE_UNITCACHE_H
