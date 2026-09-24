#pragma once

#include <toml/toml.hxx>

#include <data/serializer.hxx>

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_set>
#include <json/json.hxx>

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

    struct DependencyConfig
    {
        std::unordered_set<std::string> Modules;
        std::unordered_set<std::string> Maven;
    };

    struct DependenciesConfig
    {
        DependencyConfig General;
        DependencyConfig Compile;
        DependencyConfig Runtime;
    };

    struct ProjectConfig
    {
        std::optional<std::string> Name;

        ProjectArtifactConfig Artifact;

        ModulesConfig Modules;

        RepositoriesConfig Repositories;
        DependenciesConfig Dependencies;
    };

    enum class ModuleType
    {
        Application,
        Library,
    };

    struct ApplicationModuleConfigData
    {
        std::string Main;
        std::unordered_set<std::string> Include;
    };

    enum class LibraryModulePackage
    {
        Jar,
    };

    struct LibraryModuleConfigData
    {
        LibraryModulePackage Package;
        bool IncludeSources;
        std::unordered_set<std::string> Include;
    };

    struct SourceSetConfig
    {
        DependenciesConfig Dependencies;
    };

    struct ModuleConfig
    {
        std::filesystem::path Root;

        ModuleType Type;
        std::optional<std::string> Name;

        ArtifactConfig Artifact;

        RepositoriesConfig Repositories;
        DependenciesConfig Dependencies;

        std::unordered_map<std::string, SourceSetConfig> SourceSets;

        std::variant<ApplicationModuleConfigData, LibraryModuleConfigData> Data;
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
struct data::serializer<kompose::SourceSetConfig>
{
    static bool from_data(
        const toml::node &node,
        kompose::SourceSetConfig &value);
};

template<>
struct data::serializer<kompose::DependencyConfig>
{
    static bool from_data(
        const toml::node &node,
        kompose::DependencyConfig &value);
};

template<>
struct data::serializer<kompose::LibraryModulePackage>
{
    static bool from_data(
        const toml::node &node,
        kompose::LibraryModulePackage &value);

    static void to_data(json::node &node, const kompose::LibraryModulePackage &value);
};
