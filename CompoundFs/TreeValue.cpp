

#include "TreeValue.h"

using namespace TxFs;

namespace
{

template <typename... Ts> struct overload : Ts... { using Ts::operator()...; };
template <class... Ts> overload(Ts...) -> overload<Ts...>;

template<typename... Ts>
void defaultCreate(size_t index, std::variant<Ts...>& var)
{
    index = std::min(index, size_t(TreeValue::Type::Unknown));
    static std::array<std::variant<Ts...>, sizeof...(Ts)> defaultCreatedTypes { Ts()... };
    var = defaultCreatedTypes.at(index);
}

}

std::string_view TreeValue::getTypeName() const
{
    constexpr std::array<std::string_view, std::variant_size_v<Variant>> ar = 
    { "File", "Folder", "Version", "Double", "Int64", "Int32", "String", "Unknown" };

    static_assert(!ar.at(std::variant_size_v<Variant> - 1).empty(), "Missing type name?");
    return ar.at(m_variant.index());
}

void TreeValue::toStream(ByteStringStream& bss) const
{
    static_assert(sizeof(FileDescriptor) == 16);
    static_assert(sizeof(Version) == 12);

    auto index = static_cast<uint8_t>(m_variant.index());
    bss.push(index);
    std::visit(overload
    { 
        [bss = &bss](const std::string& val) { bss->push(ByteStringView(val)); },
        [bss = &bss](const auto& val) { bss->push(val); }
    }, m_variant);
}

TreeValue TreeValue::fromStream(ByteStringView bsv)
{
    uint8_t index = 0;
    bsv = ByteStringStream::pop(index, bsv);
    TreeValue tv;
    defaultCreate(index, tv.m_variant);
    std::visit(overload 
    { 
        [bsv](std::string& val) { val = std::string(bsv.data(), bsv.end()); }, 
        [bsv](auto& val) { ByteStringStream::pop(val, bsv); }
    }
    ,tv.m_variant);

    return tv;
}

