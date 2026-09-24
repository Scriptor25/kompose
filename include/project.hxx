#pragma once

#include <config.hxx>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace kompose
{
    struct Module;

    struct Dependency
    {
        std::unordered_set<const Module *> Modules;
        std::unordered_set<std::string> Maven;
    };

    struct SourceSet
    {
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

        const Module &operator[](const std::string &name) const;

        iterator find(const std::string &name) const;

        iterator begin() const;
        iterator end() const;

        std::string Name;
        std::filesystem::path Path;

        std::unordered_map<std::string, Module> Modules;
    };
} // namespace kompose
