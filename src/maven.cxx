#include <maven.hxx>
#include <project.hxx>

#include <xml/xml.hxx>

#include <http/client.hxx>
#include <http/http.hxx>

#include <fstream>
#include <queue>

uint8_t kompose::operator|(const uint8_t a, MavenDependencyScope b)
{
    return a | static_cast<uint8_t>(b);
}

uint8_t kompose::operator|(MavenDependencyScope a, const uint8_t b)
{
    return static_cast<uint8_t>(a) | b;
}

uint8_t kompose::operator|(MavenDependencyScope a, MavenDependencyScope b)
{
    return static_cast<uint8_t>(a) | static_cast<uint8_t>(b);
}

uint8_t kompose::operator&(const uint8_t a, MavenDependencyScope b)
{
    return a & static_cast<uint8_t>(b);
}

uint8_t kompose::operator&(MavenDependencyScope a, const uint8_t b)
{
    return static_cast<uint8_t>(a) & b;
}

uint8_t kompose::operator&(MavenDependencyScope a, MavenDependencyScope b)
{
    return static_cast<uint8_t>(a) & static_cast<uint8_t>(b);
}

const kompose::MavenDocument *kompose::MavenDocument::GetParent(const MavenDocumentPool &pool) const
{
    if (!Parent)
        return nullptr;

    for (const auto &document : pool.Documents)
    {
        if (document.Group != Parent->Group)
            continue;
        if (document.Artifact != Parent->Group)
            continue;
        if (document.Version != Parent->Group)
            continue;
        return &document;
    }

    return nullptr;
}

std::unordered_set<const kompose::MavenDocument *> kompose::MavenDocument::GetDependencies(
    const MavenDocumentPool &pool,
    const uint8_t scopes) const
{
    std::unordered_set<const MavenDocument *> dependencies;

    for (const auto &dependency : Dependencies)
        if (dependency.Scope.value_or(MavenDependencyScope::Compile) & scopes)
            for (const auto &document : pool.Documents)
            {
                if (document.Group != dependency.Group)
                    continue;
                if (document.Artifact != dependency.Artifact)
                    continue;
                if (document.Version != dependency.Version)
                    continue;

                dependencies.insert(&document);
                break;
            }

    return dependencies;
}

std::unordered_set<const kompose::MavenDocument *> kompose::MavenDocument::GetDependencies(
    const MavenDocumentPool &pool,
    MavenDependencyScope scope) const
{
    return GetDependencies(pool, static_cast<uint8_t>(scope));
}

bool kompose::MavenCoordinate::operator==(const MavenCoordinate &other) const
{
    return Group == other.Group && Artifact == other.Artifact && Version == other.Version;
}

std::filesystem::path kompose::MavenCoordinate::locate() const
{
    std::filesystem::path path;

    for (const auto segments = toolkit::split(Group, '.');
         const auto &segment : segments)
        path /= segment;

    path /= Artifact;
    path /= Version;

    return path;
}

std::string kompose::MavenCoordinate::get_filename(const std::string &extension) const
{
    return Artifact + '-' + Version + '.' + extension;
}

std::string kompose::MavenCoordinate::get_filename(const std::string &classifier, const std::string &extension) const
{
    return Artifact + '-' + Version + classifier + '.' + extension;
}

