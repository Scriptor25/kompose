#pragma once

#include <kotlin.hxx>
#include <maven.hxx>

#include <toml/toml.hxx>

#include <data/serializer.hxx>

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_set>

namespace kompose
{
    enum class KotlinCompilerPlatform
    {
        None,
        Jvm,
        Js,
        Native,
    };

    struct CommonCompilerConfigData
    {
        void Merge(const CommonCompilerConfigData &data);

        std::optional<std::string> KotlinHome;

        std::optional<std::string> ApiVersion;
        std::optional<std::string> LanguageVersion;

        std::unordered_set<std::string> OptIn;
        std::optional<bool> Progressive;

        std::unordered_map<std::string, std::string> PluginOptions;

        std::vector<std::string> Extra;
    };

    struct JvmCompilerConfigData
    {
        void Merge(const JvmCompilerConfigData &data);

        std::optional<std::string> JdkHome;
        std::optional<KotlinJvmDefault> Default;
        std::optional<std::string> Target;
        std::optional<bool> JavaParameters;
        std::optional<bool> NoJdk;
        std::optional<bool> NoReflect;
        std::optional<bool> NoStdlib;
        std::unordered_set<std::string> ScriptTemplates;
        std::unordered_set<std::string> AddModules;
        std::optional<bool> ExposeBoxed;
        std::unordered_map<std::string, KotlinJvmNullabilityAnnotationReportLevel> NullabilityAnnotations;
        std::optional<bool> EnablePreview;

        std::vector<std::string> Extra;
    };

    struct JsCompilerConfigData
    {
        void Merge(const JsCompilerConfigData &data);

        std::optional<std::string> ModuleKind;
        std::optional<std::string> Target;
        std::unordered_set<std::string> Libraries;

        std::optional<std::filesystem::path> OutputPrefix;
        std::optional<std::filesystem::path> OutputPostfix;

        std::optional<std::string> SourceMapEmbedSources;
        std::optional<std::string> SourceMapNamesPolicy;
        std::optional<std::string> SourceMapPrefix;
        std::unordered_set<std::filesystem::path> SourceMapBaseDirs;

        std::optional<bool> NoStdlib;

        std::optional<bool> GenerateMetaInfo;
        std::optional<bool> GenerateSourceMap;

        std::vector<std::string> Extra;
    };

    struct NativeCompilerConfigData
    {
        void Merge(const NativeCompilerConfigData &data);

        std::optional<std::string> Target;

        std::optional<bool> EnableDebug;
        std::optional<bool> EnableAssertions;
        std::optional<bool> EnableOptimizations;

        std::optional<bool> NoStdlib;
        std::optional<bool> NoDefaultLibs;

        std::optional<bool> GenerateTestRunner;
        std::optional<bool> GenerateNoExitTestRunner;

        std::vector<std::string> Extra;
    };

    struct KotlinCompilerConfig
    {
        void Merge(const KotlinCompilerConfig &config);

        KotlinCompilerPlatform Platform{};

        CommonCompilerConfigData Common;
        JvmCompilerConfigData Jvm;
        JsCompilerConfigData Js;
        NativeCompilerConfigData Native;
    };

    struct ProjectArtifactConfig
    {
        std::optional<std::string> Group;
        std::optional<std::string> Version;
    };

    struct ArtifactConfig
    {
        std::optional<std::string> Group;
        std::optional<std::string> Name;
        std::optional<std::string> Version;
    };

    struct ModulesConfig
    {
        std::unordered_set<std::string> Include;
    };

    struct RepositoriesConfig
    {
        std::vector<std::string> Maven;
    };

    struct DependencyConfig
    {
        std::unordered_set<std::string> Modules;
        std::unordered_set<MavenCoordinate> Maven;
    };

    struct DependenciesConfig
    {
        DependencyConfig Common;
        DependencyConfig Compile;
        DependencyConfig Runtime;
    };

    struct ProjectConfig
    {
        std::optional<std::string> Name;

        ProjectArtifactConfig Artifact;

        ModulesConfig Modules;

        RepositoriesConfig Repositories;
        DependenciesConfig Dependencies;

        KotlinCompilerConfig CompilerConfig;
    };

    enum class ModuleType
    {
        Application,
        Library,
    };

    struct ApplicationModuleConfigData
    {
        std::string Main;
        std::unordered_set<std::string> Include;
    };

    struct LibraryModuleConfigData
    {
        bool IncludeSources;
        std::unordered_set<std::string> Include;
    };

    struct SourceSetConfig
    {
        DependenciesConfig Dependencies;
        KotlinCompilerConfig CompilerConfig;
    };

    struct ModuleConfig
    {
        std::filesystem::path Path;

        ModuleType Type;
        std::optional<std::string> Name;

        ArtifactConfig Artifact;

        DependenciesConfig Dependencies;

        std::unordered_map<std::string, SourceSetConfig> SourceSets;

        std::variant<ApplicationModuleConfigData, LibraryModuleConfigData> Data;

        KotlinCompilerConfig CompilerConfig;
    };
} // namespace kompose

template<>
struct data::serializer<toml::node, kompose::ProjectConfig>
{
    static bool from_data(
        const toml::node &node,
        kompose::ProjectConfig &value);
};

template<>
struct data::serializer<toml::node, kompose::ModuleConfig>
{
    static bool from_data(
        const toml::node &node,
        kompose::ModuleConfig &value);
};

template<>
struct data::serializer<toml::node, kompose::SourceSetConfig>
{
    static bool from_data(
        const toml::node &node,
        kompose::SourceSetConfig &value);
};

template<>
struct data::serializer<toml::node, kompose::DependencyConfig>
{
    static bool from_data(
        const toml::node &node,
        kompose::DependencyConfig &value);
};

template<>
struct data::serializer<toml::node, kompose::MavenCoordinate>
{
    static bool from_data(
        const toml::node &node,
        kompose::MavenCoordinate &value);
};

template<>
struct data::serializer<toml::node, kompose::KotlinCompilerConfig>
{
    static bool from_data(
        const toml::node &node,
        kompose::KotlinCompilerConfig &value);
};

template<>
struct data::serializer<toml::node, kompose::KotlinCompilerPlatform>
{
    static bool from_data(
        const toml::node &node,
        kompose::KotlinCompilerPlatform &value);
};

template<>
struct data::serializer<toml::node, kompose::CommonCompilerConfigData>
{
    static bool from_data(
        const toml::node &node,
        kompose::CommonCompilerConfigData &value);
};

template<>
struct data::serializer<toml::node, kompose::JvmCompilerConfigData>
{
    static bool from_data(
        const toml::node &node,
        kompose::JvmCompilerConfigData &value);
};

template<>
struct data::serializer<toml::node, kompose::JsCompilerConfigData>
{
    static bool from_data(
        const toml::node &node,
        kompose::JsCompilerConfigData &value);
};

template<>
struct data::serializer<toml::node, kompose::NativeCompilerConfigData>
{
    static bool from_data(
        const toml::node &node,
        kompose::NativeCompilerConfigData &value);
};

template<>
struct data::serializer<toml::node, std::filesystem::path>
{
    static bool from_data(
        const toml::node &node,
        std::filesystem::path &value);
};

template<>
struct data::serializer<toml::node, kompose::KotlinJvmDefault>
{
    static bool from_data(
        const toml::node &node,
        kompose::KotlinJvmDefault &value);
};

template<>
struct data::serializer<toml::node, kompose::KotlinJvmNullabilityAnnotationReportLevel>
{
    static bool from_data(
        const toml::node &node,
        kompose::KotlinJvmNullabilityAnnotationReportLevel &value);
};
