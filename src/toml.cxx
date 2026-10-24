#include <config.hxx>
#include <project.hxx>

#include <toolkit/string.hxx>

void kompose::CommonCompilerConfigData::Merge(const CommonCompilerConfigData &data)
{
    if (!KotlinHomePath)
        KotlinHomePath = data.KotlinHomePath;
    if (!ApiVersion)
        ApiVersion = data.ApiVersion;
    if (!LanguageVersion)
        LanguageVersion = data.LanguageVersion;
    OptIn.insert(data.OptIn.begin(), data.OptIn.end());
    if (!Progressive)
        Progressive = data.Progressive;
    PluginOptions.insert(data.PluginOptions.begin(), data.PluginOptions.end());
    ExtraArguments.insert(ExtraArguments.end(), data.ExtraArguments.begin(), data.ExtraArguments.end());
}

void kompose::JvmCompilerConfigData::Merge(const JvmCompilerConfigData &data)
{
    if (!JdkHomePath)
        JdkHomePath = data.JdkHomePath;
    if (!DefaultMode)
        DefaultMode = data.DefaultMode;
    if (!Target)
        Target = data.Target;
    if (!NoJdk)
        NoJdk = data.NoJdk;
    if (!NoStdlib)
        NoStdlib = data.NoStdlib;
    if (!NoReflect)
        NoReflect = data.NoReflect;
    if (!GenerateJavaParameters)
        GenerateJavaParameters = data.GenerateJavaParameters;
}

void kompose::JsCompilerConfigData::Merge(const JsCompilerConfigData &data)
{
    if (!ModuleKind)
        ModuleKind = data.ModuleKind;
    if (!Target)
        Target = data.Target;
    Libraries.insert(data.Libraries.begin(), data.Libraries.end());
    if (!OutputPrefix)
        OutputPrefix = data.OutputPrefix;
    if (!OutputPostfix)
        OutputPostfix = data.OutputPostfix;
    if (!SourceMapEmbedSources)
        SourceMapEmbedSources = data.SourceMapEmbedSources;
    if (!SourceMapNamesPolicy)
        SourceMapNamesPolicy = data.SourceMapNamesPolicy;
    if (!SourceMapPrefix)
        SourceMapPrefix = data.SourceMapPrefix;
    SourceMapBaseDirs.insert(data.SourceMapBaseDirs.begin(), data.SourceMapBaseDirs.end());
    if (!NoStdlib)
        NoStdlib = data.NoStdlib;
    if (!GenerateMetaInfo)
        GenerateMetaInfo = data.GenerateMetaInfo;
    if (!GenerateSourceMap)
        GenerateSourceMap = data.GenerateSourceMap;
}

void kompose::NativeCompilerConfigData::Merge(const NativeCompilerConfigData &data)
{
    if (!Target)
        Target = data.Target;
    if (!EnableDebug)
        EnableDebug = data.EnableDebug;
    if (!EnableAssertions)
        EnableAssertions = data.EnableAssertions;
    if (!EnableOptimizations)
        EnableOptimizations = data.EnableOptimizations;
    if (!NoStdlib)
        NoStdlib = data.NoStdlib;
    if (!NoDefaultLibs)
        NoDefaultLibs = data.NoDefaultLibs;
    if (!GenerateTestRunner)
        GenerateTestRunner = data.GenerateTestRunner;
    if (!GenerateNoExitTestRunner)
        GenerateNoExitTestRunner = data.GenerateNoExitTestRunner;
}

void kompose::KotlinCompilerConfig::Merge(const KotlinCompilerConfig &config)
{
    if (Platform == KotlinCompilerPlatform::None)
        Platform = config.Platform;

    Common.Merge(config.Common);
    Jvm.Merge(config.Jvm);
    Js.Merge(config.Js);
    Native.Merge(config.Native);
}

bool data::serializer<toml::node, kompose::ProjectConfig>::from_data(
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

    ok &= from_data_opt(dependencies, value.Dependencies.Common);
    ok &= from_data_opt(dependencies["compile"], value.Dependencies.Compile);
    ok &= from_data_opt(dependencies["runtime"], value.Dependencies.Runtime);

    ok &= from_data_opt(node["compiler"], value.CompilerConfig);

    return ok;
}

bool data::serializer<toml::node, kompose::ModuleConfig>::from_data(
    const toml::node &node,
    kompose::ModuleConfig &value)
{
    if (!node.is<toml::table>())
        return false;

    std::string type;
    if (!(node["type"] >> type))
        return false;

    auto &artifact = node["artifact"];
    auto &dependencies = node["dependencies"];
    auto &sources = node["sources"];

    auto ok = true;

    ok &= node["name"] >> value.Name;

    ok &= artifact["group"] >> value.Artifact.Group;
    ok &= artifact["name"] >> value.Artifact.Name;
    ok &= artifact["version"] >> value.Artifact.Version;

    ok &= from_data_opt(dependencies, value.Dependencies.Common);
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

        ok &= from_data_opt(library["sources"], data.IncludeSources, false);
        ok &= from_data_opt(library["include"], data.Include, {});

        value.Data = std::move(data);
        return ok;
    }

    ok &= from_data_opt(node["compiler"], value.CompilerConfig);

    return false;
}

bool data::serializer<toml::node, kompose::SourceSetConfig>::from_data(
    const toml::node &node,
    kompose::SourceSetConfig &value)
{
    if (!node)
        return true;

    if (!node.is<toml::table>())
        return false;

    auto &dependencies = node["dependencies"];

    auto ok = true;

    ok &= from_data_opt(dependencies, value.Dependencies.Common);
    ok &= from_data_opt(dependencies["compile"], value.Dependencies.Compile);
    ok &= from_data_opt(dependencies["runtime"], value.Dependencies.Runtime);

    ok &= from_data_opt(node["compiler"], value.CompilerConfig);

    return ok;
}

