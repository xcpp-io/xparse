/**
 * *****************************************************************************
 * @file        reflect.h
 * @brief
 * @author      hsz (hszsoftware@qq.com)
 * @date        2024-10-22
 * @copyright   hszsoft
 * *****************************************************************************
 */

#ifndef __XPARSE_REFLECT_H__
#define __XPARSE_REFLECT_H__

#include "log.h"
#include "meta.h"

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/Attr.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/FormatVariadic.h>

#include <filesystem>

namespace xparse {

namespace detail {

    using LimitedString = llvm::SmallString<1024>;

    inline std::string removeAllSpaces(const std::string& input)
    {
        std::string output = input;
        output.erase(std::remove(output.begin(), output.end(), ' '), output.end());
        return output;
    }

    inline std::string getMemberAccess(clang::NamedDecl* decl)
    {
        std::string result = "none";
        auto access_specifier = decl->getAccess();
        switch (access_specifier) {
        case clang::AS_public:
            result = "public";
            break;
        case clang::AS_protected:
            result = "protected";
            break;
        case clang::AS_private:
            result = "private";
            break;
        default:
            break;
        }
        return result;
    }

    inline bool isValid(clang::NamedDecl* decl)
    {
        return (decl != nullptr && !decl->isInvalidDecl());
    }

    inline bool isMarked(clang::NamedDecl* decl)
    {
        for (auto* annotate : decl->specific_attrs<clang::AnnotateAttr>()) {
            if (annotate->getAnnotation() == "__reflect__") {
                return true;
            }
        }
        return false;
    }

} // namespace detail

class ReflectASTConsumer : public clang::ASTConsumer {
public:
    ReflectASTConsumer(ProjectMetaInfo& metadata)
        : m_metadata(&metadata)
    {
    }

    /**
     * @brief       LLVM's interface
     *
     * @param       ctx
     */
    void HandleTranslationUnit(clang::ASTContext& ctx) override;

protected:
    unsigned int getDeclLine(clang::NamedDecl* decl);
    std::string getDeclFilename(clang::NamedDecl* decl);

    enum HandleResult : std::uint8_t {
        kSuccess,
        kFailure
    };

    void handleDecl(clang::NamespaceDecl* decl);

    HandleResult handleDecl(clang::NamedDecl* decl, MetaInfo& info);
    HandleResult handleDecl(clang::ValueDecl* decl, ValueMetaInfo& info);
    HandleResult handleDecl(clang::FunctionDecl* decl, FunctionMetaInfo& info);
    HandleResult handleDecl(clang::ParmVarDecl* decl, ValueMetaInfo& info);

    void handleDecl(clang::CXXRecordDecl* decl);
    HandleResult handleDecl(clang::FieldDecl* decl, FieldMetaInfo& info);
    HandleResult handleDecl(clang::VarDecl* decl, FieldMetaInfo& info);
    HandleResult handleDecl(clang::CXXMethodDecl* decl, MethodMetaInfo& info);

    void handleDecl(clang::FunctionDecl* decl);

