#include <kompose.hxx>

#include <queue>

toolkit::result<std::vector<const kompose::Module *>> kompose::topological_sort(
    const std::unordered_set<const Module *> &modules)
{
    std::unordered_map<const Module *, size_t> in_degree;
    std::unordered_map<const Module *, std::unordered_set<const Module *>> dependents;

    for (const auto *module : modules)
        in_degree[module] = {};

    for (const auto *module : modules)
        for (const auto source_sets = module->IncludeInCompile();
             const auto *source_set : source_sets)
            for (const auto *dependency : source_set->Compile.Modules)
            {
                if (!modules.contains(dependency))
                    continue;

                ++in_degree[module];
                dependents[dependency].insert(module);
            }

    std::queue<const Module *> ready;
    for (const auto *module : modules)
        if (!in_degree[module])
            ready.push(module);

    std::vector<const Module *> path;
    path.reserve(modules.size());

    for (; !ready.empty(); ready.pop())
    {
        const auto *module = ready.front();
        path.push_back(module);

        for (const auto *dependent : dependents[module])
            if (!--in_degree[dependent])
                ready.push(dependent);
    }

    if (path.size() != modules.size())
        return toolkit::make_error("cyclic dependencies detected");

    return path;
}
