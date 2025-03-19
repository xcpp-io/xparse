/**
 * *****************************************************************************
 * @file        serialize.h
 * @brief
 * @author      hsz (hszsoftware@qq.com)
 * @date        2024-10-22
 * @copyright   hszsoft
 * *****************************************************************************
 */

#include <llvm/Support/JSON.h>

#ifndef __XPARSE_SERIALIZE_H__
#define __XPARSE_SERIALIZE_H__

#define XPARSE_SERIALIZE_ATTR(NAME)        \
    outs.attributeBegin(#NAME);            \
    Serializer::serialize(outs, ins.NAME); \
    outs.attributeEnd()

#define XPARSE_SERIALIZE_ATTR_FROM_OBJECT(NAME) \
    Serializer::serializeAttrs(outs, static_cast<const NAME&>(ins))

#define XPARSE_SERIALIZE_OBJECT(NAME)                                               \
    template <>                                                                     \
    void Serializer::serializeAttrs(llvm::json::OStream& outs, const NAME& ins);    \
    template <>                                                                     \
    inline void Serializer::serialize(llvm::json::OStream& outs, const NAME& ins) { \
        outs.object([&] { Serializer::serializeAttrs(outs, ins); });                \
    }                                                                               \
    template <>                                                                     \
    inline void Serializer::serializeAttrs(llvm::json::OStream& outs, const NAME& ins)

    namespace xparse
{

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

} // namespace xparse

#endif // __XPARSE_SERIALIZE_H__
