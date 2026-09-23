#include <config.hxx>
#include <graph.hxx>
#include <kotlin.hxx>

#include <args/args.hxx>
#include <toml/toml.hxx>

#include <toolkit/result.hxx>

#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <process.hxx>
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
    for (auto &name : names)
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

[[nodiscard]] static toolkit::result<kompose::Graph> configure(
    const std::filesystem::path &path,
    const kompose::ProjectConfig &project,
    const std::vector<std::unique_ptr<kompose::ModuleConfig>> &modules)
{
    std::unordered_map<std::string, std::unique_ptr<kompose::Node>> nodes(modules.size());

    for (auto &mod : modules)
    {
        std::cerr << "task " << *mod->Name << ":configure" << std::endl;

        auto &node = nodes[*mod->Name];

        auto src = mod->Root / "src";
        auto build = path / "build" / *mod->Name;

        kompose::Node base_node
        {
            .Name = *mod->Name,
            .Src = src,
            .Build = build,
            .SourceSets = source_sets(
                src,
                build,
                { "main" }
            ),
        };

        switch (mod->Type)
        {
        case kompose::ModuleType::Application:
        {
            auto &application_module = reinterpret_cast<const kompose::ApplicationModuleConfig &>(*mod);

            kompose::ApplicationNode application_node(base_node);
            application_node.Type = kompose::NodeType::Application;
            application_node.Main = application_module.Main;

            node = std::make_unique<kompose::ApplicationNode>(application_node);
            break;
        }

        case kompose::ModuleType::Library:
        {
            auto &library_module = reinterpret_cast<const kompose::LibraryModuleConfig &>(*mod);

            kompose::LibraryNode library_node(base_node);
            library_node.Type = kompose::NodeType::Library;

            node = std::make_unique<kompose::LibraryNode>(library_node);
            break;
        }
        }
    }

    for (auto &mod : modules)
    {
        auto &node = nodes[*mod->Name];

        std::unordered_set<const kompose::Node *> module_dependencies;
        std::unordered_set<std::string> maven_dependencies;

        for (auto &dependency : mod->Dependencies.Modules)
            module_dependencies.insert(nodes[dependency].get());
        for (auto &dependency : mod->Dependencies.Maven)
            maven_dependencies.insert(dependency);

        for (auto &dependency : mod->CompileDependencies.Modules)
            module_dependencies.insert(nodes[dependency].get());
        for (auto &dependency : mod->CompileDependencies.Maven)
            maven_dependencies.insert(dependency);

        for (auto &set : node->SourceSets | std::views::values)
        {
            set.ModuleDependencies = module_dependencies;
            set.MavenDependencies = maven_dependencies;
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

static std::vector<const kompose::Node *> build_compile_path(std::unordered_set<const kompose::Node *> nodes)
{
}

[[nodiscard]] static toolkit::result<> compile(
    const kompose::Node &node,
    const kompose::SourceSet &set)
{
    std::cerr << "task " << node.Name << ":compile" << std::endl;

    std::unordered_set<std::string> sources;
    for (auto &entry : std::filesystem::recursive_directory_iterator(set.Src / "kotlin"))
    {
        if (entry.is_directory())
            continue;

        auto &path = entry.path();
        if (!path.has_extension() || path.extension() != ".kt")
            continue;

        sources.insert(path);
    }

    const auto destination = set.Build / "classes";

    std::filesystem::create_directories(destination);

    std::unordered_set<std::string> class_path;
    for (const auto *dependency : set.ModuleDependencies)
    {
        auto &source_set = dependency->SourceSets.at("main");
        class_path.insert(source_set.Build / "classes");
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
    std::cerr << "task " << node.Name << ":launch" << std::endl;

    switch (node.Type)
    {
    case kompose::NodeType::Application:
        break;

    default:
        return toolkit::make_error("node type does not support task :launch");
    }

    const auto &application_node = reinterpret_cast<const kompose::ApplicationNode &>(node);
    auto &main_class = application_node.Main;

    std::unordered_set<const kompose::Node *> module_dependencies;
    std::unordered_set<std::string> maven_dependencies;

    std::queue<const kompose::Node *> queue;
    queue.push(&node);
    for (; !queue.empty(); queue.pop())
    {
        const auto *next = queue.front();
        auto &set = next->SourceSets.at("main");

        module_dependencies.insert(next);

        for (const auto *dependency : set.ModuleDependencies)
            queue.push(dependency);

        for (auto &dependency : set.MavenDependencies)
            maven_dependencies.insert(dependency);
    }

    std::vector<std::string> class_path;

    for (auto *dependency : module_dependencies)
        class_path.emplace_back(dependency->SourceSets.at("main").Build / "classes");

    for (auto &dependency : maven_dependencies)
    {
        // TODO: resolve maven dependency, add to class path
    }

    class_path.emplace_back("/home/felix/.local/opt/kotlinc/lib/kotlin-stdlib.jar");
    // TODO: locate kotlin compiler home, $KOTLIN_HOME/lib/...

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
//  tasks
//  clean
//  compile
//  resources
//  build   -> compile + resources
//  launch  -> build
//  package -> build

static const std::unordered_map<std::string_view, std::unordered_set<std::string_view>> task_graph
{
    { "version", {} },
    { "help", {} },
    { "tasks", {} },
    { "clean", {} },
    { "compile", {} },
    { "resources", {} },
    { "build", { "compile", "resources" } },
    { "launch", { "build" } },
    { "package", { "build" } },
};

/**
 * kompose version
 */
[[nodiscard]] static toolkit::result<> task_version(
    const kompose::Graph &graph,
    const std::optional<std::string_view> &node_name)
{
    std::cerr << "task :version" << std::endl;

    std::cout << "0.0.0" << std::endl;
    return {};
}

/**
 * kompose help
 */
[[nodiscard]] static toolkit::result<> task_help(
    const kompose::Graph &graph,
    const std::optional<std::string_view> &node_name)
{
    std::cerr << "task :help" << std::endl;

    std::cout << "kompose [<option>...] <[module:]task>... [-- <argument>...]" << std::endl;
    return {};
}

/**
 * kompose [<module>:]tasks
 */
[[nodiscard]] static toolkit::result<> task_tasks(
    const kompose::Graph &graph,
    const std::optional<std::string_view> &node_name)
{
    std::cerr << "task " << node_name.value_or({}) << ":tasks" << std::endl;

    if (node_name)
    {
        auto &node = graph[std::string(*node_name)];

        std::cout
                << std::format(
                    "{0}:tasks {0}:clean {0}:compile {0}:resources {0}:build {0}:launch {0}:package",
                    *node_name)
                << std::endl;
    }
    else
    {
        std::cout << "version help tasks clean compile resources build launch package" << std::endl;
    }

    return {};
}

/**
 * kompose [<module>:]clean
 */
[[nodiscard]] static toolkit::result<> task_clean(
    const kompose::Graph &graph,
    const std::optional<std::string_view> &node_name)
{
    std::cerr << "task " << node_name.value_or({}) << ":clean" << std::endl;

    std::filesystem::path build;

    if (node_name)
    {
        auto &node = graph[std::string(*node_name)];

        build = node.Build;
    }
    else
    {
        build = graph.Path / "build";
    }

    if (std::error_code ec; std::filesystem::remove_all(build, ec), ec)
        return toolkit::make_error(
            "failed to remove directory '{}': {} ({})",
            build.string(),
            ec.message(),
            ec.value());

    return {};
}

/**
 * kompose [<module>:]compile
 */
[[nodiscard]] static toolkit::result<> task_compile(
    const kompose::Graph &graph,
    const std::optional<std::string_view> &node_name)
{
    std::cerr << "task " << node_name.value_or({}) << ":compile" << std::endl;

    std::vector<const kompose::Node *> path;

    if (node_name)
    {
        const auto &node = graph[std::string(*node_name)];

        path = build_compile_path({ &node });
    }
    else
    {
        std::unordered_set<const kompose::Node *> nodes;
        for (const auto &node : graph)
            nodes.insert(&node);

        path = build_compile_path(nodes);
    }

    // TODO: compile module dependencies

    for (const auto *entry : path)
    {
        auto &node = *entry;
        if (auto res = compile(node, node["main"]); !res)
            return res;
    }

    return {};
}

/**
 * kompose [<module>:]resources
 */
[[nodiscard]] static toolkit::result<> task_resources(
    const kompose::Graph &graph,
    const std::optional<std::string_view> &node_name)
{
    std::cerr << "task " << node_name.value_or({}) << ":resources" << std::endl;

    if (node_name)
    {
        auto &node = graph[std::string(*node_name)];

        // TODO: copy the resources of the specified module
    }
    else
    {
        // TODO: copy all resources
    }

    std::unordered_set<std::string> resources;
    for (auto &entry : std::filesystem::recursive_directory_iterator(src_set / "resources"))
        if (!entry.is_directory())
            resources.insert(entry.path());

    return {};
}

/**
 * kompose [<module>:]build
 */
[[nodiscard]] static toolkit::result<> task_build(
    const kompose::Graph &graph,
    const std::optional<std::string_view> &node_name)
{
    if (auto res = task_compile(graph, node_name); !res)
        return res;
    if (auto res = task_resources(graph, node_name); !res)
        return res;

    std::cerr << "task " << node_name.value_or({}) << ":build" << std::endl;

    if (node_name)
    {
        auto &node = graph[std::string(*node_name)];

        // TODO: do something with node?
    }
    else
    {
        // TODO: do something?
    }

    return {};
}

/**
 * kompose [<module>:]launch
 */
[[nodiscard]] static toolkit::result<> task_launch(
    const kompose::Graph &graph,
    const std::optional<std::string_view> &node_name)
{
    if (auto res = task_build(graph, node_name); !res)
        return res;

    std::cerr << "task " << node_name.value_or({}) << ":launch" << std::endl;

    const kompose::Node *app_node{};
    if (node_name)
    {
        auto &node = graph[std::string(*node_name)];

        app_node = &node;
    }
    else
    {
        for (auto &node : graph)
            if (node.Type == kompose::NodeType::Application)
            {
                if (app_node)
                    return toolkit::make_error("project graph contains more than one application module");

                app_node = &node;
            }

        if (!app_node)
            return toolkit::make_error("project graph does not contain any application modules");
    }

    return launch(*app_node);
}

/**
 * kompose [<module>:]package
 */
[[nodiscard]] static toolkit::result<> task_package(
    const kompose::Graph &graph,
    const std::optional<std::string_view> &node_name)
{
    if (auto res = task_build(graph, node_name); !res)
        return res;

    std::cerr << "task " << node_name.value_or({}) << ":package" << std::endl;

    if (node_name)
    {
        auto &node = graph[std::string(*node_name)];

        // TODO: build and package the specified module
    }
    else
    {
        // TODO: build and package all modules
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

    std::vector<std::pair<std::string_view, std::optional<std::string_view>>> tasks;
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
    if (auto res = configure(work, project, modules) >> graph; !res)
        return res;

    // TODO: build task graph, so that tasks are executed in right order and no duplicates happen

    for (auto &[task, target] : tasks)
    {
        static const std::unordered_map<std::string_view, std::function<toolkit::result<>(
            const kompose::Graph &,
            const std::optional<std::string_view> &)>> mapping
        {
            { "version", task_version },
            { "help", task_help },
            { "tasks", task_tasks },
            { "clean", task_clean },
            { "compile", task_compile },
            { "resources", task_resources },
            { "build", task_build },
            { "launch", task_launch },
            { "package", task_package },
        };

        toolkit::result<> res;
        if (auto it = mapping.find(task); it != mapping.end())
            res = it->second(graph, target);
        else
            res = toolkit::make_error(
                "undefined task {}:{}",
                target.value_or({}),
                task);

        if (!res)
            return res;
    }

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
