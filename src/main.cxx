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

static toolkit::result<kompose::ProjectGraph> build_project_graph(
    kompose::ProjectConfig project,
    std::vector<std::unique_ptr<kompose::ModuleConfig>> modules) {}

static toolkit::result<> run(int argc, const char *const *argv) {
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

    for (auto &entry : project.Dependencies.Maven) {
      mod->Dependencies.Maven.insert(entry);
    }

    for (auto &entry : project.CompileDependencies.Maven) {
      mod->CompileDependencies.Maven.insert(entry);
    }

    for (auto &entry : project.RuntimeDependencies.Maven) {
      mod->RuntimeDependencies.Maven.insert(entry);
    }

    for (auto &entry : project.TestDependencies.Maven) {
      mod->TestDependencies.Maven.insert(entry);
    }

    for (auto &entry : mod->Dependencies.Maven) {
      mod->CompileDependencies.Maven.insert(entry);
      mod->RuntimeDependencies.Maven.insert(entry);
      mod->TestDependencies.Maven.insert(entry);
    }

    modules.push_back(std::move(mod));
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
