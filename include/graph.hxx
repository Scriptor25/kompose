#pragma once

#include <memory>
#include <vector>

namespace kompose {

struct ModuleNode {
  std::string Name;
};

struct ApplicationModuleNode : ModuleNode {};

struct LibraryModuleNode : ModuleNode {};

struct ProjectGraph {
  std::string Name;

  std::vector<std::shared_ptr<ModuleNode>> Nodes;
};

} // namespace kompose
