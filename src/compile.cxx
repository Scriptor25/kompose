#include <kompose.hxx>
#include <kotlin.hxx>

#include <iostream>

toolkit::result<> kompose::compile(
    const Project &project,
    const Module &module,
    const SourceSet &source_set,
    const http::client &client)
{
    std::unordered_set<std::string> sources;
    for (const auto &entry : std::filesystem::recursive_directory_iterator(source_set.Source / "kotlin"))
    {
        if (entry.is_directory())
            continue;

        const auto &path = entry.path();
        if (!path.has_extension() || path.extension() != ".kt")
            continue;

        sources.insert(path);
    }

    const auto destination = source_set.Build / "classes";

    std::filesystem::create_directories(destination);

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
        .ApiVersion = "2.4",
        .LanguageVersion = "2.4",
        .JvmClassPath = std::move(class_path_string),
        .JvmDestination = destination,
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

    std::stringstream out, err;
    if (auto res = command(out, err); !res)
    {
        std::cout << out.str();
        std::cerr << err.str();
        return res;
    }

    return {};
}
