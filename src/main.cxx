#include <args/args.hxx>
#include <config.hxx>
#include <filesystem>
#include <fstream>
#include <graph.hxx>
#include <iostream>
#include <memory>
#include <string_view>
#include <toml/toml.hxx>
#include <toolkit/result.hxx>
#include <unordered_set>
#include <utility>

static const args::manifest manifest;

// kompose [(--<option>|-<o>)...] <[module:]task>... [-- <argument>...]

// tasks:
//  version
//  help
//  tasks
//  clean
//  compile
//  test    -> compile
//  package -> compile
//  build   -> package
//  install -> package

static const std::unordered_map<std::string_view,
                                std::unordered_set<std::string_view>>
    graph = {
        {"version", {}},
        {"help", {}},
        {"tasks", {}},
        {"clean", {}},
        {"compile", {}},
        {"test", {"compile"}},
        {"package", {"compile"}},
        {"build", {"package"}},
        {"install", {"package"}},
};

static std::unordered_map<std::string, kompose::SourceSet>
source_sets(const std::filesystem::path &src,
            const std::filesystem::path &build,
            const std::unordered_set<std::string> &names) {
  std::unordered_map<std::string, kompose::SourceSet> sets;
  for (auto &name : names) {
    sets.insert({
        name,
        {
            .Name = name,
            .Src = src / name,
            .Build = build / name,
        },
    });
  }
  return sets;
}

[[nodiscard]] static toolkit::result<kompose::Graph>
configure(const std::filesystem::path &path,
          const kompose::ProjectConfig &project,
          const std::vector<std::unique_ptr<kompose::ModuleConfig>> &modules) {

  std::vector<std::unique_ptr<kompose::Node>> nodes(modules.size());

  for (size_t i = 0; i < nodes.size(); ++i) {
    auto &node = nodes[i];
    auto &mod = modules[i];

    auto src = mod->Root / "src";
    auto build = path / "build" / *mod->Name;

    kompose::Node base_node = {
        .Name = *mod->Name,
        .Src = src,
        .Build = build,
        .SourceSets = source_sets(src, build, {"main", "test"}),
    };

    switch (mod->Type) {
    case kompose::ModuleType::Application: {
      auto &application_module =
          reinterpret_cast<const kompose::ApplicationModuleConfig &>(*mod);

      kompose::ApplicationNode application_node(base_node);
      application_node.Main = application_module.Main;

      node = std::make_unique<kompose::ApplicationNode>(application_node);
      break;
    }

    case kompose::ModuleType::Library: {
      auto &library_module =
          reinterpret_cast<const kompose::LibraryModuleConfig &>(*mod);

      kompose::LibraryNode library_node(base_node);

      node = std::make_unique<kompose::LibraryNode>(library_node);
      break;
    }
    }
  }

  kompose::Graph graph = {
      .Name = *project.Name,
      .Path = path,
      .Nodes = std::move(nodes),
  };

  return graph;
}

[[nodiscard]] static toolkit::result<> run(int argc, const char *const *argv) {
  auto work = std::filesystem::current_path();

  args::context context;
  if (auto res =
          args::context::parse(manifest, {argv, static_cast<size_t>(argc)}) >>
          context;
      !res) {
    return res;
  }

  std::unordered_set<std::string_view> tasks;

  size_t task_count = (context.limited() ? context.limit() : context.size());
  for (size_t i = 0; i < task_count; ++i) {
    tasks.insert(context[i]);
  }

  if (tasks.contains("version")) {
    std::cout << "0.0.0" << std::endl;
  }

  if (tasks.contains("help")) {
    std::cout << "kompose [<option>...] <[module:]task>... [-- <argument>...]"
              << std::endl;
  }

  auto project_toml = work / "project.toml";
  if (!std::filesystem::exists(project_toml)) {
    return toolkit::make_error("project.toml does not exist");
  }

  toml::node node;
  std::ifstream(project_toml) >> node;

  kompose::ProjectConfig project;
  if (!(node >> project)) {
    return toolkit::make_error("failed to parse project.toml");
  }

  if (!project.Name) {
    project.Name = work.filename();
  }

  std::vector<std::unique_ptr<kompose::ModuleConfig>> modules;
  for (auto name : project.Modules.Include) {
    if (!std::filesystem::is_directory(name)) {
      std::cerr << "skip module '" << name << "': not a directory" << std::endl;
      continue;
    }

    auto module_toml = work / name / "module.toml";
    if (!std::filesystem::exists(module_toml)) {
      std::cerr << "skip module '" << name << "': module.toml does not exist"
                << std::endl;
      continue;
    }

    toml::node node;
    std::ifstream(module_toml) >> node;

    std::unique_ptr<kompose::ModuleConfig> mod;
    if (!(node >> mod)) {
      std::cerr << "skip module '" << name << "': failed to parse module.toml"
                << std::endl;
      continue;
    }

    mod->Root = work / name;

    if (!mod->Name) {
      mod->Name = name;
    }

    if (!mod->Artifact.Name) {
      mod->Artifact.Name = mod->Name;
    }

    if (!mod->Artifact.Group) {
      mod->Artifact.Group = project.Artifact.Group;
    }

    if (!mod->Artifact.Version) {
      mod->Artifact.Version = project.Artifact.Version;
    }

    for (auto &entry : project.Repositories.Maven) {
      mod->Repositories.Maven.insert(entry);
    }

    for (auto &entry : project.Dependencies.Modules) {
      mod->Dependencies.Modules.insert(entry);
    }

    for (auto &entry : project.Dependencies.Maven) {
      mod->Dependencies.Maven.insert(entry);
    }

    for (auto &entry : project.CompileDependencies.Modules) {
      mod->CompileDependencies.Modules.insert(entry);
    }

    for (auto &entry : project.CompileDependencies.Maven) {
      mod->CompileDependencies.Maven.insert(entry);
    }

    for (auto &entry : project.RuntimeDependencies.Modules) {
      mod->RuntimeDependencies.Modules.insert(entry);
    }

    for (auto &entry : project.RuntimeDependencies.Maven) {
      mod->RuntimeDependencies.Maven.insert(entry);
    }

    for (auto &entry : project.TestDependencies.Modules) {
      mod->TestDependencies.Modules.insert(entry);
    }

    for (auto &entry : project.TestDependencies.Maven) {
      mod->TestDependencies.Maven.insert(entry);
    }

    for (auto &entry : mod->Dependencies.Modules) {
      mod->CompileDependencies.Modules.insert(entry);
      mod->RuntimeDependencies.Modules.insert(entry);
      mod->TestDependencies.Modules.insert(entry);
    }

    for (auto &entry : mod->Dependencies.Maven) {
      mod->CompileDependencies.Maven.insert(entry);
      mod->RuntimeDependencies.Maven.insert(entry);
      mod->TestDependencies.Maven.insert(entry);
    }

    modules.push_back(std::move(mod));
  }

  kompose::Graph graph;
  if (auto res = configure(work, project, modules) >> graph; !res) {
    return res;
  }

  return {};
}

int main(int argc, const char *const *argv) {

  if (auto res = run(argc, argv); !res) {
    std::cerr << res.error() << std::endl;
    return 1;
  }

  return 0;
}
