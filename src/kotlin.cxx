#include <kotlin.hxx>

std::vector<std::string> kompose::KotlinCommand::operator()() const {
  std::vector<std::string> command;
  command.push_back("kotlinc");

#pragma region Common

  if (ApiVersion) {
    command.push_back("-api-version");
    command.push_back(*ApiVersion);
  }
  if (KotlinHome) {
    command.push_back("-kotlin-home");
    command.push_back(*KotlinHome);
  }
  if (LanguageVersion) {
    command.push_back("-language-version");
    command.push_back(*LanguageVersion);
  }
  for (auto &entry : OptIn) {
    command.push_back("-opt-in");
    command.push_back(entry);
  }
  for (auto &entry : PluginOptions) {
    command.push_back("-P");
    command.push_back(entry.first + '=' + entry.second);
  }
  if (Progressive) {
    command.push_back("-progressive");
  }
  if (Script) {
    command.push_back("-script");
  }
  if (Verbose) {
    command.push_back("-verbose");
  }
  if (AllowContractsOnMoreFunctions) {
    command.push_back("-Xallow-contracts-on-more-functions");
  }
  if (AllowConditionImpliesReturnsContracts) {
    command.push_back("-Xallow-condition-implies-returns-contracts");
  }
  if (AllowHoldsinContract) {
    command.push_back("-Xallow-holdsin-contract");
  }
  if (AllowReturnsResultOf) {
    command.push_back("-Xallow-returns-result-of");
  }
  if (AllowReifiedTypeInCatch) {
    command.push_back("-Xallow-reified-type-in-catch");
  }
  if (CollectionLiterals) {
    command.push_back("-Xcollection-literals");
  }
  for (auto &entry : CompilerPluginOrder) {
    command.push_back("-X-compiler-plugin-order");
    command.push_back(entry.first + '>' + entry.second);
  }
  if (DataFlowBasedExhaustiveness) {
    command.push_back("-Xdata-flow-based-exhaustiveness");
  }
  if (ExplicitContextArguments) {
    command.push_back("-Xexplicit-context-arguments");
  }
  switch (KLibIrInliner) {
  case KotlinKLibIrInliner::None:
    break;
  case KotlinKLibIrInliner::Disabled:
    command.push_back("-Xklib-ir-inliner");
    command.push_back("disabled");
    break;
  case KotlinKLibIrInliner::Full:
    command.push_back("-Xklib-ir-inliner");
    command.push_back("full");
    break;
  }
  if (IntrinsicConstEvaluation) {
    command.push_back("-Xintrinsic-const-evaluation");
  }
  switch (NameBasedDestructuring) {
  case KotlinNameBasedDestructuring::None:
    break;
  case KotlinNameBasedDestructuring::OnlySyntax:
    command.push_back("-Xname-based-destructuring");
    command.push_back("only-syntax");
    break;
  case KotlinNameBasedDestructuring::NameMismatch:
    command.push_back("-Xname-based-destructuring");
    command.push_back("name-mismatch");
    break;
  case KotlinNameBasedDestructuring::Complete:
    command.push_back("-Xname-based-destructuring");
    command.push_back("complete");
    break;
  }
  switch (ReturnValueChecker) {
  case KotlinReturnValueChecker::None:
    break;
  case KotlinReturnValueChecker::Disable:
    command.push_back("-Xreturn-value-checker");
    command.push_back("disable");
    break;
  case KotlinReturnValueChecker::Check:
    command.push_back("-Xreturn-value-checker");
    command.push_back("check");
    break;
  case KotlinReturnValueChecker::Full:
    command.push_back("-Xreturn-value-checker");
    command.push_back("full");
    break;
  }
  if (NoWarn) {
    command.push_back("-nowarn");
  }
  if (WError) {
    command.push_back("-Werror");
  }
  if (WExtra) {
    command.push_back("-Wextra");
  }
  if (RenderInternalDiagnosticNames) {
    command.push_back("-Xrender-internal-diagnostic-names");
  }
  for (auto &entry : WarningLevel) {
    std::string name;
    switch (entry.second) {
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
    command.push_back("-Xwarning-level");
    command.push_back(entry.first + ':' + name);
  }

#pragma endregion

#pragma region JVM

  if (!JvmClassPath.empty()) {
    std::string classpath;
    for (auto it = JvmClassPath.begin(); it != JvmClassPath.end(); ++it) {
      if (it != JvmClassPath.begin()) {
        classpath += ':';
      }
      classpath += *it;
    }
    command.push_back("-classpath");
    command.push_back(classpath);
  }
  if (JvmDestination) {
    command.push_back("-d");
    command.push_back(*JvmDestination);
  }
  if (JvmIncludeRuntime) {
    command.push_back("-include-runtime");
  }
  if (JvmJdkHomePath) {
    command.push_back("-jdk-home-path");
    command.push_back(*JvmJdkHomePath);
  }
  if (JvmJdkRelease) {
    command.push_back("-jdk-release");
    command.push_back(*JvmJdkRelease);
  }
  switch (JvmDefaultMode) {
  case KotlinJvmDefaultMode::None:
    break;
  case KotlinJvmDefaultMode::Enable:
    command.push_back("-jvm-default-mode");
    command.push_back("enable");
    break;
  case KotlinJvmDefaultMode::NoCompatibility:
    command.push_back("-jvm-default-mode");
    command.push_back("no-compatibility");
    break;
  case KotlinJvmDefaultMode::Disable:
    command.push_back("-jvm-default-mode");
    command.push_back("disable");
    break;
  }
  if (JvmTargetVersion) {
    command.push_back("-jvm-target-version");
    command.push_back(*JvmTargetVersion);
  }
  if (JvmJavaParameters) {
    command.push_back("-java-parameters");
  }
  if (JvmModuleName) {
    command.push_back("-module-name");
    command.push_back(*JvmModuleName);
  }
  if (JvmNoJdk) {
    command.push_back("-no-jdk");
  }
  if (JvmNoReflect) {
    command.push_back("-no-reflect");
  }
  if (JvmNoStdlib) {
    command.push_back("-no-stdlib");
  }
  if (!JvmScriptTemplates.empty()) {
    std::string templates;
    for (auto it = JvmScriptTemplates.begin(); it != JvmScriptTemplates.end();
         ++it) {
      if (it != JvmScriptTemplates.begin()) {
        templates += ',';
      }
      templates += *it;
    }
    command.push_back("-script-templates");
    command.push_back(templates);
  }
  if (JvmExposeBoxed) {
    command.push_back("-Xjvm-expose-boxed");
  }
  for (auto &entry : JvmNullabilityAnnotations) {
    std::string level;
    switch (entry.second) {
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
    command.push_back("-Xnullability-annotations");
    command.push_back('@' + entry.first + ':' + level);
  }

#pragma endregion

#pragma region JS

  if (!JsLibraries.empty()) {
    std::string path;
    for (auto it = JsLibraries.begin(); it != JsLibraries.end(); ++it) {
      if (it != JsLibraries.begin()) {
        path += ','; // TODO
      }
      path += *it;
    }
    command.push_back("-libraries");
    command.push_back(path);
  }
  switch (JsMain) {
  case KotlinJsMain::None:
    break;
  case KotlinJsMain::Call:
    command.push_back("-main");
    command.push_back("call");
    break;
  case KotlinJsMain::NoCall:
    command.push_back("-main");
    command.push_back("noCall");
    break;
  }
  if (JsMetaInfo) {
    command.push_back("-meta-info");
  }
  switch (JsModuleKind) {
  case KotlinJsModuleKind::None:
    break;
  case KotlinJsModuleKind::Umd:
    command.push_back("-module-kind");
    command.push_back("umd");
    break;
  case KotlinJsModuleKind::CommonJs:
    command.push_back("-module-kind");
    command.push_back("commonjs");
    break;
  case KotlinJsModuleKind::Amd:
    command.push_back("-module-kind");
    command.push_back("amd");
    break;
  case KotlinJsModuleKind::Plain:
    command.push_back("-module-kind");
    command.push_back("plain");
    break;
  }
  if (JsNoStdlib) {
    command.push_back("-no-stdlib");
  }
  if (JsOutput) {
    command.push_back("-output");
    command.push_back(*JsOutput);
  }
  if (JsOutputPostfix) {
    command.push_back("-output-postfix");
    command.push_back(*JsOutputPostfix);
  }
  if (JsOutputPrefix) {
    command.push_back("-output-prefix");
    command.push_back(*JsOutputPrefix);
  }
  if (JsSourceMap) {
    command.push_back("-source-map");
  }
  if (!JsSourceMapBaseDirs.empty()) {
    std::string dirs;
    for (auto it = JsSourceMapBaseDirs.begin(); it != JsSourceMapBaseDirs.end();
         ++it) {
      if (it != JsSourceMapBaseDirs.begin()) {
        dirs += ','; // TODO
      }
      dirs += *it;
    }
    command.push_back("-source-map-base-dirs");
    command.push_back(dirs);
  }
  switch (JsSourceMapEmbedSources) {
  case KotlinJsSourceMapEmbedSources::None:
    break;
  case KotlinJsSourceMapEmbedSources::Always:
    command.push_back("-source-map-embed-sources");
    command.push_back("always");
    break;
  case KotlinJsSourceMapEmbedSources::Never:
    command.push_back("-source-map-embed-sources");
    command.push_back("never");
    break;
  case KotlinJsSourceMapEmbedSources::Inlining:
    command.push_back("-source-map-embed-sources");
    command.push_back("inlining");
    break;
  }
  switch (JsSourceMapNamesPolicy) {
  case KotlinJsSourceMapNamesPolicy::None:
    break;
  case KotlinJsSourceMapNamesPolicy::SimpleNames:
    command.push_back("-source-map-names-policy");
    command.push_back("simple-names");
    break;
  case KotlinJsSourceMapNamesPolicy::FullyQualifiedNames:
    command.push_back("-source-map-names-policy");
    command.push_back("fully-qualified-names");
    break;
  case KotlinJsSourceMapNamesPolicy::No:
    command.push_back("-source-map-names-policy");
    command.push_back("no");
    break;
  }
  if (JsSourceMapPrefix) {
    command.push_back("-source-map-prefix");
    command.push_back(*JsSourceMapPrefix);
  }
  if (JsTarget) {
    command.push_back("-target");
    command.push_back(*JsTarget);
  }
  if (JsEnableImplementingInterfacesFromTypeScript) {
    command.push_back("-Xenable-implementing-interfaces-from-typescript");
  }
  if (JsEsLongAsBigint) {
    command.push_back("-Xes-long-as-bigint");
  }

#pragma endregion

#pragma region Native

  // TODO

#pragma endregion

  return command;
}
