#include <project.hxx>
#include <ranges>

kompose::SourceSet &kompose::Module::operator[](const std::string &name)
{
    return SourceSets.at(name);
}

const kompose::SourceSet &kompose::Module::operator[](const std::string &name) const
{
    return SourceSets.at(name);
}

std::unordered_set<const kompose::SourceSet *> kompose::Module::GetSourceSets(const bool test) const
{
    std::unordered_set<const SourceSet *> source_sets;
    for (const auto &source_set : SourceSets | std::views::values)
        if (test || !source_set.Test)
            source_sets.insert(&source_set);
    return source_sets;
}

bool kompose::Project::iterator::operator==(const iterator &other) const
{
    return base == other.base;
}

kompose::Project::iterator &kompose::Project::iterator::operator++()
{
    ++base;
    return *this;
}

const kompose::Module &kompose::Project::iterator::operator*() const
{
    return base->second;
}

kompose::Module &kompose::Project::operator[](const std::string &id)
{
    return Modules[id];
}

const kompose::Module &kompose::Project::operator[](const std::string &id) const
{
    return Modules.at(id);
}

kompose::Project::iterator kompose::Project::find(const std::string &id) const
{
    return { Modules.find(id) };
}

kompose::Project::iterator kompose::Project::begin() const
{
    return { Modules.begin() };
}

kompose::Project::iterator kompose::Project::end() const
{
    return { Modules.end() };
}