toolkit::result<std::unordered_set<std::filesystem::path>> kompose::MavenCoordinate::resolve(
    const Project &project,
    const http::client &client,
    const MavenResolveScope scope) const
{
    // first things first i'ma resolve all pom files from the dependency chain
    // second things second i'ma resolve all required transitive jar files
    // you make me a believer, believer... oooooh

    MavenDocumentPool pool;
    {
        std::queue<MavenCoordinate> queue;
        queue.push(*this);

        for (; !queue.empty(); queue.pop())
        {
            const auto &coordinate = queue.front();

            MavenDocument document;
            if (auto res = coordinate.resolve_pom(project, client) >> document; !res)
                return res;

            pool.Documents.push_back(std::move(document));
            const auto &ref = pool.Documents.back();

            if (ref.Parent)
                queue.push(
                    {
                        .Group = ref.Parent->Group,
                        .Artifact = ref.Parent->Artifact,
                        .Version = ref.Parent->Version,
                    });

            for (const auto &dependency : ref.Dependencies)
            {
                if (dependency.Optional.value_or(false))
                    continue;

                std::string version;
                if (dependency.Version)
                    version = *dependency.Version;
                else
                    return toolkit::make_error("dependency management not yet implemented");

                queue.push(
                    {
                        .Group = dependency.Group,
                        .Artifact = dependency.Artifact,
                        .Version = std::move(version),
                    });
            }
        }
    }

    if (pool.Documents.empty())
        return toolkit::make_error("failed to resolve maven pom documents.");

    std::unordered_set<const MavenDocument *> chain;
    {
        std::queue<const MavenDocument *> queue;
        queue.push(&pool.Documents.front());

        MavenDependencyScope scopes;
        switch (scope)
        {
        case MavenResolveScope::Compile:
            scopes = MavenDependencyScope::Compile;
            break;
        case MavenResolveScope::Runtime:
            scopes = MavenDependencyScope::Runtime;
            break;
        }

        for (; !queue.empty(); queue.pop())
        {
            const auto *document = queue.front();
            chain.insert(document);

            // todo: propagate dependency scope
            for (const auto *dependency : document->GetDependencies(pool, scopes))
                queue.push(dependency);
        }
    }

    std::unordered_set<std::filesystem::path> paths;

    for (const auto *link : chain)
    {
        MavenCoordinate coordinate
        {
            .Group = link->Group,
            .Artifact = link->Artifact,
            .Version = link->Version,
        };

        auto path = coordinate.locate() / coordinate.get_filename("jar");

        const auto local_jar = project.Path / "repository" / path;

        paths.insert(local_jar);

        if (std::filesystem::exists(local_jar))
            continue;

        if (std::error_code ec; std::filesystem::create_directories(local_jar.parent_path(), ec), ec)
            return toolkit::make_error(
                "failed to create directory '{}': {} ({})",
                local_jar.parent_path().string(),
                ec.message(),
                ec.value());

        auto found = false;
        for (const auto &repository : project.Repositories.Maven)
        {
            if (repository == "local")
            {
                const auto *home = getenv("HOME");
                const auto remote_jar = std::filesystem::path(home) / ".m2" / "repository" / path;

                if (!std::filesystem::exists(remote_jar))
                    continue;

                if (std::error_code ec;
                    std::filesystem::copy_file(
                        remote_jar,
                        local_jar,
                        std::filesystem::copy_options::overwrite_existing,
                        ec), ec)
                    return toolkit::make_error(
                        "failed to copy file from '{}' to '{}': {} ({})",
                        remote_jar.string(),
                        local_jar.string(),
                        ec.message(),
                        ec.value());

                found = true;
                break;
            }

            const auto remote_jar = repository / path;

            std::ofstream jar_stream(local_jar, std::ios::binary);
            http::response_t jar_response
            {
                .body = &jar_stream,
            };

            if (auto res = client.fetch_with_redirects(
                {
                    .method = http::method::get,
                    .location = http::url::parse(remote_jar.string()),
                    .headers = {},
                    .body = nullptr,
                },
                jar_response); !res)
                return res;

            if (jar_response.code == http::status_code::not_found)
                continue;

            if (!http::is_success(jar_response.code))
                return toolkit::make_error(
                    "failed to get jar: {} ({})",
                    jar_response.message,
                    jar_response.code);

            found = true;
            break;
        }

        if (!found)
            return toolkit::make_error(
                "failed to resolve maven jar '{}:{}:{}'",
                Group,
                Artifact,
                Version);
    }

    return paths;
}

toolkit::result<kompose::MavenDocument> kompose::MavenCoordinate::resolve_pom(
    const Project &project,
    const http::client &client) const
{
    const auto path = locate() / get_filename("pom");

    const auto local_pom = project.Path / "repository" / path;

    if (!std::filesystem::exists(local_pom))
    {
        if (std::error_code ec; std::filesystem::create_directories(local_pom.parent_path(), ec), ec)
            return toolkit::make_error(
                "failed to create directory '{}': {} ({})",
                local_pom.parent_path().string(),
                ec.message(),
                ec.value());

        auto found = false;
        for (const auto &repository : project.Repositories.Maven)
        {
            const auto remote_pom = repository / path;

            std::ofstream pom_stream(local_pom, std::ios::binary);
            http::response_t pom_response
            {
                .body = &pom_stream,
            };

            if (auto res = client.fetch_with_redirects(
                {
                    .method = http::method::get,
                    .location = http::url::parse(remote_pom.string()),
                    .headers = {},
                    .body = nullptr,
                },
                pom_response); !res)
                return res;

            if (pom_response.code == http::status_code::not_found)
                continue;

            if (!http::is_success(pom_response.code))
                return toolkit::make_error(
                    "failed to get maven pom: {} ({})",
                    pom_response.message,
                    pom_response.code);

            found = true;
            break;
        }

        if (!found)
            return toolkit::make_error(
                "failed to resolve maven pom '{}:{}:{}'",
                Group,
                Artifact,
                Version);
    }

    std::ifstream stream(local_pom, std::ios::binary);

    xml::node node;
    stream >> node;

    if (!node)
        return toolkit::make_error("failed to parse maven document");

    MavenDocument document;
    if (!(node >> document))
        return toolkit::make_error("failed to parse maven document");

    return document;
}

bool data::serializer<xml::node, kompose::MavenDocument>::from_data(
    const xml::node &node,
    kompose::MavenDocument &value)
{
    if (!node.is<xml::element>())
        return false;

    return node.get<xml::element>() >> value;
}

