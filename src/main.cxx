#include <config.hxx>
#include <graph.hxx>
#include <kotlin.hxx>
#include <process.hxx>

#include <args/args.hxx>
#include <toml/toml.hxx>

#include <toolkit/result.hxx>

#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <queue>
#include <ranges>
#include <string_view>
#include <unordered_set>
#include <utility>

[[nodiscard]] static std::unordered_map<std::string, kompose::SourceSet> source_sets(
    const std::filesystem::path &src,
    const std::filesystem::path &build,
    const std::unordered_set<std::string> &names)
{
    std::unordered_map<std::string, kompose::SourceSet> sets;
    for (const auto &name : names)
        sets.insert(
            {
                name,
                {
                    .Name = name,
                    .Src = src / name,
                    .Build = build / name,
                },
            });
    return sets;
}

[[nodiscard]] static toolkit::result<kompose::Graph> build_graph(
    const std::filesystem::path &path,
    const kompose::ProjectConfig &project,
    const std::vector<std::unique_ptr<kompose::ModuleConfig>> &modules)
{
    std::unordered_map<std::string, std::unique_ptr<kompose::Node>> nodes(modules.size());

    for (const auto &mod : modules)
    {
        auto &node = nodes[*mod->Name];

        const auto src = mod->Root / "src";
        const auto build = path / "build" / *mod->Name;

        kompose::Node base_node
        {
            .Name = *mod->Name,
            .Src = src,
            .Build = build,
            .SourceSets = source_sets(
                src,
                build,
                { "main" }),
        };

        switch (mod->Type)
        {
        case kompose::ModuleType::Application:
        {
            const auto &application_module = reinterpret_cast<const kompose::ApplicationModuleConfig &>(*mod);

            kompose::ApplicationNode application_node(base_node);
            application_node.Type = kompose::NodeType::Application;
            application_node.Main = application_module.Main;

            node = std::make_unique<kompose::ApplicationNode>(application_node);
            break;
        }

        case kompose::ModuleType::Library:
        {
            const auto &library_module = reinterpret_cast<const kompose::LibraryModuleConfig &>(*mod);

            kompose::LibraryNode library_node(base_node);
            library_node.Type = kompose::NodeType::Library;

            node = std::make_unique<kompose::LibraryNode>(library_node);
            break;
        }
        }
    }

    for (const auto &mod : modules)
    {
        auto &node = nodes[*mod->Name];

        std::unordered_set<const kompose::Node *> module_dependencies;
        std::unordered_set<std::string> maven_dependencies;

        for (const auto &dependency : mod->Dependencies.Modules)
            module_dependencies.insert(nodes[dependency].get());
        for (const auto &dependency : mod->Dependencies.Maven)
            maven_dependencies.insert(dependency);

        for (const auto &dependency : mod->CompileDependencies.Modules)
            module_dependencies.insert(nodes[dependency].get());
        for (const auto &dependency : mod->CompileDependencies.Maven)
            maven_dependencies.insert(dependency);

        for (auto &source_set : node->SourceSets | std::views::values)
        {
            source_set.ModuleDependencies = module_dependencies;
            source_set.MavenDependencies = maven_dependencies;
        }
    }

    kompose::Graph graph
    {
        .Name = *project.Name,
        .Path = path,
        .Nodes = std::move(nodes),
    };

    return graph;
}

[[nodiscard]] static toolkit::result<std::vector<const kompose::Node *>> topological_sort(
    const std::unordered_set<const kompose::Node *> &nodes)
{
    std::unordered_map<const kompose::Node *, size_t> in_degree;
    std::unordered_map<const kompose::Node *, std::unordered_set<const kompose::Node *>> dependents;

    for (const auto *node : nodes)
        in_degree[node] = {};

    for (const auto *node : nodes)
        for (const auto *dependency : (*node)["main"].ModuleDependencies)
        {
            if (!nodes.contains(dependency))
                continue;

            ++in_degree[node];
            dependents[dependency].insert(node);
        }

    std::queue<const kompose::Node *> ready;
    for (const auto *node : nodes)
        if (!in_degree[node])
            ready.push(node);

    std::vector<const kompose::Node *> path;
    path.reserve(nodes.size());

    for (; !ready.empty(); ready.pop())
    {
        const auto *node = ready.front();
        path.push_back(node);

        for (const auto *dependent : dependents[node])
            if (!--in_degree[dependent])
                ready.push(dependent);
    }

    if (path.size() != nodes.size())
        return toolkit::make_error("cyclic dependencies detected");

    return path;
}

