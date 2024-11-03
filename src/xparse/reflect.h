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
#include "mark.h"
#include "meta.h"

#include <clang/AST/ASTConsumer.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/JSON.h>

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

    inline MarkDatabase loadMarksFromFiles(const std::vector<std::string>& filepaths)
    {
        MarkDatabase marks;

        for (const auto& filepath : filepaths) {
            llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> file_or_err = llvm::MemoryBuffer::getFile(filepath);
            if (std::error_code err_code = file_or_err.getError()) {
                XPARSE_LOG_ERROR("error reading file {0} while loading marks, reason: {1}.",
                    filepath, err_code.message());
                continue;
            }

            llvm::Expected<llvm::json::Value> json_or_err = llvm::json::parse(file_or_err.get()->getBuffer());
            if (!json_or_err) {
                llvm::handleAllErrors(json_or_err.takeError(), [filepath](const llvm::ErrorInfoBase& ei) {
                    XPARSE_LOG_ERROR("JSON parse error, file: {0}, reason: {1}.", filepath, ei.message());
                });
                continue;
            }

            llvm::json::Value json_value = json_or_err.get();
            auto* mark_obj = json_value.getAsObject();
            if (!mark_obj) {
                XPARSE_LOG_ERROR("expected an object in file: {0}.", filepath);
                continue;
            }

            for (auto& [name_obj, attrs_obj] : *mark_obj) {

                Attrs attrs;
                if (auto* attrs_arr = attrs_obj.getAsArray()) {
                    for (const auto& attr_obj : *attrs_arr) {
                        if (auto attr_str = attr_obj.getAsString()) {
                            attrs.push_back(attr_str->str());
                        }
                    }
                }
                marks[removeAllSpaces(name_obj.str())] = attrs;
            }
        }
        return marks;
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

    inline std::string getClassTemplateAppendedArgs(clang::ClassTemplateSpecializationDecl* decl)
    {
        std::string appended_name = "<";
        const auto& template_args = decl->getTemplateArgs();
        for (unsigned i = 0; i < template_args.size(); ++i) {
            const auto& arg = template_args[i];

            std::string arg_str;
            if (arg.getKind() == clang::TemplateArgument::Type) {
                arg_str = arg.getAsType().getAsString();
            } else if (arg.getKind() == clang::TemplateArgument::Integral) {
                LimitedString integral_str;
                arg.getAsIntegral().toString(integral_str, 10);
                arg_str = integral_str.str();
            }

            appended_name += arg_str;
            if (i < template_args.size() - 1) {
                appended_name += ",";
            }
        }
        appended_name += ">";

        return appended_name;
    }

} // namespace detail

class ReflectASTConsumer : public clang::ASTConsumer {
public:
    ReflectASTConsumer(ProjectMetaInfo& metadata,
        std::string root,
        const std::vector<std::string>& marks)
        : m_metadata(&metadata)
        , m_root(std::move(root))
    {
        m_marks = detail::loadMarksFromFiles(marks);
    }

    /**
     * @brief       LLVM's interface
     *
     * @param       ctx
     */
    void HandleTranslationUnit(clang::ASTContext& ctx) override;

protected:
    bool isMarked(const std::string& name);

    std::string getFilename(clang::NamedDecl* decl);

    /**
     * @brief       Fill the following info: name, access, comment, attrs.
     *              If "full_name" isn't assigned in advance, then attrs will not be filled in.
     *              This is because different decls may have different ways of obtaining "full_name".
     */
    void fillBaseInfo(MetaInfo* info, clang::NamedDecl* decl);
    void fillValueInfo(ValueMetaInfo* info, clang::ValueDecl* decl);
    void fillFunctionInfo(FunctionMetaInfo* info, clang::FunctionDecl* decl);
    void fillRecordInfo(RecordMetaInfo* info, clang::CXXRecordDecl* decl);
    void fillFieldInfo(FieldMetaInfo* info, clang::FieldDecl* decl);

    void handleDecl(clang::NamespaceDecl* decl);

    void handleDecl(clang::CXXRecordDecl* decl);
    void handleDecl(RecordMetaInfo* info, clang::FieldDecl* decl);
    void handleDecl(RecordMetaInfo* info, clang::CXXMethodDecl* decl);
    void handleDecl(RecordMetaInfo* info, clang::IndirectFieldDecl* decl);

