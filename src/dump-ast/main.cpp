#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>

using clang::tooling::CommonOptionsParser;

class DumpASTConsumer : public clang::ASTConsumer {
public:
    DumpASTConsumer() = default;

    void HandleTranslationUnit(clang::ASTContext& ctx) override
    {
        auto* tu_decl = ctx.getTranslationUnitDecl();
        tu_decl->dump();
    }
};

class DumpFrontndAction : public clang::ASTFrontendAction {
public:
    DumpFrontndAction() = default;

    std::unique_ptr<clang::ASTConsumer>
    CreateASTConsumer(clang::CompilerInstance&  /*compiler*/, llvm::StringRef  /*file*/) override
    {
        return std::make_unique<DumpASTConsumer>();
    }
};

static llvm::cl::OptionCategory s_category_option("Dump-AST");

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

    auto& options_parser = expected_options_parser.get();
    clang::tooling::ClangTool tool(
        options_parser.getCompilations(),
        options_parser.getSourcePathList());

    return tool.run(clang::tooling::newFrontendActionFactory<DumpFrontndAction>().get());
}