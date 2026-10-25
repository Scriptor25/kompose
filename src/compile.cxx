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

    Hash hash;

    std::unordered_set<std::string> sources;
    for (const auto &entry : std::filesystem::recursive_directory_iterator(source_set.Source / "kotlin"))
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

        std::vector<char> buffer(count);
        stream.read(buffer.data(), count);

        target_fingerprint = { buffer.begin(), buffer.end() };
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
        for (const auto source_sets = dependency->IncludeInCompile();
             const auto *set : source_sets)
            class_path.insert(set->Build / "classes");

    {
        std::unordered_set<std::filesystem::path> paths;
        if (auto res = resolve(project, client, MavenResolveScope::Compile, source_set.Compile.Maven) >> paths; !res)
            return res;

        for (const auto &path : paths)
            class_path.insert(path);
    }

    for (const auto &path : class_path)
    {
        if (std::filesystem::is_directory(path))
        {
            for (const auto &entry : std::filesystem::recursive_directory_iterator(path))
            {
                if (entry.is_directory())
                    continue;

                std::ifstream stream(entry.path(), std::ios::binary);
                hash.Push(stream);
            }
        }
        else
        {
            std::ifstream stream(path, std::ios::binary);
            hash.Push(stream);
        }
    }

    KotlinCommand command
    {
        .Input = std::move(sources),
    };

    command.ApiVersion = source_set.Compiler.Common.ApiVersion;
    command.KotlinHome = source_set.Compiler.Common.KotlinHomePath;
    command.LanguageVersion = source_set.Compiler.Common.LanguageVersion;
    command.OptIn = source_set.Compiler.Common.OptIn;
    command.PluginOptions = source_set.Compiler.Common.PluginOptions;
    command.Progressive = source_set.Compiler.Common.Progressive.value_or(false);

    command.InsertExtra(source_set.Compiler.Common.Extra);

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
        command.JvmModuleName = module.Name;
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

        command.InsertExtra(data.Extra);
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
