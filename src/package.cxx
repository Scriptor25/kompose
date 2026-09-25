#include <kompose.hxx>
#include <process.hxx>

#include <iostream>

toolkit::result<> kompose::package(const Module &module)
{
    auto output_file = module.Build / "bundle.jar";

    std::vector<std::string> args
    {
        "jar",
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

    for (const auto *source_set : source_sets)
    {
        auto classes_directory = source_set->Build / "classes";

        args.emplace_back("-C");
        args.push_back(classes_directory);
        args.emplace_back(".");

        auto resources_directory = source_set->Source / "resources";

        args.emplace_back("-C");
        args.push_back(resources_directory);
        args.emplace_back(".");

        if (include_sources)
        {
            auto sources_directory = source_set->Source / "kotlin";

            args.emplace_back("-C");
            args.push_back(sources_directory);
            args.emplace_back(".");
        }
    }

    std::string out, err;
    if (auto res = Process(std::move(args))(out, err); !res)
    {
        std::cout << out;
        std::cerr << err;
        return res;
    }

    return {};
}
