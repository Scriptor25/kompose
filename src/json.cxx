#include <project.hxx>

void data::serializer<kompose::Project>::to_data(json::node &node, const kompose::Project &value)
{
    node = json::object
    {
        { "name", value.Name },
        { "path", value.Path },
        { "modules", value.Modules },
    };
}

void data::serializer<kompose::Module>::to_data(json::node &node, const kompose::Module &value)
{
    const auto &base = value.Parent->Path;

    node = json::object
    {
        { "type", value.Type },
        { "name", value.Name },
        { "source", std::filesystem::relative(value.Source, base) },
        { "build", std::filesystem::relative(value.Build, base) },
        { "source_sets", value.SourceSets },
        { "data", value.Data },
    };
}

void data::serializer<std::filesystem::path>::to_data(json::node &node, const std::filesystem::path &value)
{
    node = json::string(value.string());
}

void data::serializer<kompose::ModuleType>::to_data(json::node &node, const kompose::ModuleType &value)
{
    static const std::unordered_map<kompose::ModuleType, std::string_view> map
    {
        { kompose::ModuleType::Application, "application" },
        { kompose::ModuleType::Library, "library" },
    };

    node = json::string(map.at(value));
}

void data::serializer<kompose::SourceSet>::to_data(json::node &node, const kompose::SourceSet &value)
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

void data::serializer<kompose::ApplicationModuleData>::to_data(
    json::node &node,
    const kompose::ApplicationModuleData &value)
{
    node = json::object
    {
        { "main", value.Main },
        { "include", value.Include },
    };
}

void data::serializer<kompose::LibraryModuleData>::to_data(json::node &node, const kompose::LibraryModuleData &value)
{
    node = json::object
    {
        { "package", value.Package },
        { "include_sources", value.IncludeSources },
        { "include", value.Include },
    };
}

void data::serializer<kompose::Dependency>::to_data(json::node &node, const kompose::Dependency &value)
{
    node = json::object
    {
        { "modules", value.Modules },
        { "maven", value.Maven },
    };
}

void data::serializer<const kompose::SourceSet *>::to_data(json::node &node, const kompose::SourceSet *value)
{
    node = json::string(value->Name);
}

void data::serializer<const kompose::Module *>::to_data(json::node &node, const kompose::Module *value)
{
    node = json::string(value->Name);
}

void data::serializer<kompose::LibraryModulePackage>::to_data(
    json::node &node,
    const kompose::LibraryModulePackage &value)
{
    static const std::unordered_map<kompose::LibraryModulePackage, std::string_view> map
    {
        { kompose::LibraryModulePackage::Jar, "jar" },
    };

    node = json::string(map.at(value));
}
