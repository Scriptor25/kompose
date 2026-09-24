#pragma once

#include <config.hxx>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <json/json.hxx>

namespace kompose
{
    struct Project;
    struct Module;

    struct Dependency
    {
        std::unordered_set<const Module *> Modules;
        std::unordered_set<std::string> Maven;
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

        std::unordered_map<std::string, Module> Modules;
    };
} // namespace kompose

template<>
struct data::serializer<kompose::Project>
{
    static void to_data(json::node &node, const kompose::Project &value);
};

template<>
struct data::serializer<kompose::Module>
{
    static void to_data(json::node &node, const kompose::Module &value);
};

template<>
struct data::serializer<std::filesystem::path>
{
    static void to_data(json::node &node, const std::filesystem::path &value);
};

template<>
struct data::serializer<kompose::ModuleType>
{
    static void to_data(json::node &node, const kompose::ModuleType &value);
};

template<>
struct data::serializer<kompose::SourceSet>
{
    static void to_data(json::node &node, const kompose::SourceSet &value);
};

template<>
struct data::serializer<kompose::ApplicationModuleData>
{
    static void to_data(json::node &node, const kompose::ApplicationModuleData &value);
};

template<>
struct data::serializer<kompose::LibraryModuleData>
{
    static void to_data(json::node &node, const kompose::LibraryModuleData &value);
};

template<>
struct data::serializer<kompose::Dependency>
{
    static void to_data(json::node &node, const kompose::Dependency &value);
};

template<>
struct data::serializer<const kompose::SourceSet *>
{
    static void to_data(json::node &node, const kompose::SourceSet *value);
};

template<>
struct data::serializer<const kompose::Module *>
{
    static void to_data(json::node &node, const kompose::Module *value);
};
