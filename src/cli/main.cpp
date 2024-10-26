#include <xparse/log.h>
#include <xparse/reflect.h>
#include <xparse/serialize.h>

#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>

using clang::tooling::CommonOptionsParser;

using namespace xparse;

static llvm::cl::OptionCategory s_category_option("XParse");

static llvm::cl::opt<std::string> s_root(
    "root", llvm::cl::Required,
    llvm::cl::desc("Specify root directory"),
    llvm::cl::cat(s_category_option), llvm::cl::value_desc("string"));

static llvm::cl::list<std::string> s_marks(
    "marks",
    llvm::cl::desc("<mark0> [... <markN>]"),
    llvm::cl::cat(s_category_option), llvm::cl::value_desc("string(s)"));

static llvm::cl::opt<std::string> s_output(
    "output", llvm::cl::Required,
    llvm::cl::desc("Specify database output directory"),
    llvm::cl::cat(s_category_option), llvm::cl::value_desc("string"));

static ProjectMetaInfo s_project_meta_info;

class ReflectFrontendAction : public clang::ASTFrontendAction {
public:
    ReflectFrontendAction() = default;

    std::unique_ptr<clang::ASTConsumer>
    CreateASTConsumer(clang::CompilerInstance& compiler, llvm::StringRef file)
    {
        auto& options = compiler.getLangOpts();
        options.CommentOpts.ParseAllComments = true;
        return std::make_unique<ReflectASTConsumer>(s_project_meta_info, s_root, s_marks);
    }
};

int main(int argc, char** argv)
{
    std::vector<const char*> args(argc);
    for (int i = 0; i < argc; ++i) {
        args[i] = argv[i];
    }
    auto expected_options_parser = CommonOptionsParser::create(argc, args.data(), s_category_option);
    if (!expected_options_parser) {
        llvm::errs() << expected_options_parser.takeError();
        return -1;
    }

    // check if root and and output exist.
    if (!std::filesystem::exists(s_root.getValue())) {
        XPARSE_LOG_ERROR("The root directory doesn't exist, path: \"{0}\".", s_root.getValue());
        return -1;
    }

    std::filesystem::path output_dir(s_output.getValue());
    if (!std::filesystem::exists(output_dir)) {
        XPARSE_LOG_ERROR("The output directory doesn't exist, path: \"{0}\".", s_output.getValue());
        return -1;
    }

    XPARSE_LOG_INFO("Start to parse the project, root dir: \"{0}\", output dir: \"{1}\".",
        s_root.getValue(), s_output.getValue());

    // parse and collect metadata
    auto& options_parser = expected_options_parser.get();
    clang::tooling::ClangTool tool(
        options_parser.getCompilations(),
        options_parser.getSourcePathList());

    int result = tool.run(clang::tooling::newFrontendActionFactory<ReflectFrontendAction>().get());

    XPARSE_LOG_INFO("Parsing completed, preparing to output meta file.");

    // output
    for (auto& [filename, database] : s_project_meta_info) {
        if (isMetadataEmpty(database)) {
            continue;
        }
        std::filesystem::path meta_filepath = std::filesystem::weakly_canonical(output_dir / filename);
        meta_filepath.replace_extension(meta_filepath.extension().string() + ".meta");

        std::filesystem::path directory = meta_filepath.parent_path();
        if (!directory.empty() && !std::filesystem::exists(directory)) {
            std::filesystem::create_directories(directory);
            XPARSE_LOG_INFO("Create directory: \"{0}\".", directory.string());
        }

        std::error_code err_code;
        llvm::raw_fd_stream outs(meta_filepath.string(), err_code);
        if (err_code) {
            XPARSE_LOG_ERROR("Error writing file \"{0}\", reason: {1}.",
                meta_filepath.string(), err_code.message());
            return -1;
        }

        llvm::json::OStream json_outs { outs };
        Serializer::serialize(json_outs, database);
        outs.flush();

        XPARSE_LOG_INFO("Output meta file: \"{0}\".", meta_filepath.string());
    }
    XPARSE_LOG_INFO("Meta file output completed!");

    return result;
}