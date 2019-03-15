
#include "BlobTransformation.h"

using namespace TxFs;

namespace
{
//////////////////////////////////////////////////////////////////////////

    struct FixedBlob : BlobRef
{
    uint8_t m_buffer[UINT8_MAX];
    uint8_t* m_iterator;

    FixedBlob()
        : m_iterator(m_buffer + 1)
    {
        m_buffer[0] = 0;
        m_data = m_buffer;
    }

    void pushBack(const uint8_t* valBegin, const uint8_t* valEnd)
    {
        assert(valEnd >= valBegin);
        uint8_t* endIterator = m_iterator + (valEnd - valBegin);
        if (endIterator > (m_buffer + sizeof(m_buffer)))
            throw std::runtime_error("Data exceeds max Blob size");
        m_iterator = std::copy(valBegin, valEnd, m_iterator);
        m_data[0] = uint8_t(m_iterator - begin() - 1);
    }

    template <typename T>
    void pushBack(const T& value)
    {
        auto begin = (uint8_t*) &value;
        auto end = begin + sizeof(T);
        pushBack(begin, end);
    }
};

//////////////////////////////////////////////////////////////////////////

template <typename T>
void valueToBlob(FixedBlob& blob, const T& value)
{
    blob.pushBack(value);
}

void valueToBlob(FixedBlob& blob, const std::string& value)
{
    auto begin = (const uint8_t*) &value[0];
    blob.pushBack(begin, begin + value.size());
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
BlobTransformation::Variant BlobToVariant(const BlobRef& blob)
{
    T value;
    std::copy(blob.begin() + 2, blob.end(), (uint8_t*)&value);
    return BlobTransformation::Variant(value);
}

template <>
BlobTransformation::Variant BlobToVariant<std::string>(const BlobRef& blob)
{
    auto begin = (const char*)blob.begin() + 2;
    auto end = (const char*)blob.end();
    std::string s(begin, end);
    return BlobTransformation::Variant(std::move(s));
}

using ToVariantFunc = BlobTransformation::Variant (*)(const BlobRef&);

template <typename T>
struct ToVariantFuncArray;

template <typename... Args>
struct ToVariantFuncArray<std::tuple<Args...>>
{
    inline static const ToVariantFunc g_funcs[] = { BlobToVariant<Args>... };
};

}

//////////////////////////////////////////////////////////////////////////

Blob BlobTransformation::variantToBlob(const Variant& v)
{
    auto idx = static_cast<BlobTransformation::TypeEnum>(v.index());
    return std::visit(
        [idx = idx](const auto& value) {
            FixedBlob blob;
            blob.pushBack(idx);
            valueToBlob(blob, value);
            return Blob(blob);
        },
        v);
}

BlobTransformation::TypeEnum BlobTransformation::getBlobType(const BlobRef& blob)
{
    if (blob.size() < 2)
        return TypeEnum::Undefined;
    auto type = *(blob.begin() + 1);
    type = std::min(type, uint8_t(TypeEnum::Undefined));
    return TypeEnum(type);
}

std::string BlobTransformation::getBlobTypeName(const BlobRef& blob)
{
    auto idx = int(getBlobType(blob));
    return TypeNames[idx];
}

BlobTransformation::Variant BlobTransformation::toVariant(const BlobRef& blob)
{
    return ToVariantFuncArray<BlobTransformation::Types>::g_funcs[*(blob.begin() + 1)](blob);
}
