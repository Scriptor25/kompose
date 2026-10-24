#include <fstream>
#include <kompose.hxx>
#include <kotlin.hxx>

#include <iostream>
#include <sstream>

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

    std::string class_path_string;
    for (auto it = class_path.begin(); it != class_path.end(); ++it)
    {
        if (it != class_path.begin())
            class_path_string += ':';
        class_path_string += *it;
    }

    KotlinCommand command
    {
        .Input = std::move(sources),
    };

    command.ApiVersion = source_set.Compiler.Common.ApiVersion;
    command.LanguageVersion = source_set.Compiler.Common.LanguageVersion;
    command.KotlinHome = source_set.Compiler.Common.KotlinHomePath;
    command.OptIn = source_set.Compiler.Common.OptIn;
    command.PluginOptions = source_set.Compiler.Common.PluginOptions;
    command.Progressive = source_set.Compiler.Common.Progressive.value_or(false);
    command.ExtraArguments = source_set.Compiler.Common.ExtraArguments;

    switch (source_set.Compiler.Platform)
    {
    case KotlinCompilerPlatform::Jvm:
    {
        const auto &data = std::get<JvmCompilerConfigData>(source_set.Compiler.Data);

        command.JvmModuleName = module.Name;
        command.JvmDestination = destination_path;
        command.JvmClassPath = class_path_string;

        command.JvmTarget = data.Target;
        command.JvmJdkHome = data.JdkHomePath;

        command.JvmJavaParameters = data.GenerateJavaParameters.value_or(false);
        command.JvmNoJdk = data.NoJdk.value_or(false);
        command.JvmNoReflect = data.NoReflect.value_or(false);
        command.JvmNoStdlib = data.NoStdlib.value_or(false);

        if (data.DefaultMode)
        {
            if (data.DefaultMode == "disable")
                command.JvmDefaultMode = KotlinJvmDefaultMode::Disable;
            if (data.DefaultMode == "enable")
                command.JvmDefaultMode = KotlinJvmDefaultMode::Enable;
            if (data.DefaultMode == "no-compatibility")
                command.JvmDefaultMode = KotlinJvmDefaultMode::NoCompatibility;
        }

        switch (module.Type)
        {
        case ModuleType::Application:
            command.JvmIncludeRuntime = true;
            break;

        case ModuleType::Library:
            break;
        }

        break;
    }
    case KotlinCompilerPlatform::Js:
        return toolkit::make_error("TODO");
    case KotlinCompilerPlatform::Native:
        return toolkit::make_error("TODO");
    default:
        return toolkit::make_error("TODO");
    }

    auto process = command.Build();
    process.Push(hash);

    std::string fingerprint;
    hash(fingerprint);

    if (fingerprint == target_fingerprint)
        return {};

    std::stringstream out, err;
    if (auto res = process(out, err); !res)
    {
        std::cout << out.str();
        std::cerr << err.str();
        return res;
    }

    {
        std::ofstream stream(fingerprint_path, std::ios::binary);
        stream << fingerprint;
    }

    return {};
}
