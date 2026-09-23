#pragma once

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

    enum class NodeType
    {
        Application,
        Library,
    };

    struct Node
    {
        const SourceSet &operator[](const std::string &name) const;

        NodeType Type;

        std::string Name;
        std::filesystem::path Src;
        std::filesystem::path Build;

        std::unordered_map<std::string, SourceSet> SourceSets;
    };

    struct ApplicationNode : Node
    {
        std::string Main;
    };

    struct LibraryNode : Node
    {
    };

    struct Graph
    {
        struct iterator
        {
            bool operator==(const iterator &other) const;

            iterator &operator++();

            const Node &operator*() const;

            std::unordered_map<std::string, std::unique_ptr<Node>>::const_iterator base;
        };

        const Node &operator[](const std::string &name) const;

        iterator find(const std::string &name) const;

        iterator begin() const;
        iterator end() const;

        std::string Name;
        std::filesystem::path Path;

        std::unordered_map<std::string, std::unique_ptr<Node>> Nodes;
    };
} // namespace kompose
