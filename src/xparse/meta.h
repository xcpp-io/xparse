/**
 * *****************************************************************************
 * @file        meta.h
 * @brief
 * @author      hsz (hszsoftware@qq.com)
 * @date        2024-10-22
 * @copyright   hszsoft
 * *****************************************************************************
 */

#ifndef __XPARSE_META_H__
#define __XPARSE_META_H__

#include <string>
#include <unordered_map>
#include <vector>

#define XPARSE_DEFAULT_MEMBER(CLASS_NAME)               \
    CLASS_NAME(const CLASS_NAME&) = default;            \
    CLASS_NAME(CLASS_NAME&&) = default;                 \
    CLASS_NAME& operator=(const CLASS_NAME&) = default; \
    CLASS_NAME& operator=(CLASS_NAME&&) = default;

namespace xparse {

/**
 * @brief       Base of all meta info
 * 
 */
struct MetaInfo {
    std::string name;
    std::vector<std::string> attrs;
    std::string comment;
    std::string access;
};

/**
 * @brief       Store type info of variables
 * 
 */
struct ValueMetaInfo : MetaInfo {
    std::string type;
    std::string raw_type;
};

struct FieldMetaInfo : ValueMetaInfo {
    bool is_mutable = false;
};

struct ParamVarMetaInfo : ValueMetaInfo {
    bool is_default = false;
    std::string default_value;
};

struct VarMetaInfo : ValueMetaInfo {
};

struct FunctionMetaInfo : MetaInfo {
    std::string ret_type;
    std::string ret_raw_type;
    std::vector<ParamVarMetaInfo> params;
};

struct MethodMetaInfo : FunctionMetaInfo {
    bool is_virtual = false;
    bool is_pure_virtual = false;
    bool is_override = false;
};

/**
 * @brief       Store meta info for class, struct and union.
 * 
 */
struct RecordMetaInfo : MetaInfo {
    std::vector<std::string> bases;
    std::vector<FieldMetaInfo> fields;
    std::vector<MethodMetaInfo> methods;
};

struct EnumConstantMetaInfo : MetaInfo {
    uint64_t value {};
};

struct EnumMetaInfo : MetaInfo {
    std::vector<EnumConstantMetaInfo> constants;
};

/**
 * @brief       Stores meta information for a single file.
 * @note        In Clang, variables are stored using VarDecl instead of ValueDecl.
 *              However, since we only care about their type information, we simplify this distinction.
 */
struct FileMetaInfo {
    std::vector<RecordMetaInfo> records;
    std::vector<VarMetaInfo> variables;
    std::vector<FunctionMetaInfo> functions;
    std::vector<EnumMetaInfo> enums;
};

inline bool isMetadataEmpty(const FileMetaInfo& metadata)
{
    return metadata.records.empty() && metadata.variables.empty() && metadata.functions.empty() && metadata.enums.empty();
}

using ProjectMetaInfo = std::unordered_map<std::string, FileMetaInfo>;

} // namespace xparse

#endif // __XPARSE_META_H__