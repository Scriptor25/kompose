#include <kompose.hxx>
#include <kotlin.hxx>

#include <fstream>
#include <iostream>

toolkit::result<> kompose::compile(
    const Project &project,
    const Module &module,
    const SourceSet &source_set,
    const http::client &client)
{
    const auto destination_path = source_set.Build / "classes";
    const auto fingerprint_path = source_set.Build / "fingerprint";

    const auto kotlin_directory = source_set.Source / "kotlin";

    if (!std::filesystem::exists(kotlin_directory) || std::filesystem::is_empty(kotlin_directory))
        return {};

    Hash hash;

    std::unordered_set<std::string> sources;
    for (const auto &entry : std::filesystem::recursive_directory_iterator(kotlin_directory))
    {
        if (entry.is_directory())
            continue;

        const auto &path = entry.path();
        if (!path.has_extension() || path.extension() != ".kt")
            continue;

        sources.insert(path);

        std::ifstream stream(path, std::ios::binary);
        hash.Push(stream);
    }

    std::string target_fingerprint;
    if (std::filesystem::exists(fingerprint_path))
    {
        std::ifstream stream(fingerprint_path, std::ios::binary);
        stream.seekg(0, std::ios::end);
        auto count = stream.tellg();
        stream.seekg(0, std::ios::beg);

        std::string buffer(count, '\0');
        stream.read(buffer.data(), count);

        target_fingerprint = std::move(buffer);
    }

    if (!std::filesystem::exists(destination_path))
        if (std::error_code ec; std::filesystem::create_directories(destination_path, ec), ec)
            return toolkit::make_error(
                "failed to create directory '{}': {} ({})",
                destination_path.string(),
                ec.message(),
                ec.value());

    std::unordered_set<std::filesystem::path> class_path;
    for (const auto *dependency : source_set.Compile.Modules)
        for (const auto source_sets = dependency->GetSourceSets(false); const auto *set : source_sets)
            class_path.insert(set->Build / "classes");

    for (const auto *set : source_set.Include)
        class_path.insert(set->Build / "classes");

    {
        std::unordered_set<std::filesystem::path> paths;
        if (auto res = resolve(
                           project,
                           client,
                           MavenResolveScope::Compile,
                           source_set.Compile.Maven
                       ) >> paths; !res)
            return res;

        class_path.insert(paths.begin(), paths.end());
    }

    for (const auto &path : class_path)
    {
        if (!std::filesystem::exists(path))
            continue;

        if (std::filesystem::is_directory(path))
        {
            for (const auto &entry : std::filesystem::recursive_directory_iterator(path))
            {
                if (entry.is_directory())
                    continue;

                std::ifstream stream(entry.path(), std::ios::binary);
                hash.Push(stream);
            }
            continue;
        }

        std::ifstream stream(path, std::ios::binary);
        hash.Push(stream);
    }

    const auto &kotlin_home = source_set.Compiler.Common.KotlinHome;
    const auto &kotlin_version = source_set.Compiler.Common.KotlinVersion;

    std::vector<std::string> executable;
    if (kotlin_home)
        executable.push_back(std::filesystem::path(*kotlin_home) / "bin" / "kotlinc");
    else if (kotlin_version)
    {
        const MavenCoordinate coordinate
        {
            .Group    = "org.jetbrains.kotlin",
            .Artifact = "kotlin-compiler-embeddable",
            .Version  = *kotlin_version,
        };

        std::unordered_set<std::filesystem::path> compiler_class_path;
        if (auto res = resolve(
                           project,
                           client,
                           MavenResolveScope::Runtime,
                           { coordinate }
                       ) >> compiler_class_path; !res)
            return res;

        std::string compiler_class_path_string;
        for (auto it = compiler_class_path.begin(); it != compiler_class_path.end(); ++it)
        {
            if (it != compiler_class_path.begin())
                compiler_class_path_string += ':';
            compiler_class_path_string += it->string();
        }

        executable.emplace_back("java");
        executable.emplace_back("--enable-native-access=ALL-UNNAMED");
        executable.emplace_back("--sun-misc-unsafe-memory-access=allow");
        executable.emplace_back("--class-path");
        executable.push_back(std::move(compiler_class_path_string));
        executable.emplace_back("org.jetbrains.kotlin.cli.jvm.K2JVMCompiler");
        executable.emplace_back("-no-stdlib");
        executable.emplace_back("-no-reflect");
    }
    else
        executable.emplace_back("kotlinc");

    KotlinCommand command
    {
        .Executable      = std::move(executable),
        .Input           = std::move(sources),
        .ApiVersion      = source_set.Compiler.Common.ApiVersion,
        .KotlinHome      = kotlin_home,
        .LanguageVersion = source_set.Compiler.Common.LanguageVersion,
        .OptIn           = source_set.Compiler.Common.OptIn,
        .PluginOptions   = source_set.Compiler.Common.PluginOptions,
        .Progressive     = source_set.Compiler.Common.Progressive.value_or(false),
    };

    command.PushExtra(source_set.Compiler.Common.Extra);

    switch (source_set.Compiler.Platform)
    {
    case KotlinCompilerPlatform::Jvm:
    {
        const auto &data = std::get<JvmCompilerConfigData>(source_set.Compiler.Data);

        std::unordered_set<std::string> class_path_string;
        for (auto &entry : class_path)
            class_path_string.insert(entry.string());
        command.JvmClassPath = std::move(class_path_string);
        command.JvmDestination = destination_path;
        if (module.Type == ModuleType::Application)
            command.JvmIncludeRuntime = true;
        command.JvmJdkHome = data.JdkHome;
        if (data.Default)
            command.JvmDefault = *data.Default;
        command.JvmTarget = data.Target;
        if (data.JavaParameters)
            command.JvmJavaParameters = *data.JavaParameters;
        command.JvmModuleName = module.Id;
        if (data.NoJdk)
            command.JvmNoJdk = *data.NoJdk;
        if (data.NoReflect)
            command.JvmNoReflect = *data.NoReflect;
        if (data.NoStdlib)
            command.JvmNoStdlib = *data.NoStdlib;
        command.JvmScriptTemplates = data.ScriptTemplates;
        command.JvmAddModules = data.AddModules;
        if (data.ExposeBoxed)
            command.JvmExposeBoxed = *data.ExposeBoxed;
        command.JvmNullabilityAnnotations = data.NullabilityAnnotations;
        if (data.EnablePreview)
            command.JvmEnablePreview = *data.EnablePreview;

        command.PushExtra(data.Extra);
        break;
    }
    case KotlinCompilerPlatform::Js:
        return toolkit::make_error("TODO: platform.js");
    case KotlinCompilerPlatform::Native:
        return toolkit::make_error("TODO: platform.native");
    default:
        return toolkit::make_error("TODO: platform.none");
    }

    auto process = command.Build();
    process.Push(hash);

    std::string fingerprint;
    hash(fingerprint);

    if (fingerprint == target_fingerprint)
        return {};

    if (auto res = process(std::cout, std::cerr); !res)
        return res;

    {
        std::ofstream stream(fingerprint_path, std::ios::binary);
        stream << fingerprint;
    }

    return {};
}
