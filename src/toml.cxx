#include <config.hxx>

#include <memory>
#include <unordered_map>

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

    ok &= from_data_opt(dependencies, value.Dependencies);
    ok &= from_data_opt(dependencies["compile"], value.CompileDependencies);
    ok &= from_data_opt(dependencies["runtime"], value.RuntimeDependencies);

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

    auto ok = true;

    ok &= node["name"] >> value.Name;

    ok &= artifact["group"] >> value.Artifact.Group;
    ok &= artifact["name"] >> value.Artifact.Name;
    ok &= artifact["version"] >> value.Artifact.Version;

    ok &= from_data_opt(repositories["maven"], value.Repositories.Maven);

    ok &= from_data_opt(dependencies, value.Dependencies);
    ok &= from_data_opt(dependencies["compile"], value.CompileDependencies);
    ok &= from_data_opt(dependencies["runtime"], value.RuntimeDependencies);

    if (type == "application")
    {
        value.Type = kompose::ModuleType::Application;

        auto &application = node["application"];

        kompose::ApplicationModuleData data;

        ok &= application["main"] >> data.Main;
        ok &= from_data_opt(application["include"], data.Include, {});

        value.Data = std::move(data);
        return ok;
    }

    if (type == "library")
    {
        value.Type = kompose::ModuleType::Library;

        auto &library = node["library"];

        kompose::LibraryModuleData data;

        ok &= from_data_opt(library["package"], data.Package, kompose::LibraryModulePackage::Jar);
        ok &= from_data_opt(library["sources"], data.IncludeSources, false);
        ok &= from_data_opt(library["include"], data.Include, {});

        value.Data = std::move(data);
        return ok;
    }

    return false;
}

bool data::serializer<kompose::DependenciesConfig>::from_data(
    const toml::node &node,
    kompose::DependenciesConfig &value)
{
    if (!node.is<toml::table>())
        return false;

    auto ok = true;

    ok &= from_data_opt(node["modules"], value.Modules);
    ok &= from_data_opt(node["maven"], value.Maven);

    return ok;
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
