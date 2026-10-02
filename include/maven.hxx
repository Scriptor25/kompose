#pragma once

#include <xml/xml.hxx>

#include <data/serializer.hxx>

#include <toolkit/result.hxx>

#include <filesystem>

namespace http
{
    class client;
}

namespace kompose
{
    struct Project;

    struct MavenDocument;
    struct MavenDocumentPool;

    enum class MavenResolveScope
    {
        Compile,
        Runtime,
    };

    struct MavenCoordinate
    {
        bool operator==(const MavenCoordinate &other) const;

        [[nodiscard]] std::filesystem::path locate() const;

        [[nodiscard]] std::string get_filename(const std::string &extension) const;
        [[nodiscard]] std::string get_filename(const std::string &classifier, const std::string &extension) const;

        [[nodiscard]] toolkit::result<MavenDocument> resolve_pom(
            const Project &project,
            const http::client &client) const;

        std::string Group;
        std::string Artifact;
        std::string Version;
    };

    enum class MavenDependencyScope : uint8_t
    {
        Compile  = 1 << 0,
        Provided = 1 << 1,
        Runtime  = 1 << 2,
        Test     = 1 << 3,
        System   = 1 << 4,
        Import   = 1 << 5,
    };

    uint8_t operator|(uint8_t a, MavenDependencyScope b);
    uint8_t operator|(MavenDependencyScope a, uint8_t b);
    uint8_t operator|(MavenDependencyScope a, MavenDependencyScope b);

    uint8_t operator&(uint8_t a, MavenDependencyScope b);
    uint8_t operator&(MavenDependencyScope a, uint8_t b);
    uint8_t operator&(MavenDependencyScope a, MavenDependencyScope b);

    struct MavenDocumentDependencyExclusion
    {
        std::string Group;
        std::string Artifact;
    };

    struct MavenDocumentDependency
    {
        std::string Group;
        std::string Artifact;
        std::optional<std::string> Version;
        std::optional<std::string> Classifier;
        std::optional<MavenDependencyScope> Scope;
        std::optional<std::string> SystemPath;
        std::optional<bool> Optional;

        std::vector<MavenDocumentDependencyExclusion> Exclusions;
    };

    struct MavenDocumentParent
    {
        std::string Group;
        std::string Artifact;
        std::string Version;
    };

    struct MavenDocument
    {
        [[nodiscard]] toolkit::result<MavenCoordinate> GetCoordinate(const MavenDocumentPool &pool) const;

        [[nodiscard]] toolkit::result<std::string> GetProperty(
            const MavenDocumentPool &pool,
            const std::string &key) const;

        toolkit::result<const MavenDocument *> GetParent(const MavenDocumentPool &pool) const;

        [[nodiscard]] toolkit::result<std::unordered_set<const MavenDocument *>> GetDependencies(
            const MavenDocumentPool &pool,
            uint8_t scopes) const;
        [[nodiscard]] toolkit::result<std::unordered_set<const MavenDocument *>> GetDependencies(
            const MavenDocumentPool &pool,
            MavenDependencyScope scope) const;

        std::optional<std::string> Group;
        std::optional<std::string> Artifact;
        std::optional<std::string> Version;

        std::optional<std::string> Packaging;

        std::vector<std::string> Modules;

        std::optional<MavenDocumentParent> Parent;

        std::vector<MavenDocumentDependency> DependencyManagement;

        std::vector<MavenDocumentDependency> Dependencies;

        std::unordered_map<std::string, std::string> Properties;
    };

    struct MavenDocumentPool
    {
        [[nodiscard]] toolkit::result<MavenCoordinate> GetCoordinate(
            const MavenDocument &document,
            const MavenDocumentParent &parent) const;
        [[nodiscard]] toolkit::result<MavenCoordinate> GetCoordinate(
            const MavenDocument &document,
            const MavenDocumentDependency &dependency) const;

        [[nodiscard]] toolkit::result<const MavenDocument *> GetDocument(const MavenCoordinate &coordinate) const;

        std::vector<MavenDocument> Documents;
    };

    [[nodiscard]] toolkit::result<std::unordered_set<std::filesystem::path>> resolve(
        const Project &project,
        const http::client &client,
        MavenResolveScope scope,
        const std::unordered_set<MavenCoordinate> &coordinates);
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

namespace data
{
    template<>
    struct serializer<xml::node, kompose::MavenDocument>
    {
        static bool from_data(const xml::node &node, kompose::MavenDocument &value);
    };

    template<>
    struct serializer<xml::element, kompose::MavenDocument>
    {
        static bool from_data(const xml::element &element, kompose::MavenDocument &value);
    };

    template<>
    struct serializer<xml::element, kompose::MavenDocumentParent>
    {
        static bool from_data(const xml::element &element, kompose::MavenDocumentParent &value);
    };

    template<>
    struct serializer<xml::element, kompose::MavenDocumentDependency>
    {
        static bool from_data(const xml::element &element, kompose::MavenDocumentDependency &value);
    };

    template<>
    struct serializer<xml::element, kompose::MavenDocumentDependencyExclusion>
    {
        static bool from_data(const xml::element &element, kompose::MavenDocumentDependencyExclusion &value);
    };

    template<>
    struct serializer<xml::element, kompose::MavenDependencyScope>
    {
        static bool from_data(const xml::element &element, kompose::MavenDependencyScope &value);
    };
}
