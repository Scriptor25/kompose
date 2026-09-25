#include <config.hxx>
#include <kotlin.hxx>
#include <process.hxx>
#include <project.hxx>

#include <args/args.hxx>
#include <toml/toml.hxx>

#include <toolkit/result.hxx>

#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <kompose.hxx>
#include <memory>
#include <queue>
#include <ranges>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <json/json.hxx>

[[nodiscard]] static toolkit::result<std::unique_ptr<kompose::Project>> build_project_from_config(
    const std::filesystem::path &path,
    const kompose::ProjectConfig &project_config,
    const std::vector<kompose::ModuleConfig> &module_configs)
{
    auto project = std::make_unique<kompose::Project>(
        kompose::Project
        {
            .Name = *project_config.Name,
            .Path = path,
        }
    );

    for (const auto &module_config : module_configs)
    {
        auto &module = (*project)[*module_config.Name];

        const auto source = module_config.Root / "src";
        const auto build = path / "build" / *module_config.Name;

        module = {
            .Parent = project.get(),
            .Type = module_config.Type,
            .Name = *module_config.Name,
            .Source = source,
            .Build = build,
        };

        for (const auto &name : module_config.SourceSets | std::views::keys)
            module.SourceSets[name] = {
                .Parent = &module,
                .Name = name,
                .Source = source / name,
                .Build = build / name,
            };

        switch (module_config.Type)
        {
        case kompose::ModuleType::Application:
        {
            const auto &module_data = get<kompose::ApplicationModuleConfigData>(module_config.Data);

            kompose::ApplicationModuleData data
            {
                .Main = module_data.Main,
            };

            for (auto &include : module_data.Include)
            {
                auto &it = module[include];

                data.Include.insert(&it);
            }

            module.Data = std::move(data);
            break;
        }

        case kompose::ModuleType::Library:
        {
            const auto &module_data = get<kompose::LibraryModuleConfigData>(module_config.Data);

            kompose::LibraryModuleData data
            {
                .Package = module_data.Package,
                .IncludeSources = module_data.IncludeSources,
            };

            for (auto &include : module_data.Include)
            {
                auto &it = module[include];

                data.Include.insert(&it);
            }

            module.Data = std::move(data);
            break;
        }
        }
    }

    for (const auto &module_config : module_configs)
    {
        auto &module = (*project)[*module_config.Name];

        for (const auto &[name, source_set_config] : module_config.SourceSets)
        {
            auto &source_set = module[name];

            for (const auto &dependency : module_config.Dependencies.General.Modules)
            {
                source_set.Compile.Modules.insert(&(*project)[dependency]);
                source_set.Runtime.Modules.insert(&(*project)[dependency]);
            }
            for (const auto &dependency : module_config.Dependencies.General.Maven)
            {
                source_set.Compile.Maven.insert(dependency);
                source_set.Runtime.Maven.insert(dependency);
            }

            for (const auto &dependency : module_config.Dependencies.Compile.Modules)
                source_set.Compile.Modules.insert(&(*project)[dependency]);
            for (const auto &dependency : module_config.Dependencies.Compile.Maven)
                source_set.Compile.Maven.insert(dependency);

            for (const auto &dependency : module_config.Dependencies.Runtime.Modules)
                source_set.Runtime.Modules.insert(&(*project)[dependency]);
            for (const auto &dependency : module_config.Dependencies.Runtime.Maven)
                source_set.Runtime.Maven.insert(dependency);

            for (const auto &dependency : source_set_config.Dependencies.General.Modules)
            {
                source_set.Compile.Modules.insert(&(*project)[dependency]);
                source_set.Runtime.Modules.insert(&(*project)[dependency]);
            }
            for (const auto &dependency : source_set_config.Dependencies.General.Maven)
            {
                source_set.Compile.Maven.insert(dependency);
                source_set.Runtime.Maven.insert(dependency);
            }

            for (const auto &dependency : source_set_config.Dependencies.Compile.Modules)
                source_set.Compile.Modules.insert(&(*project)[dependency]);
            for (const auto &dependency : source_set_config.Dependencies.Compile.Maven)
                source_set.Compile.Maven.insert(dependency);

            for (const auto &dependency : source_set_config.Dependencies.Runtime.Modules)
                source_set.Runtime.Modules.insert(&(*project)[dependency]);
            for (const auto &dependency : source_set_config.Dependencies.Runtime.Maven)
                source_set.Runtime.Maven.insert(dependency);
        }
    }

    return project;
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
    std::cerr << "> :version" << std::endl;

    std::cout << "0.0.0" << std::endl;
}

