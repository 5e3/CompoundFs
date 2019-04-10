
#include "DirectoryObjects.h"

using namespace TxFs;

namespace
{
//////////////////////////////////////////////////////////////////////////

template <typename T>
void valueToByteString(MutableByteString& blob, const T& value)
{
    blob.pushBack(value);
}

void valueToByteString(MutableByteString& blob, const std::string& value)
{
    auto begin = (const uint8_t*) &value[0];
    blob.pushBack(begin, begin + value.size());
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
ByteStringOps::Variant byteStringToVariant(const ByteStringView& blob)
{
    T value;
    std::copy(blob.begin() + 2, blob.end(), (uint8_t*) &value);
    return ByteStringOps::Variant(value);
}

template <>
ByteStringOps::Variant byteStringToVariant<std::string>(const ByteStringView& blob)
{
    auto begin = (const char*) blob.begin() + 2;
    auto end = (const char*) blob.end();
    std::string s(begin, end);
    return ByteStringOps::Variant(std::move(s));
}

using ToVariantFunc = ByteStringOps::Variant (*)(const ByteStringView&);

template <typename T>
struct ToVariantFuncArray;

template <typename... Args>
struct ToVariantFuncArray<std::tuple<Args...>>
{
    inline static const ToVariantFunc g_funcs[] = { byteStringToVariant<Args>... };
};

}

//////////////////////////////////////////////////////////////////////////

ByteString ByteStringOps::variantToByteString(const Variant& v)
{
    auto idx = static_cast<ByteStringOps::TypeEnum>(v.index());
    return std::visit(
        [idx = idx](const auto& value) {
            MutableByteString blob;
            blob.pushBack(idx);
            valueToByteString(blob, value);
            return ByteString(blob);
        },
        v);
}

ByteStringOps::TypeEnum ByteStringOps::getType(const ByteStringView& blob)
{
    if (blob.size() < 2)
        return TypeEnum::Undefined;
    auto type = *(blob.begin() + 1);
    type = std::min(type, uint8_t(TypeEnum::Undefined));
    return TypeEnum(type);
}

std::string ByteStringOps::getTypeName(const ByteStringView& blob)
{
    auto idx = int(getType(blob));
    return TypeNames[idx];
}

ByteStringOps::Variant ByteStringOps::toVariant(const ByteStringView& blob)
{
    return ToVariantFuncArray<ByteStringOps::Types>::g_funcs[*(blob.begin() + 1)](blob);
}
