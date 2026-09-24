#pragma once

#include <config.hxx>

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace kompose
{
    struct Node;

    struct SourceSet
    {
        std::string Name;
        std::filesystem::path Src;
        std::filesystem::path Build;

        std::unordered_set<const Node *> ModuleDependencies;
        std::unordered_set<std::string> MavenDependencies;
    };

    struct ApplicationData
    {
        std::string Main;
        std::unordered_set<const SourceSet *> IncludeSourceSets;
    };

    struct LibraryData
    {
        LibraryModulePackage Package;
        bool IncludeSources;
        std::unordered_set<const SourceSet *> IncludeSourceSets;
    };

    struct Node
    {
        const SourceSet &operator[](const std::string &name) const;

        ModuleType Type;

        std::string Name;
        std::filesystem::path Src;
        std::filesystem::path Build;

        std::unordered_map<std::string, SourceSet> SourceSets;

        std::variant<ApplicationData, LibraryData> Data;
    };

    struct Graph
    {
        struct iterator
        {
            bool operator==(const iterator &other) const;

            iterator &operator++();

            const Node &operator*() const;

            std::unordered_map<std::string, Node>::const_iterator base;
        };

        const Node &operator[](const std::string &name) const;

        iterator find(const std::string &name) const;

        iterator begin() const;
        iterator end() const;

        std::string Name;
        std::filesystem::path Path;

        std::unordered_map<std::string, Node> Nodes;
    };
} // namespace kompose
