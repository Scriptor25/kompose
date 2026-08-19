#include <kotlin.hxx>

std::vector<std::string> kompose::KotlinCommand::operator()() const
{
    std::vector<std::string> command;
    command.emplace_back("kotlinc");

#pragma region Common

    if (ApiVersion)
    {
        command.emplace_back("-api-version");
        command.push_back(*ApiVersion);
    }
    if (KotlinHome)
    {
        command.emplace_back("-kotlin-home");
        command.push_back(*KotlinHome);
    }
    if (LanguageVersion)
    {
        command.emplace_back("-language-version");
        command.push_back(*LanguageVersion);
    }
    for (auto& entry : OptIn)
    {
        command.emplace_back("-opt-in");
        command.push_back(entry);
    }
    for (auto& [fst, snd] : PluginOptions)
    {
        command.emplace_back("-P");
        command.push_back(fst + '=' + snd);
    }
    if (Progressive)
        command.emplace_back("-progressive");
    if (Script)
        command.emplace_back("-script");
    if (Verbose)
        command.emplace_back("-verbose");
    if (AllowContractsOnMoreFunctions)
        command.emplace_back("-Xallow-contracts-on-more-functions");
    if (AllowConditionImpliesReturnsContracts)
        command.emplace_back("-Xallow-condition-implies-returns-contracts");
    if (AllowHoldsinContract)
        command.emplace_back("-Xallow-holdsin-contract");
    if (AllowReturnsResultOf)
        command.emplace_back("-Xallow-returns-result-of");
    if (AllowReifiedTypeInCatch)
        command.emplace_back("-Xallow-reified-type-in-catch");
    if (CollectionLiterals)
        command.emplace_back("-Xcollection-literals");
    for (auto& [fst, snd] : CompilerPluginOrder)
    {
        command.emplace_back("-X-compiler-plugin-order");
        command.push_back(fst + '>' + snd);
    }
    if (DataFlowBasedExhaustiveness)
        command.emplace_back("-Xdata-flow-based-exhaustiveness");
    if (ExplicitContextArguments)
        command.emplace_back("-Xexplicit-context-arguments");
    switch (KLibIrInliner)
    {
    case KotlinKLibIrInliner::None:
        break;
    case KotlinKLibIrInliner::Disabled:
        command.emplace_back("-Xklib-ir-inliner");
        command.emplace_back("disabled");
        break;
    case KotlinKLibIrInliner::Full:
        command.emplace_back("-Xklib-ir-inliner");
        command.emplace_back("full");
        break;
    }
    if (IntrinsicConstEvaluation)
        command.emplace_back("-Xintrinsic-const-evaluation");
    switch (NameBasedDestructuring)
    {
    case KotlinNameBasedDestructuring::None:
        break;
    case KotlinNameBasedDestructuring::OnlySyntax:
        command.emplace_back("-Xname-based-destructuring");
        command.emplace_back("only-syntax");
        break;
    case KotlinNameBasedDestructuring::NameMismatch:
        command.emplace_back("-Xname-based-destructuring");
        command.emplace_back("name-mismatch");
        break;
    case KotlinNameBasedDestructuring::Complete:
        command.emplace_back("-Xname-based-destructuring");
        command.emplace_back("complete");
        break;
    }
    switch (ReturnValueChecker)
    {
    case KotlinReturnValueChecker::None:
        break;
    case KotlinReturnValueChecker::Disable:
        command.emplace_back("-Xreturn-value-checker");
        command.emplace_back("disable");
        break;
    case KotlinReturnValueChecker::Check:
        command.emplace_back("-Xreturn-value-checker");
        command.emplace_back("check");
        break;
    case KotlinReturnValueChecker::Full:
        command.emplace_back("-Xreturn-value-checker");
        command.emplace_back("full");
        break;
    }
    if (NoWarn)
        command.emplace_back("-nowarn");
    if (WError)
        command.emplace_back("-Werror");
    if (WExtra)
        command.emplace_back("-Wextra");
    if (RenderInternalDiagnosticNames)
        command.emplace_back("-Xrender-internal-diagnostic-names");
    for (auto& [fst, snd] : WarningLevel)
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
        command.emplace_back("-Xwarning-level");
        command.push_back(fst + ':' + name);
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
        command.emplace_back("-classpath");
        command.push_back(classpath);
    }
    if (JvmDestination)
    {
        command.emplace_back("-d");
        command.push_back(*JvmDestination);
    }
    if (JvmIncludeRuntime)
        command.emplace_back("-include-runtime");
    if (JvmJdkHomePath)
    {
        command.emplace_back("-jdk-home-path");
        command.push_back(*JvmJdkHomePath);
    }
    if (JvmJdkRelease)
    {
        command.emplace_back("-jdk-release");
        command.push_back(*JvmJdkRelease);
    }
    switch (JvmDefaultMode)
    {
    case KotlinJvmDefaultMode::None:
        break;
    case KotlinJvmDefaultMode::Enable:
        command.emplace_back("-jvm-default-mode");
        command.emplace_back("enable");
        break;
    case KotlinJvmDefaultMode::NoCompatibility:
        command.emplace_back("-jvm-default-mode");
        command.emplace_back("no-compatibility");
        break;
    case KotlinJvmDefaultMode::Disable:
        command.emplace_back("-jvm-default-mode");
        command.emplace_back("disable");
        break;
    }
    if (JvmTargetVersion)
    {
        command.emplace_back("-jvm-target-version");
        command.push_back(*JvmTargetVersion);
    }
    if (JvmJavaParameters)
        command.emplace_back("-java-parameters");
    if (JvmModuleName)
    {
        command.emplace_back("-module-name");
        command.push_back(*JvmModuleName);
    }
    if (JvmNoJdk)
        command.emplace_back("-no-jdk");
    if (JvmNoReflect)
        command.emplace_back("-no-reflect");
    if (JvmNoStdlib)
        command.emplace_back("-no-stdlib");
    if (!JvmScriptTemplates.empty())
    {
        std::string templates;
        for (auto it = JvmScriptTemplates.begin(); it != JvmScriptTemplates.end(); ++it)
        {
            if (it != JvmScriptTemplates.begin())
                templates += ',';
            templates += *it;
        }
        command.emplace_back("-script-templates");
        command.push_back(templates);
    }
    if (JvmExposeBoxed)
        command.emplace_back("-Xjvm-expose-boxed");
    for (auto& [fst, snd] : JvmNullabilityAnnotations)
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
        command.emplace_back("-Xnullability-annotations");
        command.push_back('@' + fst + ':' + level);
    }

