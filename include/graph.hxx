#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace kompose
{
    struct Node;

    struct SourceSet
    {
        std::string Name;
        std::filesystem::path Src;
        std::filesystem::path Build;

        std::unordered_set<const Node*> ModuleDependencies;
        std::unordered_set<std::string> MavenDependencies;
    };

    struct Node
    {
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
        std::string Name;
        std::filesystem::path Path;

        std::vector<std::unique_ptr<Node>> Nodes;
    };
} // namespace kompose
