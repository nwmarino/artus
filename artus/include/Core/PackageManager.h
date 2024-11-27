//>==- PackageManager.h ---------------------------------------------------==<//
//
// This header declares an important class in managing package declarations
// and their dependencies.
//
//>==----------------------------------------------------------------------==<//

#ifndef ARTUS_CORE_PACKAGEMANAGER_H
#define ARTUS_CORE_PACKAGEMANAGER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "../AST/DeclBase.h"
#include "../Core/SourcePath.h"

namespace artus {

class PackageManager final {
/// A map of owned packages by their identifier.
std::unordered_map<std::string, std::unique_ptr<PackageUnitDecl>> packages;

/// Check the package with identifier \p id for cyclical imports.
bool hasCycleUtil(const std::string &id, 
                  std::unordered_set<std::string> &visited,
                  std::unordered_set<std::string> &stack) const {
  if (!visited.count(id)) {
    visited.insert(id);
    stack.insert(id);

    auto it = packages.find(id);
    if (it != packages.end()) {
      for (ImportDecl *import : it->second->getImports()) {
        const std::string &importId = import->getPath().toString();
        if (!visited.count(importId) && hasCycleUtil(importId, visited, stack))
          return true;
        else if (stack.count(importId))
          return true;
      }
    }
  }

  stack.erase(id);
  return false;
}

public:
  PackageManager() = default;
  ~PackageManager() = default;

  void addPackage(std::unique_ptr<PackageUnitDecl> package) 
  { packages[package->getIdentifier()] = std::move(package); }

  PackageUnitDecl *getPackage(const std::string &id) const {
    auto it = packages.find(id);
    return it != packages.end() ? it->second.get() : nullptr;
  }

  std::vector<PackageUnitDecl *> getPackages() const {
    std::vector<PackageUnitDecl *> pkgs = {};
    for (const auto &pkg : packages)
      pkgs.push_back(pkg.second.get());

    return pkgs;
  }

  const std::string checkCyclicalImports() const {
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> stack;

    for (const auto &pkgKV : packages) {
      // For each package, check if it has a cycle.
      if (hasCycleUtil(pkgKV.second->getIdentifier(), visited, stack))
        return pkgKV.second->getIdentifier();
    }

    return "";
  }

};

} // end namespace artus

#endif // ARTUS_CORE_PACKAGEMANAGER_H
