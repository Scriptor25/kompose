#include <kompose.hxx>
#include <process.hxx>

#include <iostream>
#include <queue>

toolkit::result<> kompose::launch(
    const Project &project,
    const Module &module,
    const http::client &client,
    const std::vector<std::string_view> &program_args)
{
    if (module.Type != ModuleType::Application)
        return toolkit::make_error("task '{}:launch' is not supported", module.Name);

    const auto &data = std::get<ApplicationModuleData>(module.Data);
    const auto &main_class = data.Main;

    std::unordered_set<const Module *> module_dependencies;
    std::unordered_set<MavenCoordinate> maven_dependencies;

    std::queue<const Module *> queue;
    queue.push(&module);
    for (; !queue.empty(); queue.pop())
    {
        const auto *mod = queue.front();
        module_dependencies.insert(mod);

        for (const auto *source_set : mod->IncludeInCompile())
        {
            for (const auto *dependency : source_set->Compile.Modules)
                queue.push(dependency);
            for (const auto *dependency : source_set->Runtime.Modules)
                queue.push(dependency);

            for (const auto &dependency : source_set->Compile.Maven)
                maven_dependencies.insert(dependency);
            for (const auto &dependency : source_set->Runtime.Maven)
                maven_dependencies.insert(dependency);
        }
    }

    std::unordered_set<std::filesystem::path> class_path;

    for (const auto *dependency : module_dependencies)
        for (const auto *source_set : dependency->IncludeInCompile())
        {
            auto classes_directory = source_set->Build / "classes";
            class_path.insert(classes_directory);

            auto resources_directory = source_set->Source / "resources";
            if (std::filesystem::exists(resources_directory))
                class_path.insert(resources_directory);
        }

    {
        std::unordered_set<std::filesystem::path> paths;
        if (auto res = resolve(
                           project,
                           client,
                           MavenResolveScope::Runtime,
                           maven_dependencies) >> paths; !res)
            return res;

        class_path.insert(paths.begin(), paths.end());
    }

    // TODO: instead of finding common platform, select included source sets by platform

    KotlinCompilerPlatform platform{};
    for (const auto *source_set : module.IncludeInCompile())
    {
        if (platform == KotlinCompilerPlatform::None)
        {
            platform = source_set->Compiler.Platform;
            continue;
        }

        if (platform != source_set->Compiler.Platform)
            return toolkit::make_error("source sets do not share the same compiler platform");
    }

    switch (platform)
    {
    case KotlinCompilerPlatform::Jvm:
    {
        std::optional<std::string> jdkHome;
        std::optional<bool> enablePreview;

        for (const auto *source_set : module.IncludeInCompile())
        {
            const auto &compiler_data = std::get<JvmCompilerConfigData>(source_set->Compiler.Data);

            if (compiler_data.JdkHome)
            {
                if (!jdkHome)
                    jdkHome = compiler_data.JdkHome;
                else if (jdkHome != compiler_data.JdkHome)
                    return toolkit::make_error("source sets do not share the same jdk home");
            }

            if (compiler_data.EnablePreview)
            {
                if (!enablePreview)
                    enablePreview = compiler_data.EnablePreview;
                else if (enablePreview != compiler_data.EnablePreview)
                    return toolkit::make_error("source sets do not share the same enable preview");
            }
        }

        auto java = jdkHome ? (std::filesystem::path(*jdkHome) / "bin" / "java").string() : "java";

        std::vector<std::string> args;
        args.emplace_back(java);

        if (!class_path.empty())
        {
            args.emplace_back("--class-path");

            std::string str;
            for (auto it = class_path.begin(); it != class_path.end(); ++it)
            {
                if (it != class_path.begin())
                    str += ':';
                str += *it;
            }
            args.push_back(std::move(str));
        }

        if (enablePreview && *enablePreview)
            args.emplace_back("--enable-preview");

        args.push_back(main_class);

        for (const auto &arg : program_args)
            args.emplace_back(arg);

        return Process(std::move(args))(std::cout, std::cerr);
    }
    case KotlinCompilerPlatform::Js:
        return toolkit::make_error("TODO: platform.js");
    case KotlinCompilerPlatform::Native:
        return toolkit::make_error("TODO: platform.native");
    case KotlinCompilerPlatform::None:
        return toolkit::make_error("TODO: platform.none");
    }

    throw std::runtime_error("dead code");
}
