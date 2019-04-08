#pragma once

#include "FileDescriptor.h"
#include "Blob.h"

#include <variant>
#include <tuple>
#include <string>
#include <cstdint>

namespace TxFs
{
namespace Private
{
    template <typename Tuple>
    struct ToVariant;

    template <typename... Args>
    struct ToVariant<std::tuple<Args...>>
    {
        using Variant = std::variant<Args...>;
    };
}

//////////////////////////////////////////////////////////////////////////

enum class Folder : uint32_t;

//////////////////////////////////////////////////////////////////////////

struct Version
{
    uint32_t m_major = 0;
    uint32_t m_minor = 0;
    uint32_t m_patch = 0;

    bool operator==(Version rhs) const
    {
        return std::tie(m_major, m_minor, m_patch) == std::tie(rhs.m_major, rhs.m_minor, rhs.m_patch);
    }

    bool operator<(Version rhs) const
    {
        return std::tie(m_major, m_minor, m_patch) < std::tie(rhs.m_major, rhs.m_minor, rhs.m_patch);
    }
};

//////////////////////////////////////////////////////////////////////////

class BlobTransformation
{
    inline static char const* TypeNames[]
        = { "File", "Folder", "Version", "Double", "DoublePair", "Int", "String", "Undefined" /*last entry!*/ };

public:
    enum class TypeEnum : uint8_t {
        File,
        Folder,
        Version,
        Double,
        DoublePair,
        Int,
        String,
        Undefined /*last entry!*/
    };
    using Types = std::tuple<FileDescriptor, Folder, Version, double, std::pair<double, double>, int32_t, std::string>;
    using Variant = Private::ToVariant<Types>::Variant;

    template <typename T>
    static Blob toBlob(const T& value)
    {
        return variantToBlob(value);
    }

    static TypeEnum getBlobType(const BlobRef& blob);
    static std::string getBlobTypeName(const BlobRef& blob);
    static Variant toVariant(const BlobRef& blob);
    template <typename T>
    static T toValue(const BlobRef& blob)
    {
        return std::get<T>(toVariant(blob));
    }

private:
    static Blob variantToBlob(const Variant& v);
};

//////////////////////////////////////////////////////////////////////////

using TransformationTypeEnum = BlobTransformation::TypeEnum;

}
