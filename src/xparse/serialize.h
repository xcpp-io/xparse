/**
 * *****************************************************************************
 * @file        serialize.h
 * @brief
 * @author      hsz (hszsoftware@qq.com)
 * @date        2024-10-22
 * @copyright   hszsoft
 * *****************************************************************************
 */

#include "meta.h"

#include <llvm/Support/JSON.h>

#ifndef __XPARSE_SERIALIZE_H__
#define __XPARSE_SERIALIZE_H__

#define XPARSE_SERIALIZE_ATTR(NAME)        \
    outs.attributeBegin(#NAME);            \
    Serializer::serialize(outs, ins.NAME); \
    outs.attributeEnd()

#define XPARSE_SERIALIZE_ATTR_FROM_OBJECT(NAME) \
    Serializer::serializeAttrs(outs, static_cast<const NAME&>(ins))

#define XPARSE_SERIALIZE_OBJECT(NAME) \
    template <>                       \
    inline void Serializer::serialize(llvm::json::OStream& outs, const NAME& ins)

#define XPARSE_SERIALIZE_OBJECT_ATTRS(NAME) \
    template <>                             \
    inline void Serializer::serializeAttrs(llvm::json::OStream& outs, const NAME& ins)

namespace xparse {

template <typename... T>
static constexpr bool always_false = false;

template <typename T>
using serializable = std::disjunction<
    std::is_arithmetic<T>,
    std::is_constructible<std::string, T>>;

class Serializer {
public:
    template <typename T>
    static void serialize(llvm::json::OStream& outs, const T& ins)
    {
        if constexpr (serializable<T>::value) {
            outs.value(ins);
        } else {
            static_assert(always_false<T>, "No serialization function available for this type.");
        }
    }

    template <typename T>
    static void serializeAttrs(llvm::json::OStream& /*outs*/, const T& /*ins*/)
    {
        static_assert(always_false<T>, "No serialization function available for this type.");
    }

    template <typename T>
    static void serialize(llvm::json::OStream& outs, const std::vector<T>& ins)
    {
        outs.array([&] {
            for (const auto& element : ins) {
                Serializer::serialize(outs, element);
            }
        });
    }
};

XPARSE_SERIALIZE_OBJECT_ATTRS(MetaInfo)
{
    XPARSE_SERIALIZE_ATTR(name);
    XPARSE_SERIALIZE_ATTR(attrs);
    XPARSE_SERIALIZE_ATTR(comment);
    XPARSE_SERIALIZE_ATTR(access);
}

XPARSE_SERIALIZE_OBJECT(MetaInfo)
{
    outs.object([&] {
        Serializer::serializeAttrs(outs, ins);
    });
}

XPARSE_SERIALIZE_OBJECT_ATTRS(ValueMetaInfo)
{
    XPARSE_SERIALIZE_ATTR_FROM_OBJECT(MetaInfo);
    XPARSE_SERIALIZE_ATTR(type);
    XPARSE_SERIALIZE_ATTR(raw_type);
}

XPARSE_SERIALIZE_OBJECT(ValueMetaInfo)
{
    outs.object([&] {
        Serializer::serializeAttrs(outs, ins);
    });
}

XPARSE_SERIALIZE_OBJECT(FieldMetaInfo)
{
    outs.object([&] {
        XPARSE_SERIALIZE_ATTR_FROM_OBJECT(ValueMetaInfo);
        XPARSE_SERIALIZE_ATTR(is_mutable);
    });
}

XPARSE_SERIALIZE_OBJECT_ATTRS(ParamVarMetaInfo)
{
    XPARSE_SERIALIZE_ATTR_FROM_OBJECT(ValueMetaInfo);
    XPARSE_SERIALIZE_ATTR(is_default);
    XPARSE_SERIALIZE_ATTR(default_value);
}

XPARSE_SERIALIZE_OBJECT(ParamVarMetaInfo)
{
    outs.object([&] {
        Serializer::serializeAttrs(outs, ins);
    });
}

XPARSE_SERIALIZE_OBJECT_ATTRS(VarMetaInfo)
{
    XPARSE_SERIALIZE_ATTR_FROM_OBJECT(ValueMetaInfo);
}

XPARSE_SERIALIZE_OBJECT(VarMetaInfo)
{
    outs.object([&] {
        Serializer::serializeAttrs(outs, ins);
    });
}

XPARSE_SERIALIZE_OBJECT_ATTRS(FunctionMetaInfo)
{
    XPARSE_SERIALIZE_ATTR_FROM_OBJECT(MetaInfo);

    XPARSE_SERIALIZE_ATTR(ret_type);
    XPARSE_SERIALIZE_ATTR(ret_raw_type);
    XPARSE_SERIALIZE_ATTR(params);
}

XPARSE_SERIALIZE_OBJECT(FunctionMetaInfo)
{
    outs.object([&] {
        Serializer::serializeAttrs(outs, ins);
    });
}

XPARSE_SERIALIZE_OBJECT(MethodMetaInfo)
{
    outs.object([&] {
        XPARSE_SERIALIZE_ATTR_FROM_OBJECT(FunctionMetaInfo);
        XPARSE_SERIALIZE_ATTR(is_virtual);
        XPARSE_SERIALIZE_ATTR(is_pure_virtual);
        XPARSE_SERIALIZE_ATTR(is_override);
    });
}

XPARSE_SERIALIZE_OBJECT(RecordMetaInfo)
{
    outs.object([&] {
        XPARSE_SERIALIZE_ATTR_FROM_OBJECT(MetaInfo);
        XPARSE_SERIALIZE_ATTR(bases);
        XPARSE_SERIALIZE_ATTR(fields);
        XPARSE_SERIALIZE_ATTR(methods);
    });
}

XPARSE_SERIALIZE_OBJECT(EnumConstantMetaInfo)
{
    outs.object([&] {
        XPARSE_SERIALIZE_ATTR_FROM_OBJECT(MetaInfo);
        XPARSE_SERIALIZE_ATTR(value);
    });
}

XPARSE_SERIALIZE_OBJECT(EnumMetaInfo)
{
    outs.object([&] {
        XPARSE_SERIALIZE_ATTR_FROM_OBJECT(MetaInfo);
        XPARSE_SERIALIZE_ATTR(constants);
    });
}

XPARSE_SERIALIZE_OBJECT(FileMetaInfo)
{
    outs.object([&] {
        XPARSE_SERIALIZE_ATTR(records);
        XPARSE_SERIALIZE_ATTR(variables);
        XPARSE_SERIALIZE_ATTR(functions);
        XPARSE_SERIALIZE_ATTR(enums);
    });
}

} // namespace xparse

#endif // __XPARSE_SERIALIZE_H__