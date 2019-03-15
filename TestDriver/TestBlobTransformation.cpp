
#include "stdafx.h"
#include "Test.h"
#include "../CompoundFs/BlobTransformation.h"

using namespace TxFs;

namespace
{
//     using Types = std::tuple<FileDescriptor, Folder, Version, double, std::pair<double, double>, int32_t,
//     std::string>;

BlobTransformation::Types g_values
    = { { 1, 2, 100 }, { 1 }, { 1, 2, 3 }, { 1.5 }, { std::make_pair(1., 2.) }, { 5 }, { "test" } };

template <TransformationTypeEnum IType>
struct TransformationTester
{
    using Type = typename std::tuple_element<size_t(IType), BlobTransformation::Types>::type;
    Type m_value = std::get<Type>(g_values);
    TransformationTypeEnum m_enum = IType;
    Type get(const BlobRef& blob) { return std::get<Type>(BlobTransformation::toVariant(blob)); }
    std::string getTypeName(const BlobRef& blob) const { return BlobTransformation::getBlobTypeName(blob); }
};

template <typename T>
size_t getTypeSize(const T&)
{
    return sizeof(T);
}

template <>
size_t getTypeSize(const std::string& s)
{
    return s.size();
}

}

//////////////////////////////////////////////////////////////////////////

TEST(BlobTransformation, TransformFile)
{
    TransformationTester<TransformationTypeEnum::File> tt;
    Blob b = BlobTransformation::toBlob(tt.m_value);
    CHECK(b.size() == getTypeSize(tt.m_value) + 2);
    CHECK(BlobTransformation::getBlobType(b) == tt.m_enum);
    CHECK(tt.get(b) == tt.m_value);
    CHECK(tt.getTypeName(b) == "File");
}

TEST(BlobTransformation, TransformFolder)
{
    TransformationTester<TransformationTypeEnum::Folder> tt;
    Blob b = BlobTransformation::toBlob(tt.m_value);
    CHECK(b.size() == getTypeSize(tt.m_value) + 2);
    CHECK(BlobTransformation::getBlobType(b) == tt.m_enum);
    CHECK(tt.get(b).m_folderId == tt.m_value.m_folderId);
    CHECK(tt.getTypeName(b) == "Folder");
}

TEST(BlobTransformation, TransformVersion)
{
    TransformationTester<TransformationTypeEnum::Version> tt;
    Blob b = BlobTransformation::toBlob(tt.m_value);
    CHECK(b.size() == getTypeSize(tt.m_value) + 2);
    CHECK(BlobTransformation::getBlobType(b) == tt.m_enum);
    CHECK(tt.get(b) == tt.m_value);
    CHECK(tt.getTypeName(b) == "Version");
}

TEST(BlobTransformation, TransformDouble)
{
    TransformationTester<TransformationTypeEnum::Double> tt;
    Blob b = BlobTransformation::toBlob(tt.m_value);
    CHECK(b.size() == getTypeSize(tt.m_value) + 2);
    CHECK(BlobTransformation::getBlobType(b) == tt.m_enum);
    CHECK(tt.get(b) == tt.m_value);
    CHECK(tt.getTypeName(b) == "Double");
}

TEST(BlobTransformation, TransformFileDoublePair)
{
    TransformationTester<TransformationTypeEnum::DoublePair> tt;
    Blob b = BlobTransformation::toBlob(tt.m_value);
    CHECK(b.size() == getTypeSize(tt.m_value) + 2);
    CHECK(BlobTransformation::getBlobType(b) == tt.m_enum);
    CHECK(tt.get(b) == tt.m_value);
    CHECK(tt.getTypeName(b) == "DoublePair");
}

TEST(BlobTransformation, TransformInt)
{
    TransformationTester<TransformationTypeEnum::Int> tt;
    Blob b = BlobTransformation::toBlob(tt.m_value);
    CHECK(b.size() == getTypeSize(tt.m_value) + 2);
    CHECK(BlobTransformation::getBlobType(b) == tt.m_enum);
    CHECK(tt.get(b) == tt.m_value);
    CHECK(tt.getTypeName(b) == "Int");
}

TEST(BlobTransformation, TransformString)
{
    TransformationTester<TransformationTypeEnum::String> tt;
    Blob b = BlobTransformation::toBlob(tt.m_value);
    CHECK(b.size() == getTypeSize(tt.m_value) + 2);
    CHECK(BlobTransformation::getBlobType(b) == tt.m_enum);
    CHECK(tt.get(b) == tt.m_value);
    CHECK(tt.getTypeName(b) == "String");
}

//////////////////////////////////////////////////////////////////////////

TEST(BlobTransformation, WrongTypeThrows)
{
    Blob b = BlobTransformation::toBlob("test");
    std::string s = BlobTransformation::toValue<std::string>(b);
    CHECK(s == "test");
    try
    {
        double d = BlobTransformation::toValue<double>(b);
        CHECK(false);
    }
    catch (std::exception&)
    {}
}

TEST(BlobTransformation, EmptyBlobIsUndefined)
{
    Blob b;
    CHECK(BlobTransformation::getBlobType(b) == TransformationTypeEnum::Undefined);
}

TEST(BlobTransformation, EmptyStringIsLegal)
{
    Blob b = BlobTransformation::toBlob("");
    CHECK(BlobTransformation::getBlobType(b) == TransformationTypeEnum::String);
    std::string s = BlobTransformation::toValue<std::string>(b);
    CHECK(s.empty());
}

TEST(BlobTransformation, TooLongStringThrows)
{
    try
    {
        Blob b = BlobTransformation::toBlob(std::string(300, 'X'));
        CHECK(false);
    }
    catch(std::exception&)
    { }
}


