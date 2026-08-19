#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace kompose
{
    enum class KotlinKLibIrInliner
    {
        None,
        Disabled,
        Full,
    };

    enum class KotlinNameBasedDestructuring
    {
        None,
        OnlySyntax,
        NameMismatch,
        Complete,
    };

    enum class KotlinReturnValueChecker
    {
        None,
        Disable,
        Check,
        Full,
    };

    enum class KotlinWarningLevel
    {
        Error,
        Warning,
        Disabled,
    };

    enum class KotlinJvmDefaultMode
    {
        None,
        Enable,
        NoCompatibility,
        Disable,
    };

    enum class KotlinJvmNullabilityAnnotationReportLevel
    {
        Ignore,
        Warn,
        Strict,
    };

    enum class KotlinJsMain
    {
        None,
        Call,
        NoCall,
    };

    enum class KotlinJsModuleKind
    {
        None,
        Umd,
        CommonJs,
        Amd,
        Plain,
    };

    enum class KotlinJsSourceMapEmbedSources
    {
        None,
        Always,
        Never,
        Inlining,
    };

    enum class KotlinJsSourceMapNamesPolicy
    {
        None,
        SimpleNames,
        FullyQualifiedNames,
        No,
    };

    enum class KotlinNativeProduce
    {
        Program,
        Static,
        Dynamic,
        Framework,
        Library,
        Bitcode,
    };

    struct KotlinCommand
    {
        std::vector<std::string> operator()() const;

#pragma region Common

        /** -api-version <version> */
        std::optional<std::string> ApiVersion;
        /** -kotlin-home <path> */
        std::optional<std::string> KotlinHome;
        /** -language-version <version> */
        std::optional<std::string> LanguageVersion;
        /** -opt-in <annotation> */
        std::unordered_set<std::string> OptIn;
        /** -P <plugin>:<pluginId>:<optionName>=<value> */
        std::unordered_map<std::string, std::string> PluginOptions;
        /** -progressive */
        bool Progressive{};
        /** -script */
        bool Script{};
        /** -verbose */
        bool Verbose{};
        /** -Xallow-contracts-on-more-functions */
        bool AllowContractsOnMoreFunctions{};
        /** -Xallow-condition-implies-returns-contracts */
        bool AllowConditionImpliesReturnsContracts{};
        /** -Xallow-holdsin-contract */
        bool AllowHoldsinContract{};
        /** -Xallow-returns-result-of */
        bool AllowReturnsResultOf{};
        /** -Xallow-reified-type-in-catch */
        bool AllowReifiedTypeInCatch{};
        /** -Xcollection-literals */
        bool CollectionLiterals{};
        /** -Xcompiler-plugin-order <plugin before>><plugin after> */
        std::unordered_map<std::string, std::string> CompilerPluginOrder;
        /** -Xdata-flow-based-exhaustiveness */
        bool DataFlowBasedExhaustiveness{};
        /** -Xexplicit-context-arguments */
        bool ExplicitContextArguments{};
        /** -Xklib-ir-inliner (disabled|full) */
        KotlinKLibIrInliner KLibIrInliner{};
        /** -Xintrinsic-const-evaluation */
        bool IntrinsicConstEvaluation{};
        /** -Xname-based-destructuring (only-syntax|name-mismatch|complete) */
        KotlinNameBasedDestructuring NameBasedDestructuring{};
        /** -Xreturn-value-checker (disable|check|full) */
        KotlinReturnValueChecker ReturnValueChecker{};
        /** -nowarn */
        bool NoWarn{};
        /** -Werror */
        bool WError{};
        /** -Wextra */
        bool WExtra{};
        /** -Xrender-internal-diagnostic-names */
        bool RenderInternalDiagnosticNames{};
        /** -Xwarning-level <diagnostic name>:(error|warning|disabled) */
        std::unordered_map<std::string, KotlinWarningLevel> WarningLevel;

#pragma endregion

#pragma region JVM

        /** -classpath <path> | -cp <path> */
        std::unordered_set<std::string> JvmClassPath;
        /** -d <path> */
        std::optional<std::string> JvmDestination;
        /** -include-runtime */
        bool JvmIncludeRuntime{};
        /** -jdk-home-path <paht> */
        std::optional<std::string> JvmJdkHomePath;
        /** -Xjdk-release <version> */
        std::optional<std::string> JvmJdkRelease;
        /** -jvm-default-mode (enable|no-compatibility|disable) */
        KotlinJvmDefaultMode JvmDefaultMode{};
        /** -jvm-target-version <version> */
        std::optional<std::string> JvmTargetVersion;
        /** -java-parameters */
        bool JvmJavaParameters{};
        /** -module-name <name> */
        std::optional<std::string> JvmModuleName;
        /** -no-jdk */
        bool JvmNoJdk{};
        /** -no-reflect */
        bool JvmNoReflect{};
        /** -no-stdlib */
        bool JvmNoStdlib{};
        /** -script-templates <classname,...> */
        std::unordered_set<std::string> JvmScriptTemplates;
        /** -Xjvm-expose-boxed */
        bool JvmExposeBoxed{};
        /** -Xnullability-annotations \@<package name>:<report level> */
        std::unordered_map<std::string, KotlinJvmNullabilityAnnotationReportLevel>
        JvmNullabilityAnnotations;

#pragma endregion

#pragma region JS

        /** -libraries <path> */
        std::unordered_set<std::string> JsLibraries;
        /** -main (call|noCall) */
        KotlinJsMain JsMain{};
        /** -meta-info */
        bool JsMetaInfo{};
        /** -module-kind (umd|commonjs|amd|plain) */
        KotlinJsModuleKind JsModuleKind{};
        /** -no-stdlib */
        bool JsNoStdlib{};
        /** -output <filepath> */
        std::optional<std::string> JsOutput;
        /** -output-postfix <filepath> */
        std::optional<std::string> JsOutputPostfix;
        /** -output-prefix <filepath> */
        std::optional<std::string> JsOutputPrefix;
        /** -source-map */
        bool JsSourceMap{};
        /** -source-map-base-dirs <path> */
        std::unordered_set<std::string> JsSourceMapBaseDirs;
        /** -source-map-embed-sources (always|never|inlining) */
        KotlinJsSourceMapEmbedSources JsSourceMapEmbedSources{};
        /** -source-map-names-policy (simple-names|fully-qualified-names|no) */
        KotlinJsSourceMapNamesPolicy JsSourceMapNamesPolicy{};
        /** -source-map-prefix <prefix> */
        std::optional<std::string> JsSourceMapPrefix;
        /** -target (es5|es2015) */
        std::optional<std::string> JsTarget;
        /** -Xenable-implementing-interfaces-from-typescript */
        bool JsEnableImplementingInterfacesFromTypeScript{};
        /** -Xes-long-as-bigint */
        bool JsEsLongAsBigint{};

#pragma endregion

#pragma region Native

        /** -enable-assertions | -ea */
        bool NativeEnableAssertions{};
        /** -entry <name> | -e <name> */
        std::optional<std::string> NativeEntry;
        /** -g */
        bool NativeEmitDebugInformation{};
        /** -generate-test-runner | -tr */
        bool NativeGenerateTestRunner{};
        /** -generate-no-exit-test-runner | -trn */
        bool NativeGenerateNoExitTestRunner{};
        /** -include-binary <path> | -ib <path> */
        std::unordered_set<std::string> NativeIncludeBinaries;
        /** -library <path> | -l <path> */
        std::unordered_set<std::string> NativeLibraries;
        /** -library-version <version> */
        std::optional<std::string> NativeLibraryVersion;
        /** -linker-option <option> | -linker-options <option...> */
        std::vector<std::string> NativeLinkerOptions;
        /** -manifest <path> */
        std::optional<std::string> NativeManifest;
        /** -module-name <name> */
        std::optional<std::string> NativeModuleName;
        /** -native-library <path> | -nl <path> */
        std::unordered_set<std::string> NativeBitcodeLibraries;
        /** -no-default-libs */
        bool NativeNoDefaultLibs{};
        /** -nomain */
        bool NativeNoMain{};
        /** -nopack */
        bool NativeNoPack{};
        /** -nostdlib */
        bool NativeNoStdlib{};
        /** -opt */
        bool NativeOptimize{};
        /** -output <name> | -o <name> */
        std::optional<std::string> NativeOutput;
        /**
         * -produce (program|static|dynamic|framework|library|bitcode)
         * -p (program|static|dynamic|framework|library|bitcode)
         */
        KotlinNativeProduce NativeProduce;
        /** -repo <path> | -r <path> */
        std::unordered_set<std::string> NativeRepos;
        /** -target <target> */
        std::optional<std::string> NativeTarget;
        /** -Xccall-mode */
        bool NativeCcallMode{};
        /** -Xoverride-konan-properties minVersion.<platform>=<version> */
        std::unordered_map<std::string, std::string> NativeOverrideKonanProperties;

#pragma endregion
    };
} // namespace kompose
