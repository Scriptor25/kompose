#pragma once

#include <project.hxx>

#include <toolkit/result.hxx>

namespace kompose
{
    [[nodiscard]] toolkit::result<std::vector<const Module *>> topological_sort(
        const std::unordered_set<const Module *> &modules);


    [[nodiscard]] toolkit::result<> compile(
        const Project &project,
        const Module &module,
        const SourceSet &source_set,
        const http::client &client);
    [[nodiscard]] toolkit::result<> launch(
        const Project &project,
        const Module &module,
        const http::client &client,
        const std::vector<std::string_view> &program_args);
    [[nodiscard]] toolkit::result<> package(
        const Project &project,
        const Module &module,
        const http::client &client,
        bool fat);
}
