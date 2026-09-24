#pragma once

#include <toml/toml.hxx>

#include <data/serializer.hxx>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>

namespace kompose
{
    struct ProjectArtifactConfig
    {
        std::optional<std::string> Group;
        std::optional<std::string> Version;
    };

    struct ArtifactConfig
    {
        std::optional<std::string> Group;
        std::optional<std::string> Name;
        std::optional<std::string> Version;
    };

    struct ModulesConfig
    {
        std::unordered_set<std::string> Include;
    };

    struct RepositoriesConfig
    {
        std::unordered_set<std::string> Maven;
    };

    struct DependenciesConfig
    {
        std::unordered_set<std::string> Modules;
        std::unordered_set<std::string> Maven;
    };

    struct ProjectConfig
    {
        std::optional<std::string> Name;

        ProjectArtifactConfig Artifact;
        ModulesConfig Modules;
        RepositoriesConfig Repositories;

        DependenciesConfig Dependencies;
        DependenciesConfig CompileDependencies;
        DependenciesConfig RuntimeDependencies;
    };

    enum class ModuleType
    {
        Application,
        Library,
    };

    struct ApplicationModuleData
    {
        std::string Main;
        std::unordered_set<std::string> Include;
    };

    enum class LibraryModulePackage
    {
        Jar,
    };

    struct LibraryModuleData
    {
        LibraryModulePackage Package;
        bool IncludeSources;
        std::unordered_set<std::string> Include;
    };

    struct ModuleConfig
    {
        std::filesystem::path Root;

        ModuleType Type;
        std::optional<std::string> Name;

        ArtifactConfig Artifact;

        RepositoriesConfig Repositories;

        DependenciesConfig Dependencies;
        DependenciesConfig CompileDependencies;
        DependenciesConfig RuntimeDependencies;

        std::variant<ApplicationModuleData, LibraryModuleData> Data;
    };
} // namespace kompose

template<>
struct data::serializer<kompose::ProjectConfig>
{
    static bool from_data(const toml::node &node, kompose::ProjectConfig &value);
};

template<>
struct data::serializer<kompose::ModuleConfig>
{
    static bool from_data(
        const toml::node &node,
        kompose::ModuleConfig &value);
};

template<>
struct data::serializer<kompose::DependenciesConfig>
{
    static bool from_data(
        const toml::node &node,
        kompose::DependenciesConfig &value);
};

template<>
struct data::serializer<kompose::LibraryModulePackage>
{
    static bool from_data(
        const toml::node &node,
        kompose::LibraryModulePackage &value);
};
