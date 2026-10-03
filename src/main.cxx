#include <config.hxx>
#include <kompose.hxx>
#include <kotlin.hxx>
#include <project.hxx>

#include <args/args.hxx>

#include <json/json.hxx>
#include <toml/toml.hxx>

#include <http/client.hxx>

#include <toolkit/result.hxx>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <queue>
#include <ranges>
#include <string_view>
#include <unordered_set>
#include <utility>

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
            .Repositories = project_config.Repositories,
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
            .Path = module_config.Root,
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

namespace
{
    struct Task
    {
        std::string_view Name;
        std::optional<std::string_view> Module;
    };
}

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

[[nodiscard]] static toolkit::result<> task_clean(const std::unordered_set<const kompose::Module *> &modules)
{
    for (const auto *module : modules)
    {
        std::cerr << "> " << module->Name << ":clean" << std::endl;

        const auto &build = module->Build;

        if (std::error_code ec; std::filesystem::remove_all(build, ec), ec)
            return toolkit::make_error(
                "failed to remove directory '{}': {} ({})",
                build.string(),
                ec.message(),
                ec.value());
    }

    return {};
}

[[nodiscard]] static toolkit::result<> task_compile(
    const kompose::Project &project,
    const std::unordered_set<const kompose::Module *> &modules,
    const http::client &client)
{
    std::vector<const kompose::Module *> module_path;
    if (auto res = kompose::topological_sort(modules) >> module_path; !res)
        return res;

    for (const auto *module : module_path)
    {
        std::cerr << "> " << module->Name << ":compile" << std::endl;

        for (const auto source_sets = module->IncludeInCompile();
             const auto *source_set : source_sets)
            if (auto res = kompose::compile(project, *module, *source_set, client); !res)
                return res;
    }

    return {};
}

[[nodiscard]] static toolkit::result<> task_launch(
    const kompose::Project &project,
    const std::unordered_set<const kompose::Module *> &modules,
    const http::client &client,
    const std::vector<std::string_view> &program_args)
{
    for (const auto *module : modules)
    {
        if (module->Type != kompose::ModuleType::Application)
            continue;

        std::cerr << "> " << module->Name << ":launch" << std::endl;

        if (auto res = kompose::launch(project, *module, client, program_args); !res)
            return res;
    }

    return {};
}

[[nodiscard]] static toolkit::result<> task_package(
    const kompose::Project &project,
    const std::unordered_set<const kompose::Module *> &modules,
    const http::client &client,
    const bool fat)
{
    for (const auto *module : modules)
    {
        std::cerr << "> " << module->Name << ":package" << std::endl;

        if (auto res = kompose::package(project, *module, client, fat); !res)
            return res;
    }

    return {};
}

static const args::manifest manifest
{
    { .id = "project", .kind = args::entry_kind::value, .patterns = { "--project", "-p" } },
    { .id = "fat", .kind = args::entry_kind::flag, .patterns = { "--fat", "-f" } },
};

// kompose [(--<option>|-<o>)...] <[module:]task>... [-- <argument>...]

[[nodiscard]] static toolkit::result<> run(int argc, const char *const *argv)
{
    args::context context;
    if (auto res = args::context::parse(manifest, { argv, static_cast<size_t>(argc) }) >> context; !res)
        return res;

    const auto project_directory = context.get("project");
    const auto fat = context.is("fat");

    std::unordered_set<std::string_view> task_strings;
    const auto task_count = context.limited() ? context.limit() : context.size();
    for (size_t i = 0; i < task_count; ++i)
        task_strings.insert(context[i]);

    std::vector<std::string_view> program_args;
    for (auto i = task_count; i < context.size(); ++i)
        program_args.push_back(context[i]);

    std::vector<Task> tasks;
    for (const auto &task_string : task_strings)
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

    const auto project_path = std::filesystem::weakly_canonical(
        project_directory
            ? std::filesystem::path(*project_directory)
            : std::filesystem::current_path());

    const auto project_toml = project_path / "project.toml";
    if (!std::filesystem::exists(project_toml))
        return toolkit::make_error("project.toml does not exist");

    toml::node project_node;
    std::ifstream(project_toml) >> project_node;

    kompose::ProjectConfig project_config;
    if (!(project_node >> project_config))
        return toolkit::make_error("failed to parse project.toml");

    if (!project_config.Name)
        project_config.Name = project_path.filename();

    std::vector<kompose::ModuleConfig> module_configs;
    for (const auto &filename : project_config.Modules.Include)
    {
        std::filesystem::path module_path;
        std::string module_name;
        if (filename == ".")
        {
            module_path = project_path;
            module_name = "<root>";
        }
        else
        {
            module_path = project_path / filename;
            module_name = filename;
        }

        if (!std::filesystem::is_directory(module_path))
        {
            std::cerr << "skip module '" << module_name << "': not a directory" << std::endl;
            continue;
        }

        const auto module_toml = module_path / "module.toml";
        if (!std::filesystem::exists(module_toml))
        {
            std::cerr << "skip module '" << module_name << "': module.toml does not exist" << std::endl;
            continue;
        }

        toml::node module_node;
        std::ifstream(module_toml) >> module_node;

        kompose::ModuleConfig module_config;
        if (!(module_node >> module_config))
        {
            std::cerr << "skip module '" << module_name << "': failed to parse module.toml" << std::endl;
            continue;
        }

        module_config.Root = module_path;

        if (!module_config.Name)
            module_config.Name = module_name;

        if (!module_config.Artifact.Name)
            module_config.Artifact.Name = module_config.Name;

        if (!module_config.Artifact.Group)
            module_config.Artifact.Group = project_config.Artifact.Group;

        if (!module_config.Artifact.Version)
            module_config.Artifact.Version = project_config.Artifact.Version;

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
    if (auto res = build_project_from_config(project_path, project_config, module_configs) >> project; !res)
        return res;

    const auto transport = http::create_default_transport(true);
    http::client client(*transport);

    // TODO: task cache, i.e. if already compiled and source files did not change, then do not compile again
    // TODO: same if already packaged and neither source files nor resources did change, then do not package again

    std::unordered_set<const kompose::Module *> clean_modules, compile_modules, launch_modules, package_modules;
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
                clean_modules.insert(module);

            continue;
        }

        if (task == "compile")
        {
            for (const auto *module : modules_with_dependencies)
                compile_modules.insert(module);

            continue;
        }

        if (task == "build")
        {
            for (const auto *module : modules_with_dependencies)
                compile_modules.insert(module);

            continue;
        }

        if (task == "launch")
        {
            for (const auto *module : modules_with_dependencies)
                compile_modules.insert(module);

            for (const auto *module : modules)
                launch_modules.insert(module);

            continue;
        }

        if (task == "package")
        {
            for (const auto *module : modules_with_dependencies)
                compile_modules.insert(module);

            for (const auto *module : modules)
                package_modules.insert(module);

            continue;
        }

        return toolkit::make_error("undefined task {}:{}", module_name.value_or({}), task);
    }

    if (auto res = task_clean(clean_modules); !res)
        return res;
    if (auto res = task_compile(*project, compile_modules, client); !res)
        return res;
    if (auto res = task_launch(*project, launch_modules, client, program_args); !res)
        return res;
    if (auto res = task_package(*project, package_modules, client, fat); !res)
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
