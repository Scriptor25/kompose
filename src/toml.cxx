#include <config.hxx>

#include <memory>
#include <unordered_map>
#include <toolkit/string.hxx>

bool data::serializer<kompose::ProjectConfig>::from_data(
    const toml::node &node,
    kompose::ProjectConfig &value)
{
    if (!node.is<toml::table>())
        return false;

    auto &artifact = node["artifact"];
    auto &modules = node["modules"];
    auto &repositories = node["repositories"];
    auto &dependencies = node["dependencies"];

    auto ok = true;

    ok &= node["name"] >> value.Name;

    ok &= artifact["group"] >> value.Artifact.Group;
    ok &= artifact["version"] >> value.Artifact.Version;

    ok &= from_data_opt(modules["include"], value.Modules.Include, {});

    ok &= from_data_opt(repositories["maven"], value.Repositories.Maven);

    ok &= from_data_opt(dependencies, value.Dependencies.General);
    ok &= from_data_opt(dependencies["compile"], value.Dependencies.Compile);
    ok &= from_data_opt(dependencies["runtime"], value.Dependencies.Runtime);

    return ok;
}

bool data::serializer<kompose::ModuleConfig>::from_data(
    const toml::node &node,
    kompose::ModuleConfig &value)
{
    if (!node.is<toml::table>())
        return false;

    std::string type;
    if (!(node["type"] >> type))
        return false;

    auto &artifact = node["artifact"];
    auto &repositories = node["repositories"];
    auto &dependencies = node["dependencies"];
    auto &sources = node["sources"];

    auto ok = true;

    ok &= node["name"] >> value.Name;

    ok &= artifact["group"] >> value.Artifact.Group;
    ok &= artifact["name"] >> value.Artifact.Name;
    ok &= artifact["version"] >> value.Artifact.Version;

    ok &= from_data_opt(repositories["maven"], value.Repositories.Maven);

    ok &= from_data_opt(dependencies, value.Dependencies.General);
    ok &= from_data_opt(dependencies["compile"], value.Dependencies.Compile);
    ok &= from_data_opt(dependencies["runtime"], value.Dependencies.Runtime);

    ok &= from_data_opt(sources, value.SourceSets);

    if (type == "application")
    {
        value.Type = kompose::ModuleType::Application;

        auto &application = node["application"];

        kompose::ApplicationModuleConfigData data;

        ok &= application["main"] >> data.Main;
        ok &= from_data_opt(application["include"], data.Include, {});

        value.Data = std::move(data);
        return ok;
    }

    if (type == "library")
    {
        value.Type = kompose::ModuleType::Library;

        auto &library = node["library"];

        kompose::LibraryModuleConfigData data;

        ok &= from_data_opt(library["package"], data.Package, kompose::LibraryModulePackage::Jar);
        ok &= from_data_opt(library["sources"], data.IncludeSources, false);
        ok &= from_data_opt(library["include"], data.Include, {});

        value.Data = std::move(data);
        return ok;
    }

    return false;
}

bool data::serializer<kompose::SourceSetConfig>::from_data(
    const toml::node &node,
    kompose::SourceSetConfig &value)
{
    if (!node)
        return true;

    if (!node.is<toml::table>())
        return false;

    auto &dependencies = node["dependencies"];

    auto ok = true;

    ok &= from_data_opt(dependencies, value.Dependencies.General);
    ok &= from_data_opt(dependencies["compile"], value.Dependencies.Compile);
    ok &= from_data_opt(dependencies["runtime"], value.Dependencies.Runtime);

    return ok;
}

bool data::serializer<kompose::DependencyConfig>::from_data(
    const toml::node &node,
    kompose::DependencyConfig &value)
{
    if (!node.is<toml::table>())
        return false;

    auto ok = true;

    ok &= from_data_opt(node["modules"], value.Modules);
    ok &= from_data_opt(node["maven"], value.Maven);

    return ok;
}

bool data::serializer<kompose::MavenCoordinate>::from_data(const toml::node &node, kompose::MavenCoordinate &value)
{
    if (std::string str; node >> str)
    {
        switch (
            auto segments = toolkit::split(str, ':');
            segments.size()
        )
        {
        case 3:
            value.Group = std::move(segments[0]);
            value.Artifact = std::move(segments[1]);
            value.Type = kompose::MavenCoordinateType::Jar;
            value.Version = std::move(segments[2]);
            break;
        case 4:
            value.Group = std::move(segments[0]);
            value.Artifact = std::move(segments[1]);
            if (!(toml::node(std::move(segments[2])) >> value.Type))
                return false;
            value.Version = std::move(segments[3]);
            break;
        default:
            return false;
        }

        return true;
    }

    if (!node.is<toml::table>())
        return false;

    auto ok = true;

    ok &= node["group"] >> value.Group;
    ok &= node["artifact"] >> value.Artifact;
    ok &= from_data_opt(node["type"], value.Type, kompose::MavenCoordinateType::Jar);
    ok &= node["version"] >> value.Version;

    return ok;
}

bool data::serializer<kompose::MavenCoordinateType>::from_data(
    const toml::node &node,
    kompose::MavenCoordinateType &value)
{
    static const std::unordered_map<std::string_view, kompose::MavenCoordinateType> map
    {
        { "jar", kompose::MavenCoordinateType::Jar },
        { "sources-jar", kompose::MavenCoordinateType::SourcesJar },
        { "pom", kompose::MavenCoordinateType::Pom },
    };

    if (std::string str; node >> str)
    {
        if (const auto it = map.find(str); it != map.end())
        {
            value = it->second;
            return true;
        }
    }

    return false;
}

bool data::serializer<kompose::LibraryModulePackage>::from_data(
    const toml::node &node,
    kompose::LibraryModulePackage &value)
{
    static const std::unordered_map<std::string, kompose::LibraryModulePackage> map
    {
        { "jar", kompose::LibraryModulePackage::Jar },
    };

    std::string key;
    if (!(node >> key))
        return false;

    if (const auto it = map.find(key); it != map.end())
    {
        value = it->second;
        return true;
    }

    return false;
}
