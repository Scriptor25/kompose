#include <graph.hxx>

const kompose::SourceSet &kompose::Node::operator[](const std::string &name) const
{
    return SourceSets.at(name);
}

bool kompose::Graph::iterator::operator==(const iterator &other) const
{
    return base == other.base;
}

kompose::Graph::iterator &kompose::Graph::iterator::operator++()
{
    ++base;
    return *this;
}

const kompose::Node &kompose::Graph::iterator::operator*() const
{
    return *base->second;
}

const kompose::Node &kompose::Graph::operator[](const std::string &name) const
{
    return *Nodes.at(name);
}

kompose::Graph::iterator kompose::Graph::find(const std::string &name) const
{
    return { Nodes.find(name) };
}

kompose::Graph::iterator kompose::Graph::begin() const
{
    return { Nodes.begin() };
}

kompose::Graph::iterator kompose::Graph::end() const
{
    return { Nodes.end() };
}