    void handleDecl(clang::VarDecl* decl);

    void handleDecl(clang::FunctionDecl* decl);
    void handleDecl(FunctionMetaInfo* info, clang::ParmVarDecl* decl);

    void handleDecl(clang::EnumDecl* decl);
    void handleDecl(EnumMetaInfo* info, clang::EnumConstantDecl* decl);

    void handleDecl(clang::ClassTemplateSpecializationDecl* decl);

private:
    ProjectMetaInfo* m_metadata;
    std::string m_root;
    MarkDatabase m_marks;

    clang::ASTContext* m_context {};
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
        case clang::Decl::Var:
            this->handleDecl(llvm::cast<clang::VarDecl>(named_decl));
            break;
        case clang::Decl::Function:
            this->handleDecl(llvm::cast<clang::FunctionDecl>(named_decl));
            break;
        case clang::Decl::ClassTemplateSpecialization:
            this->handleDecl(llvm::cast<clang::ClassTemplateSpecializationDecl>(named_decl));
            break;
        default:
            break;
        }
    }
}

inline bool ReflectASTConsumer::isMarked(const std::string& name)
{
    return m_marks.find(name) != m_marks.end();
}

inline std::string ReflectASTConsumer::getFilename(clang::NamedDecl* decl)
{
    std::string result;

    auto location = m_context->getSourceManager().getPresumedLoc(decl->getLocation());
    if (!location.isInvalid()) {
        std::filesystem::path filepath(clang::tooling::getAbsolutePath(location.getFilename()));
        filepath = std::filesystem::canonical(filepath);
        result = std::filesystem::relative(filepath, m_root).string();
    }
    return result;
}

inline void ReflectASTConsumer::fillBaseInfo(MetaInfo* info, clang::NamedDecl* decl)
{
    info->name = decl->getNameAsString();
    info->access = detail::getMemberAccess(decl);

    auto* raw_comment = m_context->getRawCommentForDeclNoCache(decl);
    if (raw_comment) {
        info->comment = raw_comment->getBriefText(*m_context);
    }
    if (m_marks.find(info->full_name) != m_marks.end()) {
        info->attrs = m_marks.at(info->full_name);
    }
}

inline void ReflectASTConsumer::fillValueInfo(ValueMetaInfo* info, clang::ValueDecl* decl)
{
    this->fillBaseInfo(info, decl);

    info->type = decl->getType().getAsString();
    info->raw_type = decl->getType().getCanonicalType().getAsString();
}

inline void ReflectASTConsumer::fillFunctionInfo(FunctionMetaInfo* info, clang::FunctionDecl* decl)
{
    this->fillBaseInfo(info, decl);

    info->ret_type = decl->getReturnType().getAsString();
    info->ret_raw_type = decl->getReturnType().getCanonicalType().getAsString();

    for (auto* param_decl : decl->parameters()) {
        this->handleDecl(info, param_decl);
    }
}

inline void ReflectASTConsumer::fillRecordInfo(RecordMetaInfo* info, clang::CXXRecordDecl* decl)
{
    this->fillBaseInfo(info, decl);

    auto* record_decl = llvm::dyn_cast<clang::CXXRecordDecl>(decl);
    if (!record_decl->isThisDeclarationADefinition()) {
        return; // take out cases with only declarations
    }
    if (record_decl->isAnonymousStructOrUnion()) {
        return; // take out cases with anonymous record
    }
    if (record_decl->isUnion()) {
        return; // we didn't support union.
    }

    for (const auto& base : record_decl->bases()) {
        auto* base_decl = base.getType()->getAsCXXRecordDecl();
        if (base_decl) {
            info->bases.push_back(base_decl->getQualifiedNameAsString());
        }
    }

    for (auto* child_decl : record_decl->decls()) {
        switch (child_decl->getKind()) {
        case clang::Decl::Field:
            this->handleDecl(info, llvm::cast<clang::FieldDecl>(child_decl));
            break;
        case clang::Decl::CXXMethod:
            this->handleDecl(info, llvm::cast<clang::CXXMethodDecl>(child_decl));
            break;
        case clang::Decl::Var:
            this->handleDecl(llvm::cast<clang::VarDecl>(child_decl));
            break;
        case clang::Decl::IndirectField:
            this->handleDecl(info, llvm::cast<clang::IndirectFieldDecl>(child_decl));
            break;
        case clang::Decl::CXXRecord:
            this->handleDecl(llvm::cast<clang::CXXRecordDecl>(child_decl));
            break;
        case clang::Decl::Enum:
            this->handleDecl(llvm::cast<clang::EnumDecl>(child_decl));
            break;
        case clang::Decl::ClassTemplateSpecialization:
            this->handleDecl(llvm::cast<clang::ClassTemplateSpecializationDecl>(child_decl));
            break;
        default:
            break;
        }
    }
}

