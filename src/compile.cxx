#include <kompose.hxx>
#include <kotlin.hxx>

#include <iostream>

toolkit::result<> kompose::compile(const Module &module, const SourceSet &source_set)
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

    std::unordered_set<std::string> class_path;
    for (const auto *dependency : source_set.Compile.Modules)
        for (const auto source_sets = dependency->IncludeInCompile();
             const auto *set : source_sets)
            class_path.insert(set->Build / "classes");

    KotlinCommand command
    {
        .Input = std::move(sources),
        .JvmClassPath = std::move(class_path),
        .JvmDestination = destination,
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

    std::string out, err;
    if (auto res = command(out, err); !res)
    {
        std::cout << out;
        std::cerr << err;
        return res;
    }

    return {};
}
