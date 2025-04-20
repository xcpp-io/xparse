#include <xparse/reflect.h>

#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>

using clang::tooling::CommonOptionsParser;

class ReflectFrontendAction : public clang::ASTFrontendAction {
public:
    ReflectFrontendAction() = default;

    std::unique_ptr<clang::ASTConsumer>
    CreateASTConsumer(clang::CompilerInstance& compiler, llvm::StringRef /*file*/) override
    {
        auto& options = compiler.getLangOpts();
        options.CommentOpts.ParseAllComments = true;
        return std::make_unique<xparse::ReflectASTConsumer>();
    }
};

int main(int argc, char** argv)
{
    std::vector<const char*> args(argc);
    for (int i = 0; i < argc; ++i) {
        args[i] = argv[i];
    }

    args.insert(args.begin() + 1, "--extra-arg=-D__META__");
    argc = llvm::cast<int>(args.size());

    {
        std::string args_content;
        for (size_t i = 0; i < args.size(); ++i) {
            args_content += args[i];
            if (i != args.size() - 1) {
                args_content += " ";
            }
        }
        XPARSE_LOG_INFO("start parsing, command: \"{0}\"", args_content);
    }

    llvm::cl::OptionCategory s_category_option("XParse");
    auto expected_options_parser = CommonOptionsParser::create(argc, args.data(), s_category_option);
    if (!expected_options_parser) {
        llvm::errs() << expected_options_parser.takeError();
        return -1;
    }

    // parse and collect metadata
    auto& options_parser = expected_options_parser.get();

    clang::tooling::ClangTool tool(
        options_parser.getCompilations(),
        options_parser.getSourcePathList());

    int result = tool.run(clang::tooling::newFrontendActionFactory<ReflectFrontendAction>().get());

    XPARSE_LOG_INFO("parsing completed.");

    return result;
}
