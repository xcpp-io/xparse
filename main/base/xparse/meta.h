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

#include "serialize.h"

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
 * @brief       Base of all meta info.
 * 
 */
struct MetaInfo {
    std::string name;
    std::string full_name;
    std::vector<std::string> attrs;
    std::string access;
    std::string comment;
};

XPARSE_SERIALIZE_OBJECT(MetaInfo)
{
    XPARSE_SERIALIZE_ATTR(name);
    XPARSE_SERIALIZE_ATTR(full_name);
    XPARSE_SERIALIZE_ATTR(attrs);
    XPARSE_SERIALIZE_ATTR(access);
    XPARSE_SERIALIZE_ATTR(comment);
}

/**
 * @brief       Store type info of variables.
 * 
 */
struct ValueMetaInfo : MetaInfo {
    std::string type;
    std::string raw_type;
    std::string default_value;
};

XPARSE_SERIALIZE_OBJECT(ValueMetaInfo)
{
    XPARSE_SERIALIZE_ATTR_FROM_OBJECT(MetaInfo);
    XPARSE_SERIALIZE_ATTR(type);
    XPARSE_SERIALIZE_ATTR(raw_type);
    XPARSE_SERIALIZE_ATTR(default_value);
}

struct FieldMetaInfo : ValueMetaInfo {
    bool is_static = false;
};

XPARSE_SERIALIZE_OBJECT(FieldMetaInfo)
{
    XPARSE_SERIALIZE_ATTR_FROM_OBJECT(ValueMetaInfo);
    XPARSE_SERIALIZE_ATTR(is_static);
}

struct FunctionMetaInfo : MetaInfo {
    std::string ret_type;
    std::string ret_raw_type;
    std::vector<ValueMetaInfo> params;
    bool is_static;
};

XPARSE_SERIALIZE_OBJECT(FunctionMetaInfo)
{
    XPARSE_SERIALIZE_ATTR_FROM_OBJECT(MetaInfo);
    XPARSE_SERIALIZE_ATTR(ret_type);
    XPARSE_SERIALIZE_ATTR(ret_raw_type);
    XPARSE_SERIALIZE_ATTR(params);
    XPARSE_SERIALIZE_ATTR(is_static);
}

struct MethodMetaInfo : FunctionMetaInfo {
    bool is_virtual = false;
    bool is_pure_virtual = false;
    bool is_override = false;
};

XPARSE_SERIALIZE_OBJECT(MethodMetaInfo)
{
    XPARSE_SERIALIZE_ATTR_FROM_OBJECT(FunctionMetaInfo);
    XPARSE_SERIALIZE_ATTR(is_virtual);
    XPARSE_SERIALIZE_ATTR(is_pure_virtual);
    XPARSE_SERIALIZE_ATTR(is_override);
}

/**
 * @brief       Store meta info for class, struct and union.
 * 
 */
struct RecordMetaInfo : MetaInfo {
    std::vector<std::string> bases;
    std::vector<FieldMetaInfo> fields;
    std::vector<MethodMetaInfo> methods;
};

XPARSE_SERIALIZE_OBJECT(RecordMetaInfo)
{
    XPARSE_SERIALIZE_ATTR_FROM_OBJECT(MetaInfo);
    XPARSE_SERIALIZE_ATTR(bases);
    XPARSE_SERIALIZE_ATTR(fields);
    XPARSE_SERIALIZE_ATTR(methods);
}

struct EnumConstantMetaInfo : MetaInfo {
    uint64_t value {};
};

XPARSE_SERIALIZE_OBJECT(EnumConstantMetaInfo)
{
    XPARSE_SERIALIZE_ATTR_FROM_OBJECT(MetaInfo);
    XPARSE_SERIALIZE_ATTR(value);
}

struct EnumMetaInfo : MetaInfo {
    std::vector<EnumConstantMetaInfo> constants;
};

XPARSE_SERIALIZE_OBJECT(EnumMetaInfo)
{
    XPARSE_SERIALIZE_ATTR_FROM_OBJECT(MetaInfo);
    XPARSE_SERIALIZE_ATTR(constants);
}

/**
 * @brief       Stores meta information for a single file.
 * @note        In Clang, variables are stored using VarDecl instead of ValueDecl.
 *              However, since we only care about their type information, we simplify this distinction.
 */
struct FileMetaInfo {
    std::string path;
    std::vector<RecordMetaInfo> records;
    std::vector<FunctionMetaInfo> functions;
    std::vector<EnumMetaInfo> enums;
};

XPARSE_SERIALIZE_OBJECT(FileMetaInfo)
{
    XPARSE_SERIALIZE_ATTR(path);
    XPARSE_SERIALIZE_ATTR(records);
    XPARSE_SERIALIZE_ATTR(functions);
    XPARSE_SERIALIZE_ATTR(enums);
}

inline bool isMetadataEmpty(const FileMetaInfo& metadata)
{
    return metadata.records.empty() && metadata.functions.empty() && metadata.enums.empty();
}

using ProjectMetaInfo = std::unordered_map<std::string, FileMetaInfo>;

} // namespace xparse

#endif // __XPARSE_META_H__
