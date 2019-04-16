
#include "stdafx.h"
#include "Test.h"
#include "../CompoundFs/DirectoryObjects.h"

using namespace TxFs;

namespace
{
//     using Types = std::tuple<FileDescriptor, Folder, Version, double, std::pair<double, double>, int32_t,
//     std::string>;

ByteStringOps::Types g_values
    = { { 1, 2, 100 }, Folder { 1 }, { 1, 2, 3 }, { 1.5 }, { std::make_pair(1., 2.) }, { 5 }, { "test" } };

template <DirectoryObjType IType>
struct TransformationTester
{
    using Type = typename std::tuple_element<size_t(IType), ByteStringOps::Types>::type;
    Type m_value = std::get<Type>(g_values);
    DirectoryObjType m_enum = IType;
    Type get(const ByteStringView& blob) { return std::get<Type>(ByteStringOps::toVariant(blob)); }
    std::string getTypeName(const ByteStringView& blob) const { return ByteStringOps::getTypeName(blob); }
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

TEST(ByteStringOps, TransformFile)
{
    TransformationTester<DirectoryObjType::File> tt;
    ByteString b = ByteStringOps::toByteString(tt.m_value);
    CHECK(b.size() == getTypeSize(tt.m_value) + 2);
    CHECK(ByteStringOps::getType(b) == tt.m_enum);
    CHECK(tt.get(b) == tt.m_value);
    CHECK(tt.getTypeName(b) == "File");
}

TEST(ByteStringOps, TransformFolder)
{
    TransformationTester<DirectoryObjType::Folder> tt;
    ByteString b = ByteStringOps::toByteString(tt.m_value);
    CHECK(b.size() == getTypeSize(tt.m_value) + 2);
    CHECK(ByteStringOps::getType(b) == tt.m_enum);
    CHECK(tt.get(b) == tt.m_value);
    CHECK(tt.getTypeName(b) == "Folder");
}

TEST(ByteStringOps, TransformVersion)
{
    TransformationTester<DirectoryObjType::Version> tt;
    ByteString b = ByteStringOps::toByteString(tt.m_value);
    CHECK(b.size() == getTypeSize(tt.m_value) + 2);
    CHECK(ByteStringOps::getType(b) == tt.m_enum);
    CHECK(tt.get(b) == tt.m_value);
    CHECK(tt.getTypeName(b) == "Version");
}

TEST(ByteStringOps, TransformDouble)
{
    TransformationTester<DirectoryObjType::Double> tt;
    ByteString b = ByteStringOps::toByteString(tt.m_value);
    CHECK(b.size() == getTypeSize(tt.m_value) + 2);
    CHECK(ByteStringOps::getType(b) == tt.m_enum);
    CHECK(tt.get(b) == tt.m_value);
    CHECK(tt.getTypeName(b) == "Double");
}

TEST(ByteStringOps, TransformFileDoublePair)
{
    TransformationTester<DirectoryObjType::DoublePair> tt;
    ByteString b = ByteStringOps::toByteString(tt.m_value);
    CHECK(b.size() == getTypeSize(tt.m_value) + 2);
    CHECK(ByteStringOps::getType(b) == tt.m_enum);
    CHECK(tt.get(b) == tt.m_value);
    CHECK(tt.getTypeName(b) == "DoublePair");
}

TEST(ByteStringOps, TransformInt)
{
    TransformationTester<DirectoryObjType::Int> tt;
    ByteString b = ByteStringOps::toByteString(tt.m_value);
    CHECK(b.size() == getTypeSize(tt.m_value) + 2);
    CHECK(ByteStringOps::getType(b) == tt.m_enum);
    CHECK(tt.get(b) == tt.m_value);
    CHECK(tt.getTypeName(b) == "Int");
}

TEST(ByteStringOps, TransformString)
{
    TransformationTester<DirectoryObjType::String> tt;
    ByteString b = ByteStringOps::toByteString(tt.m_value);
    CHECK(b.size() == getTypeSize(tt.m_value) + 2);
    CHECK(ByteStringOps::getType(b) == tt.m_enum);
    CHECK(tt.get(b) == tt.m_value);
    CHECK(tt.getTypeName(b) == "String");
}

//////////////////////////////////////////////////////////////////////////

TEST(ByteStringOps, WrongTypeThrows)
{
    ByteString b = ByteStringOps::toByteString("test");
    std::string s = ByteStringOps::toValue<std::string>(b);
    CHECK(s == "test");
    try
    {
        double d = ByteStringOps::toValue<double>(b);
        CHECK(false);
    }
    catch (std::exception&)
    {}
}

TEST(ByteStringOps, EmptyBlobIsUndefined)
{
    ByteString b;
    CHECK(ByteStringOps::getType(b) == DirectoryObjType::Undefined);
}

TEST(ByteStringOps, EmptyStringIsLegal)
{
    ByteString b = ByteStringOps::toByteString("");
    CHECK(ByteStringOps::getType(b) == DirectoryObjType::String);
    std::string s = ByteStringOps::toValue<std::string>(b);
    CHECK(s.empty());
}

TEST(ByteStringOps, TooLongStringThrows)
{
    try
    {
        ByteString b = ByteStringOps::toByteString(std::string(300, 'X'));
        CHECK(false);
    }
    catch(std::exception&)
    { }
}

