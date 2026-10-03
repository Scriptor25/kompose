#include <project.hxx>

void data::serializer<json::node, kompose::Project>::to_data(
    json::node &node,
    const kompose::Project &value)
{
    node = json::object
    {
        { "name", value.Name },
        { "path", value.Path },
        { "repositories", value.Repositories },
        { "modules", value.Modules },
    };
}

void data::serializer<json::node, kompose::Module>::to_data(
    json::node &node,
    const kompose::Module &value)
{
    const auto &base = value.Parent->Path;

    node = json::object
    {
        { "name", value.Name },
        { "path", std::filesystem::relative(value.Path, base) },
        { "source", std::filesystem::relative(value.Source, base) },
        { "build", std::filesystem::relative(value.Build, base) },
        { "source_sets", value.SourceSets },
        { "data", value.Data },
    };
}

void data::serializer<json::node, std::filesystem::path>::to_data(
    json::node &node,
    const std::filesystem::path &value)
{
    node = json::string(value.string());
}

void data::serializer<json::node, kompose::ModuleType>::to_data(
    json::node &node,
    const kompose::ModuleType &value)
{
    static const std::unordered_map<kompose::ModuleType, std::string_view> map
    {
        { kompose::ModuleType::Application, "application" },
        { kompose::ModuleType::Library, "library" },
    };

    node = json::string(map.at(value));
}

void data::serializer<json::node, kompose::SourceSet>::to_data(
    json::node &node,
    const kompose::SourceSet &value)
{
    const auto &base = value.Parent->Parent->Path;

    node = json::object
    {
        { "name", value.Name },
        { "source", std::filesystem::relative(value.Source, base) },
        { "build", std::filesystem::relative(value.Build, base) },
        {
            "dependencies",
            json::object
            {
                { "compile", value.Compile },
                { "runtime", value.Runtime },
            },
        },
    };
}

void data::serializer<json::node, kompose::ApplicationModuleData>::to_data(
    json::node &node,
    const kompose::ApplicationModuleData &value)
{
    node = json::object
    {
        { "type", json::string("application") },
        { "main", value.Main },
        { "include", value.Include },
    };
}

void data::serializer<json::node, kompose::LibraryModuleData>::to_data(
    json::node &node,
    const kompose::LibraryModuleData &value)
{
    node = json::object
    {
        { "type", json::string("library") },
        { "package", value.Package },
        { "include_sources", value.IncludeSources },
        { "include", value.Include },
    };
}

void data::serializer<json::node, kompose::Dependency>::to_data(
    json::node &node,
    const kompose::Dependency &value)
{
    node = json::object
    {
        { "modules", value.Modules },
        { "maven", value.Maven },
    };
}

void data::serializer<json::node, kompose::MavenCoordinate>::to_data(
    json::node &node,
    const kompose::MavenCoordinate &value)
{
    node = json::object
    {
        { "group", value.Group },
        { "artifact", value.Artifact },
        { "version", value.Version },
        { "path", "repository" / value.locate() / value.get_filename("jar") }
    };
}

void data::serializer<json::node, const kompose::SourceSet *>::to_data(
    json::node &node,
    const kompose::SourceSet *value)
{
    node = json::string(value->Name);
}

void data::serializer<json::node, const kompose::Module *>::to_data(
    json::node &node,
    const kompose::Module *value)
{
    node = json::string(value->Name);
}

void data::serializer<json::node, kompose::LibraryModulePackage>::to_data(
    json::node &node,
    const kompose::LibraryModulePackage &value)
{
    static const std::unordered_map<kompose::LibraryModulePackage, std::string_view> map
    {
        { kompose::LibraryModulePackage::Jar, "jar" },
    };

    node = json::string(map.at(value));
}

void data::serializer<json::node, kompose::RepositoriesConfig>::to_data(
    json::node &node,
    const kompose::RepositoriesConfig &value)
{
    node = json::object
    {
        { "maven", value.Maven },
    };
}
