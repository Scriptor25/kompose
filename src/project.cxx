#include <project.hxx>

kompose::SourceSet &kompose::Module::operator[](const std::string &name)
{
    return SourceSets.at(name);
}

const kompose::SourceSet &kompose::Module::operator[](const std::string &name) const
{
    return SourceSets.at(name);
}

std::unordered_set<const kompose::SourceSet *> kompose::Module::IncludeInCompile() const
{
    switch (Type)
    {
    case ModuleType::Application:
    {
        const auto &data = get<ApplicationModuleData>(Data);
        return data.Include;
    }

    case ModuleType::Library:
    {
        const auto &data = get<LibraryModuleData>(Data);
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

kompose::Module &kompose::Project::operator[](const std::string &name)
{
    return Modules[name];
}

const kompose::Module &kompose::Project::operator[](const std::string &name) const
{
    return Modules.at(name);
}

kompose::Project::iterator kompose::Project::find(const std::string &name) const
{
    return { Modules.find(name) };
}

kompose::Project::iterator kompose::Project::begin() const
{
    return { Modules.begin() };
}

kompose::Project::iterator kompose::Project::end() const
{
    return { Modules.end() };
}
