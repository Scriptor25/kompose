#include <project.hxx>

kompose::SourceSet &kompose::Module::operator[](const std::string &name)
{
    return SourceSets.at(name);
}

const kompose::SourceSet &kompose::Module::operator[](const std::string &name) const
{
    return SourceSets.at(name);
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
