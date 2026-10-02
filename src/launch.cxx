#include <kompose.hxx>
#include <process.hxx>

#include <iostream>
#include <queue>

toolkit::result<> kompose::launch(
    const Project &project,
    const Module &module,
    const std::vector<std::string_view> &program_args,
    const http::client &client)
{
    switch (module.Type)
    {
    case ModuleType::Application:
        break;

    default:
        return toolkit::make_error("node type does not support task :launch");
    }

    const auto &data = get<ApplicationModuleData>(module.Data);
    const auto &main_class = data.Main;

    std::unordered_set<const Module *> module_dependencies;
    std::unordered_set<MavenCoordinate> maven_dependencies;

    std::queue<const Module *> queue;
    queue.push(&module);
    for (; !queue.empty(); queue.pop())
    {
        const auto *next = queue.front();
        module_dependencies.insert(next);

        for (const auto source_sets = next->IncludeInCompile();
             const auto *source_set : source_sets)
        {
            for (const auto *dependency : source_set->Runtime.Modules)
                queue.push(dependency);

            for (const auto &dependency : source_set->Runtime.Maven)
                maven_dependencies.insert(dependency);
        }
    }

    std::unordered_set<std::filesystem::path> class_path;

    for (const auto *dependency : module_dependencies)
        for (const auto source_sets = dependency->IncludeInCompile();
             const auto *source_set : source_sets)
        {
            class_path.insert(source_set->Build / "classes");
            class_path.insert(source_set->Source / "resources");
        }

    {
        std::unordered_set<std::filesystem::path> paths;
        if (auto res = resolve(project, client, MavenResolveScope::Runtime, maven_dependencies) >> paths; !res)
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

    std::vector<std::string> args;
    args.emplace_back("java");
    args.emplace_back("--class-path");
    args.push_back(class_path_string);
    args.push_back(main_class);

    for (const auto &arg : program_args)
        args.emplace_back(arg);

    return Process(std::move(args))(std::cout, std::cerr);
}