[[nodiscard]] static toolkit::result<> compile(
    const kompose::Node &node,
    const kompose::SourceSet &source_set)
{
    std::unordered_set<std::string> sources;
    for (const auto &entry : std::filesystem::recursive_directory_iterator(source_set.Src / "kotlin"))
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
    for (const auto *dependency : source_set.ModuleDependencies)
    {
        const auto &dependency_source_set = dependency->SourceSets.at("main");
        class_path.insert(dependency_source_set.Build / "classes");
    }

    kompose::KotlinCommand command
    {
        .Input = std::move(sources),
        .JvmClassPath = std::move(class_path),
        .JvmDestination = destination.string(),
        .JvmModuleName = node.Name,
    };

    switch (node.Type)
    {
    case kompose::NodeType::Application:
        command.JvmIncludeRuntime = true;
        break;

    case kompose::NodeType::Library:
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

[[nodiscard]] static toolkit::result<> launch(const kompose::Node &node)
{
    switch (node.Type)
    {
    case kompose::NodeType::Application:
        break;

    default:
        return toolkit::make_error("node type does not support task :launch");
    }

    const auto &application_node = reinterpret_cast<const kompose::ApplicationNode &>(node);
    const auto &main_class = application_node.Main;

    std::unordered_set<const kompose::Node *> module_dependencies;
    std::unordered_set<std::string> maven_dependencies;

    std::queue<const kompose::Node *> queue;
    queue.push(&node);
    for (; !queue.empty(); queue.pop())
    {
        const auto *next = queue.front();
        const auto &source_set = next->SourceSets.at("main");

        module_dependencies.insert(next);

        for (const auto *dependency : source_set.ModuleDependencies)
            queue.push(dependency);

        for (const auto &dependency : source_set.MavenDependencies)
            maven_dependencies.insert(dependency);
    }

    std::vector<std::string> class_path;

    for (const auto *dependency : module_dependencies)
    {
        auto &source_set = dependency->SourceSets.at("main");
        class_path.emplace_back(source_set.Build / "classes");
    }

    for (const auto &dependency : maven_dependencies)
    {
        // TODO: resolve maven dependency, add to class path
    }

    const auto *kotlin_home = getenv("KOTLIN_HOME");
    if (!kotlin_home)
        return toolkit::make_error("missing KOTLIN_HOME environment variable");

    class_path.emplace_back(std::filesystem::path(kotlin_home) / "lib" / "kotlin-stdlib.jar");

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

    std::string out, err;
    auto res = kompose::Process(std::move(args))(out, err);

    std::cout << out;
    std::cerr << err;

    return res;
}

// tasks:
//  version
//  help
//  clean
//  compile
//  resources
//  build   -> compile + resources
//  launch  -> build
//  package -> build

struct Task
{
    std::string_view Name;
    std::optional<std::string_view> Module;
};

static void task_version()
{
    std::cerr << "task :version" << std::endl;

    std::cout << "0.0.0" << std::endl;
}

static void task_help()
{
    std::cerr << "task :help" << std::endl;

    std::cout << "kompose [<option>...] <[module:]task>... [-- <argument>...]" << std::endl;
}

[[nodiscard]] static toolkit::result<> task_clean(const std::unordered_set<const kompose::Node *> &nodes)
{
    for (const auto *node : nodes)
    {
        std::cerr << "task " << node->Name << ":clean" << std::endl;

        const auto &build = node->Build;

        if (std::error_code ec; std::filesystem::remove_all(build, ec), ec)
            return toolkit::make_error(
                "failed to remove directory '{}': {} ({})",
                build.string(),
                ec.message(),
                ec.value());
    }

    return {};
}

[[nodiscard]] static toolkit::result<> task_compile(const std::unordered_set<const kompose::Node *> &nodes)
{
    std::vector<const kompose::Node *> path;
    if (auto res = topological_sort(nodes) >> path; !res)
        return res;

    for (const auto *node : path)
    {
        std::cerr << "task " << node->Name << ":compile" << std::endl;

        const auto &source_set = (*node)["main"];

        if (auto res = compile(*node, source_set); !res)
            return res;
    }

    return {};
}

[[nodiscard]] static toolkit::result<> task_resources(const std::unordered_set<const kompose::Node *> &nodes)
{
    for (const auto *node : nodes)
    {
        std::cerr << "task " << node->Name << ":resources" << std::endl;

        const auto &source_set = (*node)["main"];

        for (const auto &entry : std::filesystem::recursive_directory_iterator(source_set.Src / "resources"))
        {
            if (entry.is_directory())
                continue;

            const auto &from = entry.path();
            const auto to = source_set.Build / "classes" / std::filesystem::relative(entry.path(), source_set.Src);

            if (std::error_code ec; std::filesystem::create_directories(to.parent_path(), ec), ec)
                return toolkit::make_error(
                    "failed to create directory '{}': {} ({})",
                    to.parent_path().string(),
                    ec.message(),
                    ec.value());

            if (std::error_code ec;
                std::filesystem::copy_file(
                    from,
                    to,
                    std::filesystem::copy_options::overwrite_existing,
                    ec), ec)
                return toolkit::make_error(
                    "failed to copy file from '{}' to '{}': {} ({})",
                    from.string(),
                    to.string(),
                    ec.message(),
                    ec.value());
        }
    }

    return {};
}

[[nodiscard]] static toolkit::result<> task_launch(const std::unordered_set<const kompose::Node *> &nodes)
{
    for (const auto *node : nodes)
    {
        if (node->Type != kompose::NodeType::Application)
            continue;

        std::cerr << "task " << node->Name << ":launch" << std::endl;

        if (auto res = launch(*node); !res)
            return res;
    }

    return {};
}

[[nodiscard]] static toolkit::result<> task_package(const std::unordered_set<const kompose::Node *> &nodes)
{
    for (const auto *node : nodes)
    {
        std::cerr << "task " << node->Name << ":package" << std::endl;

        // TODO: package module as jar
    }

    return {};
}

static const args::manifest manifest;

// kompose [(--<option>|-<o>)...] <[module:]task>... [-- <argument>...]

[[nodiscard]] static toolkit::result<> run(int argc, const char *const *argv)
{
    auto work = std::filesystem::current_path();

    args::context context;
    if (auto res = args::context::parse(manifest, { argv, static_cast<size_t>(argc) }) >> context; !res)
        return res;

    std::unordered_set<std::string_view> task_strings;
    auto task_count = context.limited() ? context.limit() : context.size();
    for (size_t i = 0; i < task_count; ++i)
        task_strings.insert(context[i]);

    std::vector<Task> tasks;
    for (auto &task_string : task_strings)
    {
        auto pos = task_string.find(':');
        if (pos == std::string::npos)
        {
            tasks.emplace_back(task_string, std::nullopt);
            continue;
        }

        if (pos == 0)
        {
            tasks.emplace_back(task_string.substr(1), std::nullopt);
            continue;
        }

        auto name = task_string.substr(0, pos);
        auto task = task_string.substr(pos + 1);

        tasks.emplace_back(task, name);
    }

    auto project_toml = work / "project.toml";
    if (!std::filesystem::exists(project_toml))
        return toolkit::make_error("project.toml does not exist");

    toml::node project_node;
    std::ifstream(project_toml) >> project_node;

    kompose::ProjectConfig project;
    if (!(project_node >> project))
        return toolkit::make_error("failed to parse project.toml");

    if (!project.Name)
        project.Name = work.filename();

    std::vector<std::unique_ptr<kompose::ModuleConfig>> modules;
    for (auto &name : project.Modules.Include)
    {
        if (!std::filesystem::is_directory(name))
        {
            std::cerr << "skip module '" << name << "': not a directory" << std::endl;
            continue;
        }

        auto module_toml = work / name / "module.toml";
        if (!std::filesystem::exists(module_toml))
        {
            std::cerr << "skip module '" << name << "': module.toml does not exist" << std::endl;
            continue;
        }

        toml::node module_node;
        std::ifstream(module_toml) >> module_node;

        std::unique_ptr<kompose::ModuleConfig> mod;
        if (!(module_node >> mod))
        {
            std::cerr << "skip module '" << name << "': failed to parse module.toml" << std::endl;
            continue;
        }

        mod->Root = work / name;

        if (!mod->Name)
            mod->Name = name;

        if (!mod->Artifact.Name)
            mod->Artifact.Name = mod->Name;

        if (!mod->Artifact.Group)
            mod->Artifact.Group = project.Artifact.Group;

        if (!mod->Artifact.Version)
            mod->Artifact.Version = project.Artifact.Version;

        for (auto &entry : project.Repositories.Maven)
            mod->Repositories.Maven.insert(entry);

        for (auto &entry : project.Dependencies.Modules)
            mod->Dependencies.Modules.insert(entry);

        for (auto &entry : project.Dependencies.Maven)
            mod->Dependencies.Maven.insert(entry);

        for (auto &entry : project.CompileDependencies.Modules)
            mod->CompileDependencies.Modules.insert(entry);

        for (auto &entry : project.CompileDependencies.Maven)
            mod->CompileDependencies.Maven.insert(entry);

        for (auto &entry : project.RuntimeDependencies.Modules)
            mod->RuntimeDependencies.Modules.insert(entry);

        for (auto &entry : project.RuntimeDependencies.Maven)
            mod->RuntimeDependencies.Maven.insert(entry);

        modules.push_back(std::move(mod));
    }

    kompose::Graph graph;
    if (auto res = build_graph(work, project, modules) >> graph; !res)
        return res;

    std::unordered_set<const kompose::Node *> clean, compile, resources, launch, package;
    for (auto &[task, module] : tasks)
    {
        std::unordered_set<const kompose::Node *> nodes;
        if (module)
        {
            auto it = graph.find(std::string(*module));
            if (it == graph.end())
                return toolkit::make_error("undefined module {}", *module);

            const auto &node = *it;

            nodes.insert(&node);
        }
        else
        {
            for (const auto &node : graph)
                nodes.insert(&node);
        }

        std::unordered_set<const kompose::Node *> nodes_with_dependencies;

        std::queue<const kompose::Node *> queue;
        for (const auto *node : nodes)
            queue.push(node);
        for (; !queue.empty(); queue.pop())
        {
            const auto *node = queue.front();
            nodes_with_dependencies.insert(node);

            const auto &source_set = (*node)["main"];
            for (const auto *dependency : source_set.ModuleDependencies)
                queue.push(dependency);
        }

        if (task == "version")
        {
            task_version();
            continue;
        }

        if (task == "help")
        {
            task_help();
            continue;
        }

        if (task == "clean")
        {
            for (const auto *node : nodes)
                clean.insert(node);

            continue;
        }

        if (task == "compile")
        {
            for (const auto *node : nodes_with_dependencies)
                compile.insert(node);

            continue;
        }

        if (task == "resources")
        {
            for (const auto *node : nodes_with_dependencies)
                resources.insert(node);

            continue;
        }

        if (task == "build")
        {
            for (const auto *node : nodes_with_dependencies)
            {
                compile.insert(node);
                resources.insert(node);
            }

            continue;
        }

        if (task == "launch")
        {
            for (const auto *node : nodes_with_dependencies)
            {
                compile.insert(node);
                resources.insert(node);
            }

            for (const auto *node : nodes)
                launch.insert(node);

            continue;
        }

        if (task == "package")
        {
            for (const auto *node : nodes_with_dependencies)
            {
                compile.insert(node);
                resources.insert(node);
            }

            for (const auto *node : nodes)
                package.insert(node);

            continue;
        }

        return toolkit::make_error("undefined task {}:{}", module.value_or({}), task);
    }

    if (auto res = task_clean(clean); !res)
        return res;
    if (auto res = task_compile(compile); !res)
        return res;
    if (auto res = task_resources(resources); !res)
        return res;
    if (auto res = task_launch(launch); !res)
        return res;
    if (auto res = task_package(package); !res)
        return res;

    return {};
}

int main(const int argc, const char *const *argv)
{
    if (auto res = run(argc, argv); !res)
    {
        std::cerr << res.error() << std::endl;
        return 1;
    }

    return 0;
}
