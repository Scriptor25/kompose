#include <maven.hxx>
#include <project.hxx>

#include <xml/xml.hxx>

#include <http/client.hxx>
#include <http/http.hxx>

#include <fstream>
#include <queue>

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
    const http::client &client) const
{
    // first things first i'ma resolve all pom files from the dependency chain
    // second things second i'ma resolve all required transitive jar files
    // you make me a believer, believer... oooooh

    MavenDocumentPool pool;
    if (auto res = resolve_pom(pool, project, client); !res)
        return res;

    if (!pool.Begin)
        return toolkit::make_error("failed to resolve maven pom documents.");

    std::queue<const MavenDocument *> queue;
    queue.push(pool.Begin);

    std::unordered_set<const MavenDocument *> chain;
    for (; !queue.empty(); queue.pop())
    {
        // TODO: skip dependencies that are not currently targeted (compile vs runtime)

        const auto *document = queue.front();
        chain.insert(document);

        for (const auto *dependency : document->Dependencies)
            queue.push(dependency);
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

toolkit::result<> kompose::MavenCoordinate::resolve_pom(
    MavenDocumentPool &pool,
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
                    "failed to get pom: {} ({})",
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
        return toolkit::make_error("failed to parse maven pom document");

    auto document = std::make_unique<MavenDocument>();

    // check if node is element type
    // check if node has tag 'project'

    if (!node.is<xml::element>())
        return toolkit::make_error("invalid pom root node");

    const auto &element = node.get<xml::element>();
    if (element.tag != "project")
        return toolkit::make_error("invalid pom root element");

    for (const auto &child : element.elements)
    {
        if (!child.is<xml::element>())
            continue;

        const auto &child_element = child.get<xml::element>();

        if (child_element.tag == "groupId")
        {
            document->Group = child_element.get_text();
            continue;
        }
        if (child_element.tag == "artifactId")
        {
            document->Artifact = child_element.get_text();
            continue;
        }
        if (child_element.tag == "version")
        {
            document->Version = child_element.get_text();
            continue;
        }
    }

    // TODO: resolve parent and dependency documents
    document->Parent;
    document->Dependencies;

    if (!pool.Begin)
        pool.Begin = document.get();

    pool.Documents.push_back(std::move(document));
    return {};
}
