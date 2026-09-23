#include <kotlin.hxx>
#include <process.hxx>

std::vector<std::string> kompose::KotlinCommand::Build() const
{
    std::vector<std::string> args;
    args.emplace_back("kotlinc");

#pragma region Common

    if (ApiVersion)
    {
        args.emplace_back("-api-version");
        args.push_back(*ApiVersion);
    }
    if (KotlinHome)
    {
        args.emplace_back("-kotlin-home");
        args.push_back(*KotlinHome);
    }
    if (LanguageVersion)
    {
        args.emplace_back("-language-version");
        args.push_back(*LanguageVersion);
    }
    for (auto &entry : OptIn)
    {
        args.emplace_back("-opt-in");
        args.push_back(entry);
    }
    for (auto &[fst, snd] : PluginOptions)
    {
        args.emplace_back("-P");
        args.push_back(fst + '=' + snd);
    }
    if (Progressive)
        args.emplace_back("-progressive");
    if (Script)
        args.emplace_back("-script");
    if (Verbose)
        args.emplace_back("-verbose");
    if (AllowContractsOnMoreFunctions)
        args.emplace_back("-Xallow-contracts-on-more-functions");
    if (AllowConditionImpliesReturnsContracts)
        args.emplace_back("-Xallow-condition-implies-returns-contracts");
    if (AllowHoldsinContract)
        args.emplace_back("-Xallow-holdsin-contract");
    if (AllowReturnsResultOf)
        args.emplace_back("-Xallow-returns-result-of");
    if (AllowReifiedTypeInCatch)
        args.emplace_back("-Xallow-reified-type-in-catch");
    if (CollectionLiterals)
        args.emplace_back("-Xcollection-literals");
    for (auto &[fst, snd] : CompilerPluginOrder)
    {
        args.emplace_back("-X-compiler-plugin-order");
        args.push_back(fst + '>' + snd);
    }
    if (DataFlowBasedExhaustiveness)
        args.emplace_back("-Xdata-flow-based-exhaustiveness");
    if (ExplicitContextArguments)
        args.emplace_back("-Xexplicit-context-arguments");
    switch (KLibIrInliner)
    {
    case KotlinKLibIrInliner::None:
        break;
    case KotlinKLibIrInliner::Disabled:
        args.emplace_back("-Xklib-ir-inliner");
        args.emplace_back("disabled");
        break;
    case KotlinKLibIrInliner::Full:
        args.emplace_back("-Xklib-ir-inliner");
        args.emplace_back("full");
        break;
    }
    if (IntrinsicConstEvaluation)
        args.emplace_back("-Xintrinsic-const-evaluation");
    switch (NameBasedDestructuring)
    {
    case KotlinNameBasedDestructuring::None:
        break;
    case KotlinNameBasedDestructuring::OnlySyntax:
        args.emplace_back("-Xname-based-destructuring");
        args.emplace_back("only-syntax");
        break;
    case KotlinNameBasedDestructuring::NameMismatch:
        args.emplace_back("-Xname-based-destructuring");
        args.emplace_back("name-mismatch");
        break;
    case KotlinNameBasedDestructuring::Complete:
        args.emplace_back("-Xname-based-destructuring");
        args.emplace_back("complete");
        break;
    }
    switch (ReturnValueChecker)
    {
    case KotlinReturnValueChecker::None:
        break;
    case KotlinReturnValueChecker::Disable:
        args.emplace_back("-Xreturn-value-checker");
        args.emplace_back("disable");
        break;
    case KotlinReturnValueChecker::Check:
        args.emplace_back("-Xreturn-value-checker");
        args.emplace_back("check");
        break;
    case KotlinReturnValueChecker::Full:
        args.emplace_back("-Xreturn-value-checker");
        args.emplace_back("full");
        break;
    }
    if (NoWarn)
        args.emplace_back("-nowarn");
    if (WError)
        args.emplace_back("-Werror");
    if (WExtra)
        args.emplace_back("-Wextra");
    if (RenderInternalDiagnosticNames)
        args.emplace_back("-Xrender-internal-diagnostic-names");
    for (auto &[fst, snd] : WarningLevel)
    {
        std::string name;
        switch (snd)
        {
        case KotlinWarningLevel::Error:
            name = "error";
            break;
        case KotlinWarningLevel::Warning:
            name = "warning";
            break;
        case KotlinWarningLevel::Disabled:
            name = "disabled";
            break;
        }
        args.emplace_back("-Xwarning-level");
        args.push_back(fst + ':' + name);
    }

#pragma endregion

#pragma region JVM

    if (!JvmClassPath.empty())
    {
        std::string classpath;
        for (auto it = JvmClassPath.begin(); it != JvmClassPath.end(); ++it)
        {
            if (it != JvmClassPath.begin())
                classpath += ':';
            classpath += *it;
        }
        args.emplace_back("-classpath");
        args.push_back(classpath);
    }
    if (JvmDestination)
    {
        args.emplace_back("-d");
        args.push_back(*JvmDestination);
    }
    if (JvmIncludeRuntime)
        args.emplace_back("-include-runtime");
    if (JvmJdkHomePath)
    {
        args.emplace_back("-jdk-home-path");
        args.push_back(*JvmJdkHomePath);
    }
    if (JvmJdkRelease)
    {
        args.emplace_back("-jdk-release");
        args.push_back(*JvmJdkRelease);
    }
    switch (JvmDefaultMode)
    {
    case KotlinJvmDefaultMode::None:
        break;
    case KotlinJvmDefaultMode::Enable:
        args.emplace_back("-jvm-default-mode");
        args.emplace_back("enable");
        break;
    case KotlinJvmDefaultMode::NoCompatibility:
        args.emplace_back("-jvm-default-mode");
        args.emplace_back("no-compatibility");
        break;
    case KotlinJvmDefaultMode::Disable:
        args.emplace_back("-jvm-default-mode");
        args.emplace_back("disable");
        break;
    }
    if (JvmTargetVersion)
    {
        args.emplace_back("-jvm-target-version");
        args.push_back(*JvmTargetVersion);
    }
    if (JvmJavaParameters)
        args.emplace_back("-java-parameters");
    if (JvmModuleName)
    {
        args.emplace_back("-module-name");
        args.push_back(*JvmModuleName);
    }
    if (JvmNoJdk)
        args.emplace_back("-no-jdk");
    if (JvmNoReflect)
        args.emplace_back("-no-reflect");
    if (JvmNoStdlib)
        args.emplace_back("-no-stdlib");
    if (!JvmScriptTemplates.empty())
    {
        std::string templates;
        for (auto it = JvmScriptTemplates.begin(); it != JvmScriptTemplates.end(); ++it)
        {
            if (it != JvmScriptTemplates.begin())
                templates += ':'; // TODO
            templates += *it;
        }
        args.emplace_back("-script-templates");
        args.push_back(templates);
    }
    if (JvmExposeBoxed)
        args.emplace_back("-Xjvm-expose-boxed");
    for (auto &[fst, snd] : JvmNullabilityAnnotations)
    {
        std::string level;
        switch (snd)
        {
        case KotlinJvmNullabilityAnnotationReportLevel::Ignore:
            level = "ignore";
            break;
        case KotlinJvmNullabilityAnnotationReportLevel::Warn:
            level = "warn";
            break;
        case KotlinJvmNullabilityAnnotationReportLevel::Strict:
            level = "strict";
            break;
        }
        args.emplace_back("-Xnullability-annotations");
        args.push_back('@' + fst + ':' + level);
    }

#pragma endregion

#pragma region JS

    if (!JsLibraries.empty())
    {
        std::string path;
        for (auto it = JsLibraries.begin(); it != JsLibraries.end(); ++it)
        {
            if (it != JsLibraries.begin())
                path += ':'; // TODO
            path += *it;
        }
        args.emplace_back("-libraries");
        args.push_back(path);
    }
    switch (JsMain)
    {
    case KotlinJsMain::None:
        break;
    case KotlinJsMain::Call:
        args.emplace_back("-main");
        args.emplace_back("call");
        break;
    case KotlinJsMain::NoCall:
        args.emplace_back("-main");
        args.emplace_back("noCall");
        break;
    }
    if (JsMetaInfo)
        args.emplace_back("-meta-info");
    switch (JsModuleKind)
    {
    case KotlinJsModuleKind::None:
        break;
    case KotlinJsModuleKind::Umd:
        args.emplace_back("-module-kind");
        args.emplace_back("umd");
        break;
    case KotlinJsModuleKind::CommonJs:
        args.emplace_back("-module-kind");
        args.emplace_back("commonjs");
        break;
    case KotlinJsModuleKind::Amd:
        args.emplace_back("-module-kind");
        args.emplace_back("amd");
        break;
    case KotlinJsModuleKind::Plain:
        args.emplace_back("-module-kind");
        args.emplace_back("plain");
        break;
    }
    if (JsNoStdlib)
        args.emplace_back("-no-stdlib");
    if (JsOutput)
    {
        args.emplace_back("-output");
        args.push_back(*JsOutput);
    }
    if (JsOutputPostfix)
    {
        args.emplace_back("-output-postfix");
        args.push_back(*JsOutputPostfix);
    }
    if (JsOutputPrefix)
    {
        args.emplace_back("-output-prefix");
        args.push_back(*JsOutputPrefix);
    }
    if (JsSourceMap)
        args.emplace_back("-source-map");
    if (!JsSourceMapBaseDirs.empty())
    {
        std::string dirs;
        for (auto it = JsSourceMapBaseDirs.begin(); it != JsSourceMapBaseDirs.end(); ++it)
        {
            if (it != JsSourceMapBaseDirs.begin())
                dirs += ':'; // TODO
            dirs += *it;
        }
        args.emplace_back("-source-map-base-dirs");
        args.push_back(dirs);
    }
    switch (JsSourceMapEmbedSources)
    {
    case KotlinJsSourceMapEmbedSources::None:
        break;
    case KotlinJsSourceMapEmbedSources::Always:
        args.emplace_back("-source-map-embed-sources");
        args.emplace_back("always");
        break;
    case KotlinJsSourceMapEmbedSources::Never:
        args.emplace_back("-source-map-embed-sources");
        args.emplace_back("never");
        break;
    case KotlinJsSourceMapEmbedSources::Inlining:
        args.emplace_back("-source-map-embed-sources");
        args.emplace_back("inlining");
        break;
    }
    switch (JsSourceMapNamesPolicy)
    {
    case KotlinJsSourceMapNamesPolicy::None:
        break;
    case KotlinJsSourceMapNamesPolicy::SimpleNames:
        args.emplace_back("-source-map-names-policy");
        args.emplace_back("simple-names");
        break;
    case KotlinJsSourceMapNamesPolicy::FullyQualifiedNames:
        args.emplace_back("-source-map-names-policy");
        args.emplace_back("fully-qualified-names");
        break;
    case KotlinJsSourceMapNamesPolicy::No:
        args.emplace_back("-source-map-names-policy");
        args.emplace_back("no");
        break;
    }
    if (JsSourceMapPrefix)
    {
        args.emplace_back("-source-map-prefix");
        args.push_back(*JsSourceMapPrefix);
    }
    if (JsTarget)
    {
        args.emplace_back("-target");
        args.push_back(*JsTarget);
    }
    if (JsEnableImplementingInterfacesFromTypeScript)
        args.emplace_back("-Xenable-implementing-interfaces-from-typescript");
    if (JsEsLongAsBigint)
        args.emplace_back("-Xes-long-as-bigint");

#pragma endregion

#pragma region Native

    // TODO

#pragma endregion

    for (auto &input : Input)
        args.push_back(input);

    return args;
}

toolkit::result<> kompose::KotlinCommand::operator()(std::string &out, std::string &err) const
{
    auto args = Build();

    return Process(std::move(args))(out, err);
}
