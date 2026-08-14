#pragma once

#include <data/serializer.hxx>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <toml/toml.hxx>
#include <unordered_set>

namespace kompose {

struct ProjectArtifactConfig {
  std::optional<std::string> Group;
  std::optional<std::string> Version;
};

struct ArtifactConfig {
  std::optional<std::string> Group;
  std::optional<std::string> Name;
  std::optional<std::string> Version;
};

struct ModulesConfig {
  std::unordered_set<std::string> Include;
};

struct RepositoriesConfig {
  std::unordered_set<std::string> Maven;
};

struct DependenciesConfig {
  std::unordered_set<std::string> Modules;
  std::unordered_set<std::string> Maven;
};

struct ProjectConfig {
  std::optional<std::string> Name;

  ProjectArtifactConfig Artifact;
  ModulesConfig Modules;
  RepositoriesConfig Repositories;

  DependenciesConfig Dependencies;
  DependenciesConfig CompileDependencies;
  DependenciesConfig RuntimeDependencies;
  DependenciesConfig TestDependencies;
};

enum class ModuleType {
  Application,
  Library,
};

struct ModuleConfig {
  std::filesystem::path Root;

  ModuleType Type;
  std::optional<std::string> Name;

  ArtifactConfig Artifact;

  RepositoriesConfig Repositories;

  DependenciesConfig Dependencies;
  DependenciesConfig CompileDependencies;
  DependenciesConfig RuntimeDependencies;
  DependenciesConfig TestDependencies;
};

struct ApplicationModuleConfig : ModuleConfig {
  std::string Main;
};

enum class LibraryModulePackage {
  Jar,
};

struct LibraryModuleConfig : ModuleConfig {
  LibraryModulePackage Package;
  bool Sources;
};

} // namespace kompose

template <> struct data::serializer<kompose::ProjectConfig> {
  static bool from_data(const toml::node &node, kompose::ProjectConfig &value);
};

template <> struct data::serializer<std::unique_ptr<kompose::ModuleConfig>> {
  static bool from_data(const toml::node &node,
                        std::unique_ptr<kompose::ModuleConfig> &value);
};

template <> struct data::serializer<kompose::DependenciesConfig> {
  static bool from_data(const toml::node &node,
                        kompose::DependenciesConfig &value);
};

template <> struct data::serializer<kompose::LibraryModulePackage> {
  static bool from_data(const toml::node &node,
                        kompose::LibraryModulePackage &value);
};