bool data::serializer<toml::node, kompose::DependencyConfig>::from_data(
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

bool data::serializer<toml::node, kompose::MavenCoordinate>::from_data(
    const toml::node &node,
    kompose::MavenCoordinate &value)
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
            value.Version = std::move(segments[2]);
            break;
        case 4:
            value.Group = std::move(segments[0]);
            value.Artifact = std::move(segments[1]);
            value.Version = std::move(segments[2]);
            value.Classifier = std::move(segments[3]);
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
    ok &= node["version"] >> value.Version;
    ok &= node["classifier"] >> value.Classifier;

    return ok;
}

bool data::serializer<toml::node, kompose::KotlinCompilerConfig>::from_data(
    const toml::node &node,
    kompose::KotlinCompilerConfig &value)
{
    if (!node.is<toml::table>())
        return false;

    auto ok = true;

    ok &= from_data_opt(node["platform"], value.Platform);
    ok &= from_data_opt(node["common"], value.Common);
    ok &= from_data_opt(node["jvm"], value.Jvm);
    ok &= from_data_opt(node["js"], value.Js);
    ok &= from_data_opt(node["native"], value.Native);

    return ok;
}

bool data::serializer<toml::node, kompose::KotlinCompilerPlatform>::from_data(
    const toml::node &node,
    kompose::KotlinCompilerPlatform &value)
{
    static const std::unordered_map<std::string, kompose::KotlinCompilerPlatform> map
    {
        { "jvm", kompose::KotlinCompilerPlatform::Jvm },
        { "js", kompose::KotlinCompilerPlatform::Js },
        { "native", kompose::KotlinCompilerPlatform::Native },
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

bool data::serializer<toml::node, kompose::CommonCompilerConfigData>::from_data(
    const toml::node &node,
    kompose::CommonCompilerConfigData &value)
{
    if (!node.is<toml::table>())
        return false;

    auto ok = true;

    ok &= node["kotlinHomePath"] >> value.KotlinHomePath;
    ok &= node["apiVersion"] >> value.ApiVersion;
    ok &= node["languageVersion"] >> value.LanguageVersion;

    ok &= from_data_opt(node["optIn"], value.OptIn);
    ok &= node["progressive"] >> value.Progressive;
    ok &= from_data_opt(node["pluginOptions"], value.PluginOptions);
    ok &= from_data_opt(node["extraArguments"], value.ExtraArguments);

    return ok;
}

bool data::serializer<toml::node, kompose::JvmCompilerConfigData>::from_data(
    const toml::node &node,
    kompose::JvmCompilerConfigData &value)
{
    if (!node.is<toml::table>())
        return false;

    auto ok = true;

    ok &= node["jdkHomePath"] >> value.JdkHomePath;
    ok &= node["defaultMode"] >> value.DefaultMode;
    ok &= node["target"] >> value.Target;

    ok &= node["noJdk"] >> value.NoJdk;
    ok &= node["noStdlib"] >> value.NoStdlib;
    ok &= node["noReflect"] >> value.NoReflect;

    ok &= node["generateJavaParameters"] >> value.GenerateJavaParameters;

    return ok;
}

bool data::serializer<toml::node, kompose::JsCompilerConfigData>::from_data(
    const toml::node &node,
    kompose::JsCompilerConfigData &value)
{
    if (!node.is<toml::table>())
        return false;

    auto ok = true;

    ok &= node["moduleKind"] >> value.ModuleKind;
    ok &= node["target"] >> value.Target;

    ok &= from_data_opt(node["libraries"], value.Libraries);

    ok &= node["outputPrefix"] >> value.OutputPrefix;
    ok &= node["outputPostfix"] >> value.OutputPostfix;

    ok &= node["sourceMapEmbedSources"] >> value.SourceMapEmbedSources;
    ok &= node["sourceMapNamesPolicy"] >> value.SourceMapNamesPolicy;
    ok &= node["sourceMapPrefix"] >> value.SourceMapPrefix;

    ok &= from_data_opt(node["sourceMapBaseDirs"], value.SourceMapBaseDirs);

    ok &= node["noStdlib"] >> value.NoStdlib;

    ok &= node["generateMetaInfo"] >> value.GenerateMetaInfo;
    ok &= node["generateSourceMap"] >> value.GenerateSourceMap;

    return ok;
}

bool data::serializer<toml::node, kompose::NativeCompilerConfigData>::from_data(
    const toml::node &node,
    kompose::NativeCompilerConfigData &value)
{
    if (!node.is<toml::table>())
        return false;

    auto ok = true;

    ok &= node["target"] >> value.Target;

    ok &= node["enableDebug"] >> value.EnableDebug;
    ok &= node["enableAssertions"] >> value.EnableAssertions;
    ok &= node["enableOptimizations"] >> value.EnableOptimizations;

    ok &= node["noStdlib"] >> value.NoStdlib;
    ok &= node["noDefaultLibs"] >> value.NoDefaultLibs;

    ok &= node["generateTestRunner"] >> value.GenerateTestRunner;
    ok &= node["generateNoExitTestRunner"] >> value.GenerateNoExitTestRunner;

    return ok;
}

bool data::serializer<toml::node, std::filesystem::path>::from_data(
    const toml::node &node,
    std::filesystem::path &value)
{
    std::string str;
    if (!(node >> str))
        return false;

    value = std::move(str);
    return true;
}
