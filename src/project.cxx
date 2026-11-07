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

const std::unordered_set<const kompose::SourceSet *> &kompose::Module::IncludeInCompile() const
{
    switch (Type)
    {
    case ModuleType::Application:
    {
        const auto &data = std::get<ApplicationModuleData>(Data);
        return data.Include;
    }

    case ModuleType::Library:
    {
        const auto &data = std::get<LibraryModuleData>(Data);
        return data.Include;
    }

    default:
        throw std::runtime_error("dead code");
    }
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

const kompose::Module *kompose::Project::byName(const std::string &name) const
{
    for (const auto &module : Modules | std::views::values)
        if (module.Name == name)
            return &module;
    return nullptr;
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
