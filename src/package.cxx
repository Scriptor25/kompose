#include <fstream>
#include <kompose.hxx>
#include <process.hxx>

#include <iostream>
#include <queue>

toolkit::result<> kompose::package(
    const Project &project,
    const Module &module,
    const http::client &client,
    const bool fat)
{
    const auto args_file = module.Build / "args";
    const auto output_file = module.Build / "bundle.jar";

    std::vector<std::string> args
    {
        "-c",
        "-f",
        output_file,
    };

    std::unordered_set<const SourceSet *> source_sets;
    bool include_sources;

    switch (module.Type)
    {
    case ModuleType::Application:
    {
        const auto &data = std::get<ApplicationModuleData>(module.Data);

        source_sets = data.Include;
        include_sources = false;

        args.emplace_back("-e");
        args.push_back(data.Main);

        break;
    }

    case ModuleType::Library:
    {
        const auto &data = get<LibraryModuleData>(module.Data);

        source_sets = data.Include;
        include_sources = data.IncludeSources;

        break;
    }
    }

    if (fat)
    {
        std::queue<const SourceSet *> queue;
        for (const auto *source_set : source_sets)
            queue.push(source_set);

        for (; !queue.empty(); queue.pop())
        {
            const auto *source_set = queue.front();
            source_sets.insert(source_set);

            for (const auto *mod : source_set->Runtime.Modules)
                for (const auto *set : mod->IncludeInCompile())
                    queue.push(set);
        }
    }

    std::unordered_set<std::filesystem::path> jar_paths;

    std::unordered_map<std::filesystem::path, std::filesystem::path> file_paths;
    auto add_directory = [&file_paths](const std::filesystem::path &directory)
    {
        for (const auto &entry : std::filesystem::recursive_directory_iterator(directory))
        {
            if (entry.is_directory())
                continue;

            const auto path = std::filesystem::canonical(entry.path());
            const auto key = std::filesystem::relative(path, directory);

            if (file_paths.contains(key))
                std::cerr << "duplicate file '" << key.string() << "'" << std::endl;

            file_paths[key] = directory;
        }
    };

    for (const auto *source_set : source_sets)
    {
        auto classes_directory = source_set->Build / "classes";
        add_directory(classes_directory);

        auto resources_directory = source_set->Source / "resources";
        add_directory(resources_directory);

        if (include_sources)
        {
            auto sources_directory = source_set->Source / "kotlin";
            add_directory(sources_directory);
        }

        if (fat)
        {
            std::unordered_set<std::filesystem::path> paths;
            if (auto res = resolve(
                               project,
                               client,
                               MavenResolveScope::Runtime,
                               source_set->Runtime.Maven
                           ) >> paths; !res)
                return res;

            jar_paths.insert(paths.begin(), paths.end());
        }
    }

    if (fat)
    {
        const auto fat_directory = module.Build / "fat";

        if (std::error_code ec; std::filesystem::create_directories(fat_directory, ec), ec)
            return toolkit::make_error(
                "failed to create directory '{}': {} ({})",
                fat_directory.string(),
                ec.message(),
                ec.value());

        for (const auto &path : jar_paths)
        {
            std::stringstream out, err;
            if (auto res = Process(
                { "jar", "-x", "-f", path, "-C", fat_directory }
            )(out, err); !res)
            {
                std::cerr << out.str();
                std::cerr << err.str();
                return res;
            }
        }

        if (const auto manifest = fat_directory / "META-INF" / "MANIFEST.MF";
            std::filesystem::exists(manifest))
            if (std::error_code ec; std::filesystem::remove(manifest, ec), ec)
                return toolkit::make_error(
                    "failed to remove file '{}': {} ({})",
                    manifest.string(),
                    ec.message(),
                    ec.value());

        add_directory(fat_directory);
    }

    for (const auto &[file, directory] : file_paths)
    {
        args.emplace_back("-C");
        args.push_back(directory);
        args.push_back(file);
    }

    {
        std::ofstream stream(args_file, std::ios::binary);
        for (const auto &arg : args)
            stream << arg << '\n';
    }

    std::stringstream out, err;
    if (auto res = Process(
        { "jar", std::format("@{}", args_file.string()) }
    )(out, err); !res)
    {
        std::cout << out.str();
        std::cerr << err.str();
        return res;
    }

    return {};
}
