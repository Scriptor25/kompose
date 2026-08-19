#include <config.hxx>
#include <memory>
#include <unordered_map>

bool data::serializer<kompose::ProjectConfig>::from_data(
    const toml::node& node, kompose::ProjectConfig& value)
{
    if (!node.is<toml::table>())
        return false;

    auto& artifact = node["artifact"];
    auto& modules = node["modules"];
    auto& repositories = node["repositories"];
    auto& dependencies = node["dependencies"];

    auto ok = true;

    ok &= node["name"] >> value.Name;

    ok &= artifact["group"] >> value.Artifact.Group;
    ok &= artifact["version"] >> value.Artifact.Version;

    ok &= from_data_opt(modules["include"], value.Modules.Include);

    ok &= from_data_opt(repositories["maven"], value.Repositories.Maven);

    ok &= from_data_opt(dependencies, value.Dependencies);
    ok &= from_data_opt(dependencies["compile"], value.CompileDependencies);
    ok &= from_data_opt(dependencies["runtime"], value.RuntimeDependencies);
    ok &= from_data_opt(dependencies["test"], value.TestDependencies);

    return ok;
}

bool data::serializer<std::unique_ptr<kompose::ModuleConfig>>::from_data(
    const toml::node& node, std::unique_ptr<kompose::ModuleConfig>& value)
{
    if (!node.is<toml::table>())
        return false;

    std::string type;
    if (!(node["type"] >> type))
        return false;

    auto& artifact = node["artifact"];
    auto& repositories = node["repositories"];
    auto& dependencies = node["dependencies"];

    kompose::ModuleConfig config;

    auto ok = true;

    ok &= node["name"] >> config.Name;

    ok &= artifact["group"] >> config.Artifact.Group;
    ok &= artifact["name"] >> config.Artifact.Name;
    ok &= artifact["version"] >> config.Artifact.Version;

    ok &= from_data_opt(repositories["maven"], config.Repositories.Maven);

    ok &= from_data_opt(dependencies, config.Dependencies);
    ok &= from_data_opt(dependencies["compile"], config.CompileDependencies);
    ok &= from_data_opt(dependencies["runtime"], config.RuntimeDependencies);
    ok &= from_data_opt(dependencies["test"], config.TestDependencies);

    if (type == "application")
    {
        kompose::ApplicationModuleConfig application_config(config);
        application_config.Type = kompose::ModuleType::Application;

        auto& application = node["application"];

        ok &= application["main"] >> application_config.Main;

        value = std::make_unique<kompose::ApplicationModuleConfig>(
            std::move(application_config));
        return ok;
    }

    if (type == "library")
    {
        kompose::LibraryModuleConfig library_config(config);
        library_config.Type = kompose::ModuleType::Library;

        auto& library = node["library"];

        ok &= from_data_opt(library["package"], library_config.Package,
                            kompose::LibraryModulePackage::Jar);
        ok &= from_data_opt(library["sources"], library_config.Sources, false);

        value = std::make_unique<kompose::LibraryModuleConfig>(
            std::move(library_config));
        return ok;
    }

    return false;
}

bool data::serializer<kompose::DependenciesConfig>::from_data(
    const toml::node& node, kompose::DependenciesConfig& value)
{
    if (!node.is<toml::table>())
        return false;

    auto ok = true;

    ok &= from_data_opt(node["modules"], value.Modules);
    ok &= from_data_opt(node["maven"], value.Maven);

    return ok;
}

bool data::serializer<kompose::LibraryModulePackage>::from_data(
    const toml::node& node, kompose::LibraryModulePackage& value)
{
    static const std::unordered_map<std::string, kompose::LibraryModulePackage> map
    {
        {"jar", kompose::LibraryModulePackage::Jar},
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