inline void ReflectASTConsumer::fillFieldInfo(FieldMetaInfo* info, clang::FieldDecl* decl)
{
    this->fillValueInfo(info, decl);
    
    info->is_mutable = decl->isMutable();
}

inline void ReflectASTConsumer::handleDecl(clang::NamespaceDecl* decl)
{
    if (!decl || decl->isInvalidDecl()) {
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
        case clang::Decl::Var:
            this->handleDecl(llvm::cast<clang::VarDecl>(child_decl));
            break;
        case clang::Decl::Function:
            this->handleDecl(llvm::cast<clang::FunctionDecl>(child_decl));
            break;
        case clang::Decl::ClassTemplateSpecialization:
            this->handleDecl(llvm::cast<clang::ClassTemplateSpecializationDecl>(child_decl));
            break;
        default:
            break;
        }
    }
}

inline void ReflectASTConsumer::handleDecl(clang::CXXRecordDecl* decl)
{
    if (!decl || decl->isInvalidDecl()) {
        return;
    }

    std::string full_name = detail::removeAllSpaces(decl->getQualifiedNameAsString());
    if (!isMarked(full_name)) {
        return;
    }

    RecordMetaInfo record_info;
    record_info.full_name = full_name;
    this->fillRecordInfo(&record_info, decl);

    (*m_metadata)[this->getFilename(decl)].records.push_back(record_info);
}

inline void ReflectASTConsumer::handleDecl(RecordMetaInfo* info, clang::FieldDecl* decl)
{
    if (!decl || decl->isInvalidDecl()) {
        return;
    }

    std::string full_name = detail::removeAllSpaces(decl->getQualifiedNameAsString());
    if (!isMarked(full_name)) {
        return;
    }

    FieldMetaInfo field_info;
    field_info.full_name = full_name;
    this->fillFieldInfo(&field_info, decl);

    info->fields.push_back(field_info);
}

inline void ReflectASTConsumer::handleDecl(RecordMetaInfo* info, clang::CXXMethodDecl* decl)
{
    if (!decl || decl->isInvalidDecl()) {
        return;
    }

    std::string full_name = detail::removeAllSpaces(decl->getQualifiedNameAsString());
    if (!isMarked(full_name)) {
        return;
    }

    // put static functions and global functions together
    if (decl->isStatic()) {
        FunctionMetaInfo function_info;
        function_info.full_name = full_name;
        this->fillFunctionInfo(&function_info, decl);

        (*m_metadata)[this->getFilename(decl)].functions.push_back(function_info);
    } else {
        MethodMetaInfo method_info;
        method_info.full_name = full_name;
        this->fillFunctionInfo(&method_info, decl);

        method_info.is_virtual = decl->isVirtual();
        method_info.is_pure_virtual = decl->isPure();
        method_info.is_override = decl->size_overridden_methods() > 0;

        info->methods.push_back(method_info);
    }
}

inline void ReflectASTConsumer::handleDecl(clang::VarDecl* decl)
{
    if (!decl || decl->isInvalidDecl()) {
        return;
    }

    std::string full_name = detail::removeAllSpaces(decl->getQualifiedNameAsString());
    if (!isMarked(full_name)) {
        return;
    }

    ValueMetaInfo var_info;
    var_info.full_name = full_name;
    this->fillValueInfo(&var_info, decl);

    (*m_metadata)[this->getFilename(decl)].variables.push_back(var_info);
}

