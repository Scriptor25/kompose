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
        {
            "artifact",
            json::object
            {
                { "group", value.Artifact.Group },
                { "artifact", value.Artifact.Artifact },
                { "version", value.Artifact.Version },
            },
        },
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
        { "compiler", value.Compiler },
    };
}

void data::serializer<json::node, kompose::ApplicationModuleData>::to_data(
    json::node &node,
    const kompose::ApplicationModuleData &value)
{
    node = json::object
    {
        { "type", "application" },
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
        { "type", "library" },
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
        { "maven", value.Tree },
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
        { "classifier", value.Classifier },
        { "path", "repository" / value.locate() / value.get_filename("jar") },
        { "sources", "repository" / value.locate() / value.get_filename("sources", "jar") },
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

void data::serializer<json::node, kompose::RepositoriesConfig>::to_data(
    json::node &node,
    const kompose::RepositoriesConfig &value)
{
    node = json::object
    {
        { "maven", value.Maven },
    };
}

void data::serializer<json::node, kompose::KotlinCompiler>::to_data(
    json::node &node,
    const kompose::KotlinCompiler &value)
{
    node = json::object
    {
        { "common", value.Common },
        { "data", value.Data },
    };
}

void data::serializer<json::node, kompose::CommonCompilerConfigData>::to_data(
    json::node &node,
    const kompose::CommonCompilerConfigData &value)
{
    node = json::object
    {
        { "kotlin_home", value.KotlinHome },
        { "api_version", value.ApiVersion },
        { "language_version", value.LanguageVersion },
        { "opt_in", value.OptIn },
        { "progressive", value.Progressive },
        { "plugin_options", value.PluginOptions },
        { "extra", value.Extra },
    };
}

void data::serializer<json::node, kompose::JvmCompilerConfigData>::to_data(
    json::node &node,
    const kompose::JvmCompilerConfigData &value)
{
    node = json::object
    {
        { "type", "jvm" },
        { "jdk_home", value.JdkHome },
        { "default", value.Default },
        { "target", value.Target },
        { "java_parameters", value.JavaParameters },
        { "no_jdk", value.NoJdk },
        { "no_reflect", value.NoReflect },
        { "no_stdlib", value.NoStdlib },
        { "script_templates", value.ScriptTemplates },
        { "add_modules", value.AddModules },
        { "expose_boxed", value.ExposeBoxed },
        { "nullability_annotations", value.NullabilityAnnotations },
        { "enable_preview", value.EnablePreview },
        { "extra", value.Extra },
    };
}

void data::serializer<json::node, kompose::JsCompilerConfigData>::to_data(
    json::node &node,
    const kompose::JsCompilerConfigData &value)
{
    node = json::object
    {
        { "type", "js" },
        { "module_kind", value.ModuleKind },
        { "target", value.Target },
        { "libraries", value.Libraries },
        { "output_prefix", value.OutputPrefix },
        { "output_postfix", value.OutputPostfix },
        { "source_map_embed_sources", value.SourceMapEmbedSources },
        { "source_map_names_policy", value.SourceMapNamesPolicy },
        { "source_map_prefix", value.SourceMapPrefix },
        { "source_map_base_dirs", value.SourceMapBaseDirs },
        { "no_stdlib", value.NoStdlib },
        { "generate_meta_info", value.GenerateMetaInfo },
        { "generate_source_map", value.GenerateSourceMap },
        { "extra", value.Extra },
    };
}

void data::serializer<json::node, kompose::NativeCompilerConfigData>::to_data(
    json::node &node,
    const kompose::NativeCompilerConfigData &value)
{
    node = json::object
    {
        { "type", "native" },
        { "target", value.Target },
        { "enable_debug", value.EnableDebug },
        { "enable_assertions", value.EnableAssertions },
        { "enable_optimizations", value.EnableOptimizations },
        { "no_stdlib", value.NoStdlib },
        { "no_default_libs", value.NoDefaultLibs },
        { "generate_test_runner", value.GenerateTestRunner },
        { "generate_no_exit_test_runner", value.GenerateNoExitTestRunner },
        { "extra", value.Extra },
    };
}

void data::serializer<json::node, const char *>::to_data(
    json::node &node,
    const char *value)
{
    node = json::string(value);
}

void data::serializer<json::node, kompose::KotlinJvmDefault>::to_data(
    json::node &node,
    const kompose::KotlinJvmDefault &value)
{
    static const std::unordered_map<kompose::KotlinJvmDefault, const char *> map
    {
        { kompose::KotlinJvmDefault::None, "none" },
        { kompose::KotlinJvmDefault::Enable, "enable" },
        { kompose::KotlinJvmDefault::NoCompatibility, "no-compatibility" },
        { kompose::KotlinJvmDefault::Disable, "disable" },
    };

    node = map.at(value);
}

void data::serializer<json::node, kompose::KotlinJvmNullabilityAnnotationReportLevel>::to_data(
    json::node &node,
    const kompose::KotlinJvmNullabilityAnnotationReportLevel &value)
{
    static const std::unordered_map<kompose::KotlinJvmNullabilityAnnotationReportLevel, const char *> map
    {
        { kompose::KotlinJvmNullabilityAnnotationReportLevel::Ignore, "ignore" },
        { kompose::KotlinJvmNullabilityAnnotationReportLevel::Warn, "warn" },
        { kompose::KotlinJvmNullabilityAnnotationReportLevel::Strict, "strict" },
    };

    node = map.at(value);
}
