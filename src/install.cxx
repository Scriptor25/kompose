#include <filesystem>
#include <kompose.hxx>

#include <cstdlib>
#include <fstream>

toolkit::result<> kompose::install(
    const Project &project,
    const Module &module,
    const http::client &client)
{
    const auto binary_bundle_filename = module.Artifact.get_filename("jar");
    const auto sources_bundle_filename = module.Artifact.get_filename("sources", "jar");

    const auto src_binary_bundle_path = module.Build / binary_bundle_filename;
    const auto src_sources_bundle_path = module.Build / sources_bundle_filename;
    
    const auto *home = getenv("HOME");
    const auto repository = std::filesystem::path(home) / ".m2" / "repository" / module.Artifact.locate();

    if (!std::filesystem::exists(repository))
        if (std::error_code ec; std::filesystem::create_directories(repository, ec), ec)
            return toolkit::make_error(
                "failed to create directories '{}': {} ({})",
                repository.string(),
                ec.message(),
                ec.value());

    const auto dst_binary_bundle_path = repository / binary_bundle_filename;
    const auto dst_sources_bundle_path = repository / sources_bundle_filename;

    const auto pom_path = repository / module.Artifact.get_filename("pom");

    if (std::error_code ec; std::filesystem::copy_file(
                            src_binary_bundle_path,
                            dst_binary_bundle_path,
                            std::filesystem::copy_options::overwrite_existing,
                            ec), ec)
        return toolkit::make_error("failed to copy file from '{}' to '{}': {} ({})",
            src_binary_bundle_path.string(),
            dst_binary_bundle_path.string(),
            ec.message(),
            ec.value());
    if (std::error_code ec; std::filesystem::copy_file(
                            src_sources_bundle_path,
                            dst_sources_bundle_path,
                            std::filesystem::copy_options::overwrite_existing,
                            ec), ec)
        return toolkit::make_error("failed to copy file from '{}' to '{}': {} ({})",
            src_sources_bundle_path.string(),
            dst_sources_bundle_path.string(),
            ec.message(),
            ec.value());
    
    constexpr auto project_template = R"(<?xml version="1.0" encoding="UTF-8"?>
<project xmlns="http://maven.apache.org/POM/4.0.0"
         xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
         xsi:schemaLocation="http://maven.apache.org/POM/4.0.0 http://maven.apache.org/xsd/maven-4.0.0.xsd">
    <modelVersion>4.0.0</modelVersion>

    <groupId>{}</groupId>
    <artifactId>{}</artifactId>
    <version>{}</version>

    <packaging>jar</packaging>

    <dependencies>{}
    </dependencies>
</project>)";

    constexpr auto dependency_template = R"(
        <dependency>
            <groupId>{}</groupId>
            <artifactId>{}</artifactId>
            <version>{}</version>
            <scope>{}</scope>
        </dependency>)";

    std::string dependencies;
    for (const auto *source_set : module.IncludeInCompile())
    {
        for (const auto &dependency : source_set->Compile.Modules)
        {
            dependencies += std::format(
                dependency_template,
                dependency->Artifact.Group,
                dependency->Artifact.Artifact,
                dependency->Artifact.Version,
                "compile");
        }
        for (const auto &dependency : source_set->Compile.Maven)
        {
            dependencies += std::format(
                dependency_template,
                dependency.Group,
                dependency.Artifact,
                dependency.Version,
                "compile");
        }
        
        for (const auto &dependency : source_set->Runtime.Modules)
        {
            dependencies += std::format(
                dependency_template,
                dependency->Artifact.Group,
                dependency->Artifact.Artifact,
                dependency->Artifact.Version,
                "runtime");
        }
        for (const auto &dependency : source_set->Runtime.Maven)
        {
            dependencies += std::format(
                dependency_template,
                dependency.Group,
                dependency.Artifact,
                dependency.Version,
                "runtime");
        }
    }

    auto pom = std::format(
        project_template,
        module.Artifact.Group,
        module.Artifact.Artifact,
        module.Artifact.Version,
        dependencies);
    
    {
        std::ofstream stream(pom_path, std::ios::binary);
        if (!stream)
            return toolkit::make_error("failed to open file '{}'", pom_path.string());
        stream << pom;
    }

    return {};
}