    void handleDecl(clang::EnumDecl* decl);
    HandleResult handleDecl(clang::EnumConstantDecl* decl, EnumConstantMetaInfo& info);

private:
    ProjectMetaInfo*    m_metadata;
    clang::ASTContext*  m_context;
};

inline void ReflectASTConsumer::HandleTranslationUnit(clang::ASTContext& ctx)
{
    m_context = &ctx;
    auto* tu_decl = ctx.getTranslationUnitDecl();
    for (auto* decl : tu_decl->decls()) {
        auto* named_decl = llvm::dyn_cast<clang::NamedDecl>(decl);
        if (named_decl == nullptr || named_decl->isInvalidDecl()) {
            continue;
        }

        // filter out unsupported decls.
        // The processing of TranslationUnitDecl and NamepaceDecl is somewhat similar,
        // but their logic does not require reuse.
        auto kind = named_decl->getKind();
        switch (kind) {
        case clang::Decl::Namespace:
            this->handleDecl(llvm::cast<clang::NamespaceDecl>(named_decl));
            break;
        case clang::Decl::CXXRecord:
            this->handleDecl(llvm::cast<clang::CXXRecordDecl>(named_decl));
            break;
        case clang::Decl::Enum:
            this->handleDecl(llvm::cast<clang::EnumDecl>(named_decl));
            break;
        case clang::Decl::Function:
            this->handleDecl(llvm::cast<clang::FunctionDecl>(named_decl));
            break;
        default:
            break;
        }
    }
}

inline std::string ReflectASTConsumer::getDeclFilename(clang::NamedDecl* decl)
{
    std::string filename;

    auto location = m_context->getSourceManager().getPresumedLoc(decl->getLocation());
    if (!location.isInvalid()) {
        std::filesystem::path filepath(clang::tooling::getAbsolutePath(location.getFilename()));
        filename = std::filesystem::canonical(filepath).string();
    }
    return filename;
}

inline void ReflectASTConsumer::handleDecl(clang::NamespaceDecl* decl)
{
    if (!detail::isValid(decl)) {
        return;
    }

    auto* decl_context = clang::Decl::castToDeclContext(decl);
    for (auto* child_decl : decl_context->decls()) {
        switch (child_decl->getKind()) {
        case clang::Decl::Namespace:
            this->handleDecl(llvm::cast<clang::NamespaceDecl>(child_decl));
            break;
        case clang::Decl::CXXRecord:
            this->handleDecl(llvm::cast<clang::CXXRecordDecl>(child_decl));
            break;
        case clang::Decl::Enum:
            this->handleDecl(llvm::cast<clang::EnumDecl>(child_decl));
            break;
        case clang::Decl::Function:
            this->handleDecl(llvm::cast<clang::FunctionDecl>(child_decl));
            break;
        default:
            break;
        }
    }
}

inline ReflectASTConsumer::HandleResult ReflectASTConsumer::handleDecl(clang::NamedDecl* decl, MetaInfo& info)
{
    if (!detail::isValid(decl)) {
        return kFailure;
    }

    info.name = decl->getNameAsString();
    info.full_name = decl->getQualifiedNameAsString();
    info.access = detail::getMemberAccess(decl);

    for (auto* annotate : decl->specific_attrs<clang::AnnotateAttr>()) {
        if (annotate->getAnnotation() != "__reflect__") {
            info.attrs.push_back(annotate->getAnnotation().str());
        }
    }

    auto* raw_comment = m_context->getRawCommentForDeclNoCache(decl);
    if (raw_comment) {
        info.comment = raw_comment->getBriefText(*m_context);
    }

    return kSuccess;
}

inline ReflectASTConsumer::HandleResult ReflectASTConsumer::handleDecl(clang::ValueDecl* decl, ValueMetaInfo& info)
{
    if (!detail::isValid(decl)) {
        return kFailure;
    }

    if (this->handleDecl(llvm::cast<clang::NamedDecl>(decl), info) == kFailure) {
        return kFailure;
    }

    info.type = decl->getType().getAsString();
    info.raw_type = decl->getType().getCanonicalType().getAsString();

    return kSuccess;
}

inline ReflectASTConsumer::HandleResult ReflectASTConsumer::handleDecl(clang::FunctionDecl* decl, FunctionMetaInfo& info)
{
    if (!detail::isValid(decl)) {
        return kFailure;
    }

    if (this->handleDecl(llvm::cast<clang::NamedDecl>(decl), info) == kFailure) {
        return kFailure;
    }

    info.ret_type = decl->getReturnType().getAsString();
    info.ret_raw_type = decl->getReturnType().getCanonicalType().getAsString();

    for (auto* param_decl : decl->parameters()) {
        ValueMetaInfo param_info;
        if (this->handleDecl(llvm::cast<clang::ParmVarDecl>(param_decl), param_info) == kSuccess) {
            info.params.push_back(param_info);
        }
    }

    info.is_static = decl->isStatic();

    return kSuccess;
}

inline ReflectASTConsumer::HandleResult ReflectASTConsumer::handleDecl(clang::ParmVarDecl* decl, ValueMetaInfo& info)
{
    if (!detail::isValid(decl)) {
        return kFailure;
    }

    if (this->handleDecl(llvm::cast<clang::ValueDecl>(decl), info) == kFailure) {
        return kFailure;
    }

    if (decl->hasDefaultArg()) {
        const clang::Expr* default_arg = decl->getDefaultArg();
        std::string default_value;
        llvm::raw_string_ostream rso(default_value);
        default_arg->printPretty(rso, nullptr, clang::PrintingPolicy(clang::LangOptions()));
        info.default_value = rso.str();
    }

    return kSuccess;
}

inline void ReflectASTConsumer::handleDecl(clang::CXXRecordDecl* decl)
{
    if (!detail::isValid(decl)) {
        return;
    }

    // take out cases with only declarations, anonymous record and union.
    if (!decl->isThisDeclarationADefinition() || decl->isAnonymousStructOrUnion() || decl->isUnion()) {
        return;
    }

    if (!detail::isMarked(decl)) {
        return;
    }

    RecordMetaInfo info;
    if (this->handleDecl(llvm::cast<clang::NamedDecl>(decl), info) == kFailure) {
        return;
    }

    for (const auto& base : decl->bases()) {
        auto* base_decl = base.getType()->getAsCXXRecordDecl();
        if (base_decl) {
            info.bases.push_back(base_decl->getQualifiedNameAsString());
        }
    }

    for (auto* child_decl : decl->decls()) {
        switch (child_decl->getKind()) {
        case clang::Decl::Field:
        {
            FieldMetaInfo field_info;
            if (this->handleDecl(llvm::cast<clang::FieldDecl>(child_decl), field_info) == kSuccess) {
                info.fields.push_back(field_info);
            }
            break;
        }
        case clang::Decl::Var:
        {
            FieldMetaInfo field_info;
            if (this->handleDecl(llvm::cast<clang::VarDecl>(child_decl), field_info) == kSuccess) {
                info.fields.push_back(field_info);
            }
            break;
        }
        case clang::Decl::CXXMethod:
        {
            MethodMetaInfo method_info;
            if (this->handleDecl(llvm::cast<clang::CXXMethodDecl>(child_decl), method_info) == kSuccess) {
                info.methods.push_back(method_info);
            }
            break;
        }
        case clang::Decl::CXXRecord:
            this->handleDecl(llvm::cast<clang::CXXRecordDecl>(child_decl));
            break;
        case clang::Decl::Enum:
            this->handleDecl(llvm::cast<clang::EnumDecl>(child_decl));
            break;
        default:
            break;
        }
    }

    (*m_metadata)[this->getDeclFilename(decl)].records.push_back(info);

    XPARSE_LOG_INFO("handled record: {0}.", info.full_name);
}

inline ReflectASTConsumer::HandleResult ReflectASTConsumer::handleDecl(clang::FieldDecl* decl, FieldMetaInfo& info)
{
    if (!detail::isValid(decl)) {
        return kFailure;
    }

    // currently we don't support indirect field.
    if (decl->isAnonymousStructOrUnion()) {
        return kFailure;
    }

    if (this->handleDecl(llvm::cast<clang::ValueDecl>(decl), info) == kFailure) {
        return kFailure;
    }

    info.is_static = false;
    if (decl->hasInClassInitializer()) {
        const clang::Expr* default_arg = decl->getInClassInitializer();
        std::string default_value;
        llvm::raw_string_ostream rso(default_value);
        default_arg->printPretty(rso, nullptr, clang::PrintingPolicy(clang::LangOptions()));
        info.default_value = rso.str();
    }

    return kSuccess;
}

inline ReflectASTConsumer::HandleResult ReflectASTConsumer::handleDecl(clang::VarDecl* decl, FieldMetaInfo& info)
{
    if (!detail::isValid(decl)) {
        return kFailure;
    }

    if (this->handleDecl(llvm::cast<clang::ValueDecl>(decl), info) == kFailure) {
        return kFailure;
    }

    info.is_static = true;
    if (decl->hasInit()) {
        const clang::Expr* default_arg = decl->getInit();
        std::string default_value;
        llvm::raw_string_ostream rso(default_value);
        default_arg->printPretty(rso, nullptr, clang::PrintingPolicy(clang::LangOptions()));
        info.default_value = rso.str();
    }

    return kSuccess;
}

inline ReflectASTConsumer::HandleResult ReflectASTConsumer::handleDecl(clang::CXXMethodDecl* decl, MethodMetaInfo& info)
{
    if (!detail::isValid(decl)) {
        return kFailure;
    }

    if (this->handleDecl(llvm::cast<clang::FunctionDecl>(decl), info) == kFailure) {
        return kFailure;
    }

    info.is_virtual = decl->isVirtual();
    info.is_pure_virtual = decl->isPureVirtual();
    info.is_override = decl->size_overridden_methods() > 0;

    return kSuccess;
}

inline void ReflectASTConsumer::handleDecl(clang::FunctionDecl* decl)
{
    if (!detail::isValid(decl) || !detail::isMarked(decl)) {
        return;
    }

    FunctionMetaInfo info;
    if (this->handleDecl(decl, info) == kFailure) {
        return;
    }

    (*m_metadata)[this->getDeclFilename(decl)].functions.push_back(info);

    XPARSE_LOG_INFO("handled function: {0}.", info.full_name);
}

inline void ReflectASTConsumer::handleDecl(clang::EnumDecl* decl)
{
    if (!detail::isValid(decl) || !detail::isMarked(decl)) {
        return;
    }

    EnumMetaInfo info;
    if (this->handleDecl(llvm::cast<clang::NamedDecl>(decl), info) == kFailure) {
        return;
    }

    for (auto* constant_decl : decl->enumerators()) {
        EnumConstantMetaInfo constant_info;
        if (this->handleDecl(constant_decl, constant_info) == kSuccess) {
            info.constants.push_back(constant_info);
        }
    }

    (*m_metadata)[this->getDeclFilename(decl)].enums.push_back(info);

    XPARSE_LOG_INFO("handled enum: {0}.", info.full_name);
}

inline ReflectASTConsumer::HandleResult ReflectASTConsumer::handleDecl(clang::EnumConstantDecl* decl, EnumConstantMetaInfo& info)
{
    if (!detail::isMarked(decl)) {
        return kFailure;
    }

    if (this->handleDecl(llvm::cast<clang::NamedDecl>(decl), info) == kFailure) {
        return kFailure;
    }

    info.value = decl->getInitVal().getRawData()[0];

    return kSuccess;
}

} // namespace xparse

#endif // __XPARSE_REFLECT_H__
