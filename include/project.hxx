#pragma once

#include <config.hxx>
#include <maven.hxx>

#include <json/json.hxx>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace kompose
{
    struct Project;
    struct Module;

    struct Dependency
    {
        std::unordered_set<const Module *> Modules;
        std::unordered_set<MavenCoordinate> Maven;
    };

    struct SourceSet
    {
        const Module *Parent;

        std::string Name;
        std::filesystem::path Source;
        std::filesystem::path Build;

        Dependency Compile;
        Dependency Runtime;
    };

    struct ApplicationModuleData
    {
        std::string Main;
        std::unordered_set<const SourceSet *> Include;
    };

    struct LibraryModuleData
    {
        LibraryModulePackage Package;
        bool IncludeSources;
        std::unordered_set<const SourceSet *> Include;
    };

    struct Module
    {
        SourceSet &operator[](const std::string &name);
        const SourceSet &operator[](const std::string &name) const;

        [[nodiscard]] std::unordered_set<const SourceSet *> IncludeInCompile() const;

        const Project *Parent;

        ModuleType Type;

        std::string Name;
        std::filesystem::path Source;
        std::filesystem::path Build;

        std::unordered_map<std::string, SourceSet> SourceSets;

        std::variant<ApplicationModuleData, LibraryModuleData> Data;
    };

    struct Project
    {
        struct iterator
        {
            bool operator==(const iterator &other) const;

            iterator &operator++();

            const Module &operator*() const;

            std::unordered_map<std::string, Module>::const_iterator base;
        };

        Module &operator[](const std::string &name);
        const Module &operator[](const std::string &name) const;

        iterator find(const std::string &name) const;

        iterator begin() const;
        iterator end() const;

        std::string Name;
        std::filesystem::path Path;

        RepositoriesConfig Repositories;

        std::unordered_map<std::string, Module> Modules;
    };
} // namespace kompose

template<>
struct data::serializer<json::node, kompose::Project>
{
    static void to_data(
        json::node &node,
        const kompose::Project &value);
};

template<>
struct data::serializer<json::node, kompose::Module>
{
    static void to_data(
        json::node &node,
        const kompose::Module &value);
};

template<>
struct data::serializer<json::node, std::filesystem::path>
{
    static void to_data(
        json::node &node,
        const std::filesystem::path &value);
};

template<>
struct data::serializer<json::node, kompose::ModuleType>
{
    static void to_data(
        json::node &node,
        const kompose::ModuleType &value);
};

template<>
struct data::serializer<json::node, kompose::SourceSet>
{
    static void to_data(
        json::node &node,
        const kompose::SourceSet &value);
};

template<>
struct data::serializer<json::node, kompose::ApplicationModuleData>
{
    static void to_data(
        json::node &node,
        const kompose::ApplicationModuleData &value);
};

template<>
struct data::serializer<json::node, kompose::LibraryModuleData>
{
    static void to_data(
        json::node &node,
        const kompose::LibraryModuleData &value);
};

template<>
struct data::serializer<json::node, kompose::Dependency>
{
    static void to_data(
        json::node &node,
        const kompose::Dependency &value);
};

template<>
struct data::serializer<json::node, const kompose::SourceSet *>
{
    static void to_data(
        json::node &node,
        const kompose::SourceSet *value);
};

template<>
struct data::serializer<json::node, const kompose::Module *>
{
    static void to_data(
        json::node &node,
        const kompose::Module *value);
};

template<>
struct data::serializer<json::node, kompose::MavenCoordinate>
{
    static void to_data(
        json::node &node,
        const kompose::MavenCoordinate &value);
};

template<>
struct data::serializer<json::node, kompose::LibraryModulePackage>
{
    static void to_data(
        json::node &node,
        const kompose::LibraryModulePackage &value);
};

template<>
struct data::serializer<json::node, kompose::RepositoriesConfig>
{
    static void to_data(
        json::node &node,
        const kompose::RepositoriesConfig &value);
};