static void task_help()
{
    std::cerr << "> :help" << std::endl;

    std::cout << "kompose [<option>...] <[module:]task>... [-- <argument>...]" << std::endl;

    std::cout << std::endl;
    std::cout << "options:" << std::endl;
    std::cout << " --project, -p <directory>    specify the project directory" << std::endl;
}

static void task_model(const kompose::Project &project)
{
    const json::node project_node(project);
    std::cout << std::setw(4) << project_node;
}

[[nodiscard]] static toolkit::result<> task_clean(const std::unordered_set<const kompose::Module *> &nodes)
{
    for (const auto *node : nodes)
    {
        std::cerr << "> " << node->Name << ":clean" << std::endl;

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

[[nodiscard]] static toolkit::result<> task_compile(const std::unordered_set<const kompose::Module *> &nodes)
{
    std::vector<const kompose::Module *> path;
    if (auto res = kompose::topological_sort(nodes) >> path; !res)
        return res;

    for (const auto *module : path)
    {
        std::cerr << "> " << module->Name << ":compile" << std::endl;

        for (const auto source_sets = module->IncludeInCompile();
             const auto *source_set : source_sets)
            if (auto res = kompose::compile(*module, *source_set); !res)
                return res;
    }

    return {};
}

[[nodiscard]] static toolkit::result<> task_launch(const std::unordered_set<const kompose::Module *> &nodes)
{
    for (const auto *node : nodes)
    {
        if (node->Type != kompose::ModuleType::Application)
            continue;

        std::cerr << "> " << node->Name << ":launch" << std::endl;

        if (auto res = kompose::launch(*node); !res)
            return res;
    }

    return {};
}

[[nodiscard]] static toolkit::result<> task_package(const std::unordered_set<const kompose::Module *> &nodes)
{
    for (const auto *node : nodes)
    {
        std::cerr << "> " << node->Name << ":package" << std::endl;

        if (auto res = kompose::package(*node); !res)
            return res;
    }

    return {};
}

static const args::manifest manifest
{
    { .id = "project", .kind = args::entry_kind::value, .patterns = { "--project", "-p" } },
};

// kompose [(--<option>|-<o>)...] <[module:]task>... [-- <argument>...]

[[nodiscard]] static toolkit::result<> run(int argc, const char *const *argv)
{
    args::context context;
    if (auto res = args::context::parse(manifest, { argv, static_cast<size_t>(argc) }) >> context; !res)
        return res;

    const auto project_directory = context.get("project");

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

    auto work = std::filesystem::weakly_canonical(
        project_directory
            ? std::filesystem::path(*project_directory)
            : std::filesystem::current_path());

    auto project_toml = work / "project.toml";

    if (!std::filesystem::exists(project_toml))
        return toolkit::make_error("project.toml does not exist");

    toml::node project_node;
    std::ifstream(project_toml) >> project_node;

    kompose::ProjectConfig project_config;
    if (!(project_node >> project_config))
        return toolkit::make_error("failed to parse project.toml");

    if (!project_config.Name)
        project_config.Name = work.filename();

    std::vector<kompose::ModuleConfig> module_configs;
    for (const auto &name : project_config.Modules.Include)
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

        kompose::ModuleConfig module_config;
        if (!(module_node >> module_config))
        {
            std::cerr << "skip module '" << name << "': failed to parse module.toml" << std::endl;
            continue;
        }

        module_config.Root = work / name;

        if (!module_config.Name)
            module_config.Name = name;

        if (!module_config.Artifact.Name)
            module_config.Artifact.Name = module_config.Name;

        if (!module_config.Artifact.Group)
            module_config.Artifact.Group = project_config.Artifact.Group;

        if (!module_config.Artifact.Version)
            module_config.Artifact.Version = project_config.Artifact.Version;

        for (const auto &entry : project_config.Repositories.Maven)
            module_config.Repositories.Maven.insert(entry);

        for (const auto &entry : project_config.Dependencies.General.Modules)
            module_config.Dependencies.General.Modules.insert(entry);

        for (const auto &entry : project_config.Dependencies.General.Maven)
            module_config.Dependencies.General.Maven.insert(entry);

        for (const auto &entry : project_config.Dependencies.Compile.Modules)
            module_config.Dependencies.Compile.Modules.insert(entry);

        for (const auto &entry : project_config.Dependencies.Compile.Maven)
            module_config.Dependencies.Compile.Maven.insert(entry);

        for (const auto &entry : project_config.Dependencies.Runtime.Modules)
            module_config.Dependencies.Runtime.Modules.insert(entry);

        for (const auto &entry : project_config.Dependencies.Runtime.Maven)
            module_config.Dependencies.Runtime.Maven.insert(entry);

        module_configs.push_back(std::move(module_config));
    }

    std::unique_ptr<kompose::Project> project;
    if (auto res = build_project_from_config(work, project_config, module_configs) >> project; !res)
        return res;

    // TODO: task cache, i.e. if already compiled and source files did not change, then do not compile again
    // TODO: same if already packaged and neither source files nor resources did change, then do not package again

    std::unordered_set<const kompose::Module *> clean, compile, launch, package;
    for (auto &[task, module_name] : tasks)
    {
        std::unordered_set<const kompose::Module *> modules;
        if (module_name)
        {
            auto it = project->find(std::string(*module_name));
            if (it == project->end())
                return toolkit::make_error("undefined module {}", *module_name);

            const auto &module = *it;

            modules.insert(&module);
        }
        else
        {
            for (const auto &module : *project)
                modules.insert(&module);
        }

        std::unordered_set<const kompose::Module *> modules_with_dependencies;

        std::queue<const kompose::Module *> queue;
        for (const auto *module : modules)
            queue.push(module);
        for (; !queue.empty(); queue.pop())
        {
            const auto *module = queue.front();
            modules_with_dependencies.insert(module);

            for (const auto source_sets = module->IncludeInCompile();
                 const auto *source_set : source_sets)
                for (const auto *dependency : source_set->Compile.Modules)
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

        if (task == "model")
        {
            task_model(*project);
            continue;
        }

        if (task == "clean")
        {
            for (const auto *module : modules)
                clean.insert(module);

            continue;
        }

        if (task == "compile")
        {
            for (const auto *module : modules_with_dependencies)
                compile.insert(module);

            continue;
        }

        if (task == "build")
        {
            for (const auto *module : modules_with_dependencies)
                compile.insert(module);

            continue;
        }

        if (task == "launch")
        {
            for (const auto *module : modules_with_dependencies)
                compile.insert(module);

            for (const auto *module : modules)
                launch.insert(module);

            continue;
        }

        if (task == "package")
        {
            for (const auto *module : modules_with_dependencies)
                compile.insert(module);

            for (const auto *module : modules)
                package.insert(module);

            continue;
        }

        return toolkit::make_error("undefined task {}:{}", module_name.value_or({}), task);
    }

    if (auto res = task_clean(clean); !res)
        return res;
    if (auto res = task_compile(compile); !res)
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