#pragma endregion

#pragma region JS

    if (!JsLibraries.empty())
    {
        std::string path;
        for (auto it = JsLibraries.begin(); it != JsLibraries.end(); ++it)
        {
            if (it != JsLibraries.begin())
                path += ','; // TODO
            path += *it;
        }
        command.emplace_back("-libraries");
        command.push_back(path);
    }
    switch (JsMain)
    {
    case KotlinJsMain::None:
        break;
    case KotlinJsMain::Call:
        command.emplace_back("-main");
        command.emplace_back("call");
        break;
    case KotlinJsMain::NoCall:
        command.emplace_back("-main");
        command.emplace_back("noCall");
        break;
    }
    if (JsMetaInfo)
        command.emplace_back("-meta-info");
    switch (JsModuleKind)
    {
    case KotlinJsModuleKind::None:
        break;
    case KotlinJsModuleKind::Umd:
        command.emplace_back("-module-kind");
        command.emplace_back("umd");
        break;
    case KotlinJsModuleKind::CommonJs:
        command.emplace_back("-module-kind");
        command.emplace_back("commonjs");
        break;
    case KotlinJsModuleKind::Amd:
        command.emplace_back("-module-kind");
        command.emplace_back("amd");
        break;
    case KotlinJsModuleKind::Plain:
        command.emplace_back("-module-kind");
        command.emplace_back("plain");
        break;
    }
    if (JsNoStdlib)
        command.emplace_back("-no-stdlib");
    if (JsOutput)
    {
        command.emplace_back("-output");
        command.push_back(*JsOutput);
    }
    if (JsOutputPostfix)
    {
        command.emplace_back("-output-postfix");
        command.push_back(*JsOutputPostfix);
    }
    if (JsOutputPrefix)
    {
        command.emplace_back("-output-prefix");
        command.push_back(*JsOutputPrefix);
    }
    if (JsSourceMap)
        command.emplace_back("-source-map");
    if (!JsSourceMapBaseDirs.empty())
    {
        std::string dirs;
        for (auto it = JsSourceMapBaseDirs.begin(); it != JsSourceMapBaseDirs.end(); ++it)
        {
            if (it != JsSourceMapBaseDirs.begin())
                dirs += ','; // TODO
            dirs += *it;
        }
        command.emplace_back("-source-map-base-dirs");
        command.push_back(dirs);
    }
    switch (JsSourceMapEmbedSources)
    {
    case KotlinJsSourceMapEmbedSources::None:
        break;
    case KotlinJsSourceMapEmbedSources::Always:
        command.emplace_back("-source-map-embed-sources");
        command.emplace_back("always");
        break;
    case KotlinJsSourceMapEmbedSources::Never:
        command.emplace_back("-source-map-embed-sources");
        command.emplace_back("never");
        break;
    case KotlinJsSourceMapEmbedSources::Inlining:
        command.emplace_back("-source-map-embed-sources");
        command.emplace_back("inlining");
        break;
    }
    switch (JsSourceMapNamesPolicy)
    {
    case KotlinJsSourceMapNamesPolicy::None:
        break;
    case KotlinJsSourceMapNamesPolicy::SimpleNames:
        command.emplace_back("-source-map-names-policy");
        command.emplace_back("simple-names");
        break;
    case KotlinJsSourceMapNamesPolicy::FullyQualifiedNames:
        command.emplace_back("-source-map-names-policy");
        command.emplace_back("fully-qualified-names");
        break;
    case KotlinJsSourceMapNamesPolicy::No:
        command.emplace_back("-source-map-names-policy");
        command.emplace_back("no");
        break;
    }
    if (JsSourceMapPrefix)
    {
        command.emplace_back("-source-map-prefix");
        command.push_back(*JsSourceMapPrefix);
    }
    if (JsTarget)
    {
        command.emplace_back("-target");
        command.push_back(*JsTarget);
    }
    if (JsEnableImplementingInterfacesFromTypeScript)
        command.emplace_back("-Xenable-implementing-interfaces-from-typescript");
    if (JsEsLongAsBigint)
        command.emplace_back("-Xes-long-as-bigint");

#pragma endregion

#pragma region Native

    // TODO

#pragma endregion

    return command;
}
