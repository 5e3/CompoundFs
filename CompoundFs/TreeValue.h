
#pragma once

#include <string>
#include <string_view>
#include <variant>
#include <type_traits>
#include <array>

#include "ByteString.h"
#include "FileDescriptor.h"

namespace TxFs
{

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

class TreeValue
{
    struct Unknown
    {};
    using Variant = std::variant<FileDescriptor, Folder, Version, double, int, std::string, Unknown>;

    template <typename T>
    using EnableVariantTypes = std::enable_if_t<std::is_convertible_v<T, Variant>>;

public:
    enum class Type { File, Folder, Version, Double, Int, String, Unknown };

public:
    TreeValue()
        : m_variant(Unknown())
    {}

    template <typename T, typename = EnableVariantTypes<T>>
    TreeValue(T&& val)
        : m_variant(val)
    {}

    template<typename T, typename = EnableVariantTypes>
    bool operator==(T&& rhs) { return m_variant == m_variant; }

    std::string_view getTypeName() const;
    Type getType() const { return static_cast<Type>(m_variant.index()); }

    template <typename T>
    T toValue() const
    {
        return std::get<T>(m_variant);
    }

    void toStream(ByteStringStream& bss) const;
    static TreeValue fromStream(ByteStringView bsv);

private:
    Variant m_variant;
};

}
