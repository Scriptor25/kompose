#pragma once

#include <json/json.hxx>
#include <toml/toml.hxx>

#include <data/serializer.hxx>

#include <toolkit/string.hxx>

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_set>

namespace kompose
{
    enum class MavenCoordinateType
    {
        Jar,
        SourcesJar,
        Pom,
    };

    struct MavenCoordinate
    {
        bool operator==(const MavenCoordinate &other) const
        {
            return Group == other.Group && Artifact == other.Artifact && Type == other.Type && Version == other.Version;
        }

        [[nodiscard]] std::filesystem::path Locate() const
        {
            const auto *home = getenv("HOME");

            auto path = std::filesystem::path(home) / ".m2" / "repository";

            for (const auto segments = toolkit::split(Group, '.');
                 const auto &segment : segments)
                path /= segment;

            path /= Artifact;
            path /= Version;

            std::string type;

            switch (Type)
            {
            case MavenCoordinateType::Jar:
                type = ".jar";
                break;
            case MavenCoordinateType::SourcesJar:
                type = "-sources.jar";
                break;
            case MavenCoordinateType::Pom:
                type = ".pom";
                break;
            }

            path /= std::format("{}-{}{}", Artifact, Version, type);

            return path;
        }

        std::string Group;
        std::string Artifact;
        MavenCoordinateType Type;
        std::string Version;
    };
}

template<>
struct std::hash<kompose::MavenCoordinate>
{
    [[nodiscard]] static std::size_t combine(const std::size_t a, const std::size_t b) noexcept
    {
        return a ^ (b << 1);
    }

    std::size_t operator()(const kompose::MavenCoordinate &value) const noexcept
    {
        const auto h1 = std::hash<std::string>()(value.Group);
        const auto h2 = std::hash<std::string>()(value.Artifact);
        const auto h3 = std::hash<kompose::MavenCoordinateType>()(value.Type);
        const auto h4 = std::hash<std::string>()(value.Version);

        return combine(h1, combine(h2, combine(h3, h4)));
    }
};

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
        std::unordered_set<MavenCoordinate> Maven;
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
struct data::serializer<kompose::MavenCoordinate>
{
    static bool from_data(
        const toml::node &node,
        kompose::MavenCoordinate &value);

    static void to_data(json::node &node, const kompose::MavenCoordinate &value);
};

template<>
struct data::serializer<kompose::MavenCoordinateType>
{
    static bool from_data(
        const toml::node &node,
        kompose::MavenCoordinateType &value);

    static void to_data(json::node &node, const kompose::MavenCoordinateType &value);
};

template<>
struct data::serializer<kompose::LibraryModulePackage>
{
    static bool from_data(
        const toml::node &node,
        kompose::LibraryModulePackage &value);

    static void to_data(json::node &node, const kompose::LibraryModulePackage &value);
};
