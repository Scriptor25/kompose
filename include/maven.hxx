#pragma once

#include <toolkit/result.hxx>

#include <filesystem>

namespace http
{
    class client;
}

namespace kompose
{
    struct Project;

    struct MavenDocument
    {
        std::string Group;
        std::string Artifact;
        std::string Version;

        const MavenDocument *Parent{};
        std::unordered_set<const MavenDocument *> Dependencies;
    };

    struct MavenDocumentPool
    {
        const MavenDocument *Begin{};

        std::vector<std::unique_ptr<MavenDocument>> Documents;
    };

    struct MavenCoordinate
    {
        bool operator==(const MavenCoordinate &other) const;

        [[nodiscard]] std::filesystem::path locate() const;

        [[nodiscard]] std::string get_filename(const std::string &extension) const;
        [[nodiscard]] std::string get_filename(const std::string &classifier, const std::string &extension) const;

        [[nodiscard]] toolkit::result<std::unordered_set<std::filesystem::path>> resolve(
            const Project &project,
            const http::client &client) const;

        [[nodiscard]] toolkit::result<> resolve_pom(
            MavenDocumentPool &pool,
            const Project &project,
            const http::client &client) const;

        std::string Group;
        std::string Artifact;
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
        const auto h3 = std::hash<std::string>()(value.Version);

        return combine(h1, combine(h2, h3));
    }
};
