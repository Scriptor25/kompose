#pragma once

#include <project.hxx>

#include <toolkit/result.hxx>

namespace kompose
{
    [[nodiscard]] toolkit::result<std::vector<const Module *>> topological_sort(
        const std::unordered_set<const Module *> &modules);


    [[nodiscard]] toolkit::result<> compile(const Module &module, const SourceSet &source_set);
    [[nodiscard]] toolkit::result<> launch(const Module &module);
    [[nodiscard]] toolkit::result<> package(const Module &module);
}