bool data::serializer<xml::element, kompose::MavenDocument>::from_data(
    const xml::element &element,
    kompose::MavenDocument &value)
{
    if (element.tag != "project")
        return false;

    const auto *group_element = element.find("groupId");
    const auto *artifact_element = element.find("artifactId");
    const auto *version_element = element.find("version");

    if (!group_element || !artifact_element || !version_element)
        return false;

    value.Group = group_element->get_text();
    value.Artifact = artifact_element->get_text();
    value.Version = version_element->get_text();

    if (const auto *packaging_element = element.find("packaging"))
        value.Packaging = packaging_element->get_text();
    if (const auto *modules_element = element.find("modules"))
        for (const auto *module_element : modules_element->find_all("module"))
            value.Modules.push_back(module_element->get_text());
    if (const auto *parent_element = element.find("parent"))
    {
        kompose::MavenDocumentParent parent;
        if (!(*parent_element >> parent))
            return false;
        value.Parent = std::move(parent);
    }
    if (const auto *dependency_management_element = element.find("dependencyManagement"))
        if (const auto *dependencies_element = dependency_management_element->find("dependencies"))
            for (const auto *dependency_element : dependencies_element->find_all("dependency"))
            {
                kompose::MavenDocumentDependency dependency;
                if (!(*dependency_element >> dependency))
                    return false;
                value.DependencyManagement.push_back(std::move(dependency));
            }
    if (const auto *dependencies_element = element.find("dependencies"))
        for (const auto *dependency_element : dependencies_element->find_all("dependency"))
        {
            kompose::MavenDocumentDependency dependency;
            if (!(*dependency_element >> dependency))
                return false;
            value.Dependencies.push_back(std::move(dependency));
        }
    if (const auto *properties_element = element.find("properties"))
        for (const auto *property_element : properties_element->elements)
            value.Properties[property_element->tag] = property_element->get_text();

    return true;
}

bool data::serializer<xml::element, kompose::MavenDocumentParent>::from_data(
    const xml::element &element,
    kompose::MavenDocumentParent &value)
{
    if (element.tag != "parent")
        return false;

    const auto *group_element = element.find("groupId");
    const auto *artifact_element = element.find("artifactId");
    const auto *version_element = element.find("version");

    if (!group_element || !artifact_element || !version_element)
        return false;

    value.Group = group_element->get_text();
    value.Artifact = artifact_element->get_text();
    value.Version = version_element->get_text();

    return true;
}

bool data::serializer<xml::element, kompose::MavenDocumentDependency>::from_data(
    const xml::element &element,
    kompose::MavenDocumentDependency &value)
{
    if (element.tag != "dependency")
        return false;

    const auto *group_element = element.find("groupId");
    const auto *artifact_element = element.find("artifactId");
    const auto *version_element = element.find("version");
    const auto *classifier_element = element.find("classifier");
    const auto *scope_element = element.find("scope");
    const auto *system_path_element = element.find("systemPath");
    const auto *optional_element = element.find("optional");
    const auto *exclusions_element = element.find("exclusions");

    if (!group_element || !artifact_element)
        return false;

    value.Group = group_element->get_text();
    value.Artifact = artifact_element->get_text();

    if (version_element)
        value.Version = version_element->get_text();
    if (classifier_element)
        value.Classifier = classifier_element->get_text();
    if (scope_element)
    {
        kompose::MavenDependencyScope scope;
        if (!(*scope_element >> scope))
            return false;
        value.Scope = scope;
    }
    if (system_path_element)
        value.SystemPath = system_path_element->get_text();
    if (optional_element)
        value.Optional = optional_element->get_text() == "true";

    if (exclusions_element)
        for (const auto *exclusion_element : exclusions_element->find_all("exclusion"))
        {
            kompose::MavenDocumentDependencyExclusion exclusion;
            if (!(*exclusion_element >> exclusion))
                return false;
            value.Exclusions.push_back(std::move(exclusion));
        }

    return true;
}

bool data::serializer<xml::element, kompose::MavenDocumentDependencyExclusion>::from_data(
    const xml::element &element,
    kompose::MavenDocumentDependencyExclusion &value)
{
    if (element.tag != "exclusion")
        return false;

    const auto *group_element = element.find("groupId");
    const auto *artifact_element = element.find("artifactId");

    if (!group_element || !artifact_element)
        return false;

    value.Group = group_element->get_text();
    value.Artifact = artifact_element->get_text();

    return true;
}

bool data::serializer<xml::element, kompose::MavenDependencyScope>::from_data(
    const xml::element &element,
    kompose::MavenDependencyScope &value)
{
    static const std::unordered_map<std::string_view, kompose::MavenDependencyScope> map
    {
        { "compile", kompose::MavenDependencyScope::Compile },
        { "provided", kompose::MavenDependencyScope::Provided },
        { "runtime", kompose::MavenDependencyScope::Runtime },
        { "test", kompose::MavenDependencyScope::Test },
        { "system", kompose::MavenDependencyScope::System },
    };

    if (element.tag != "scope")
        return false;

    const auto text = element.get_text();

    if (const auto it = map.find(text); it != map.end())
    {
        value = it->second;
        return true;
    }

    return false;
}