inline void ReflectASTConsumer::handleDecl(RecordMetaInfo* info, clang::IndirectFieldDecl* decl)
{
    if (!decl || decl->isInvalidDecl()) {
        return;
    }

    std::string full_name = detail::removeAllSpaces(decl->getQualifiedNameAsString());
    if (!isMarked(full_name)) {
        return;
    }

    if (auto* field_decl = decl->getAnonField()) {
        FieldMetaInfo field_info;
        field_info.full_name = full_name;
        this->fillFieldInfo(&field_info, field_decl);

        info->fields.push_back(field_info);
    } else if (auto* var_decl = decl->getVarDecl()) {
        ValueMetaInfo var_info;
        var_info.full_name = full_name;
        this->fillValueInfo(&var_info, decl);

        (*m_metadata)[this->getFilename(decl)].variables.push_back(var_info);
    }
}

inline void ReflectASTConsumer::handleDecl(clang::FunctionDecl* decl)
{
    if (!decl || decl->isInvalidDecl()) {
        return;
    }

    std::string full_name = detail::removeAllSpaces(decl->getQualifiedNameAsString());
    if (!isMarked(full_name)) {
        return;
    }

    FunctionMetaInfo function_info;
    function_info.full_name = full_name;
    this->fillFunctionInfo(&function_info, decl);

    (*m_metadata)[this->getFilename(decl)].functions.push_back(function_info);
}

inline void ReflectASTConsumer::handleDecl(FunctionMetaInfo* info, clang::ParmVarDecl* decl)
{
    // we don't need to check if the function parameters are marked.
    if (!decl || decl->isInvalidDecl()) {
        return;
    }

    ParamMetaInfo param_info;
    param_info.full_name = detail::removeAllSpaces(decl->getQualifiedNameAsString());
    this->fillValueInfo(&param_info, decl);

    param_info.index = decl->getFunctionScopeIndex();

    // NOTE: fill attrs for anonymous param.
    if (param_info.name.empty()) {
        std::string anonymous_name = info->name + "::" + std::to_string(param_info.index);
        if (m_marks.find(anonymous_name) != m_marks.end()) {
            param_info.attrs = m_marks.at(anonymous_name);
        }
    }
    
    param_info.is_default = decl->hasDefaultArg();
    if (param_info.is_default) {
        const clang::Expr* default_arg = decl->getDefaultArg();
        std::string defaultValue;
        llvm::raw_string_ostream rso(defaultValue);
        default_arg->printPretty(rso, nullptr, clang::PrintingPolicy(clang::LangOptions()));
        param_info.default_value = rso.str();
    }

    info->params.push_back(param_info);
}

inline void ReflectASTConsumer::handleDecl(clang::EnumDecl* decl)
{
    if (!decl || decl->isInvalidDecl()) {
        return;
    }

    std::string full_name = detail::removeAllSpaces(decl->getQualifiedNameAsString());
    if (!isMarked(full_name)) {
        return;
    }

    EnumMetaInfo enum_info;
    enum_info.full_name = full_name;
    this->fillBaseInfo(&enum_info, decl);

    for (auto* constant_decl : decl->enumerators()) {
        this->handleDecl(&enum_info, constant_decl);
    }
    
    (*m_metadata)[this->getFilename(decl)].enums.push_back(enum_info);
}

inline void ReflectASTConsumer::handleDecl(EnumMetaInfo* info, clang::EnumConstantDecl* decl)
{
    // we don't need to check if the enum constants are marked.
    if (!decl || decl->isInvalidDecl()) {
        return;
    }

    EnumConstantMetaInfo constant_info;
    constant_info.full_name = detail::removeAllSpaces(decl->getQualifiedNameAsString());
    this->fillBaseInfo(&constant_info, decl);

    constant_info.value = decl->getInitVal().getRawData()[0];

    info->constants.push_back(constant_info);
}

inline void ReflectASTConsumer::handleDecl(clang::ClassTemplateSpecializationDecl* decl)
{
    if (!decl || decl->isInvalidDecl()) {
        return;
    }

    std::string template_args_str = detail::getClassTemplateAppendedArgs(decl);
    std::string full_name = detail::removeAllSpaces(decl->getQualifiedNameAsString() + template_args_str);
    if (m_marks.find(full_name) == m_marks.end()) {
        return;
    }

    RecordMetaInfo record_info;
    record_info.full_name = full_name;
    this->fillRecordInfo(&record_info, decl);

    record_info.name += template_args_str;

    (*m_metadata)[this->getFilename(decl)].records.push_back(record_info);
}

} // namespace xparse

#endif // __XPARSE_REFLECT_H__