#include <fstream>
#include <kompose.hxx>
#include <kotlin.hxx>

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

    std::string class_path_string;
    for (auto it = class_path.begin(); it != class_path.end(); ++it)
    {
        if (it != class_path.begin())
            class_path_string += ':';
        class_path_string += *it;
    }

    // TODO: make language version, api version and jvm target version controllable
    KotlinCommand command
    {
        .Input = std::move(sources),
        .ApiVersion = "2.4",
        .LanguageVersion = "2.4",
        .JvmClassPath = std::move(class_path_string),
        .JvmDestination = destination_path,
        .JvmTarget = "25",
        .JvmModuleName = module.Name,
    };

    switch (module.Type)
    {
    case ModuleType::Application:
        command.JvmIncludeRuntime = true;
        break;

    case ModuleType::Library:
        break;
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
