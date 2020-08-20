

#include <stdint.h>
#include <algorithm>
#include <string_view>
#include <type_traits>
#include <stdexcept>
#include <vector>

namespace TxFs
{
template <typename T>
using EnableWithString = std::enable_if_t<std::is_convertible_v<T, std::string_view>>;

class ByteStringView
{
public:
    constexpr ByteStringView() noexcept
        : m_data(nullptr)
        , m_length(0) {};
    constexpr ByteStringView(const uint8_t* data, uint8_t length) noexcept
        : m_data(data)
        , m_length(length) {};

    template <typename TStr, typename = EnableWithString<TStr>>
    ByteStringView(TStr&& str)
    {
        std::string_view sv { str };
        if (sv.size() > std::numeric_limits<uint8_t>::max())
            throw std::runtime_error("ByteStringView: size too big");

        m_data = reinterpret_cast<const uint8_t*>(sv.data());
        m_length = static_cast<uint8_t>(sv.size());
    }

    template <typename TStr, typename = EnableWithString<TStr>>
    ByteStringView& operator=(TStr&& str)
    {
        ByteStringView bsv = str;
        *this = bsv;
        return *this;
    }

    friend bool operator==(ByteStringView lhs, ByteStringView rhs);
    friend bool operator!=(ByteStringView lhs, ByteStringView rhs);
    friend bool operator<(ByteStringView lhs, ByteStringView rhs);

    constexpr size_t size() const noexcept { return m_length; }
    constexpr const uint8_t* data() const noexcept { return m_data; }
    static constexpr ByteStringView fromStream(const uint8_t* stream) noexcept { return ByteStringView(stream + 1, *stream); }

private:
    const uint8_t* m_data;
    uint8_t m_length;
};

const uint8_t* toStream(ByteStringView bsv, uint8_t* dest)
{
    *dest = bsv.size();
    return std::copy(bsv.data(), bsv.data() + bsv.size(), ++dest);
}

inline bool operator==(ByteStringView lhs, ByteStringView rhs)
{
    return std::equal(lhs.m_data, lhs.m_data + lhs.m_length, rhs.m_data, rhs.m_data + rhs.m_length);
}

inline bool operator!=(ByteStringView lhs, ByteStringView rhs)
{
    return !(lhs == rhs);
}

inline bool operator<(ByteStringView lhs, ByteStringView rhs)
{
    return std::lexicographical_compare(lhs.m_data, lhs.m_data + lhs.m_length, rhs.m_data, rhs.m_data + rhs.m_length);
}

class ByteString
{
public:
    ByteString() noexcept = default;

    ByteString(ByteStringView bsv)
        : m_buffer(bsv.data(), bsv.data() + bsv.size())
    {}
    ByteString& operator=(ByteStringView bsv)
    {
        ByteString bs(bsv);
        std::swap(bs.m_buffer, m_buffer);
        return *this;
    }

    template <typename TStr, typename = EnableWithString<TStr>>
    ByteString(TStr&& str)
        : ByteString(ByteStringView(str))
    {}

    template <typename TStr, typename = EnableWithString<TStr>>
    ByteString& operator=(TStr&& str)
    {
        *this = ByteString(str);
        return *this;
    }

    operator ByteStringView() const { return ByteStringView(&m_buffer[0], m_buffer.size()); }
    size_t size() const { return m_buffer.size(); }

private:
    std::vector<uint8_t> m_buffer;
};

}

#include <gtest/gtest.h>
#include <string>

void checkEqual(TxFs::ByteStringView bsv, const std::string& ref)
{
    if (bsv != ref)
        throw std::runtime_error("test failed");
}


using namespace TxFs;

TEST(ByteString, creation)
{
    ByteString b = "test";
    ASSERT_EQ(b, "test");

    b = "test2";
    ASSERT_EQ(b, "test2");

    ByteString b2 = std::move(b);
    ASSERT_EQ(b2, "test2");
    ASSERT_EQ(b.size(), 0);
}

TEST(ByteString, tooLongStringThrows)
{
    std::string s(256, '-');
    ASSERT_THROW(ByteString bs(s), std::exception);
    ASSERT_THROW(ByteStringView bsv(s), std::exception);
}

TEST(ByteStringView, fromStreamConvertsbackToStream)
{
    uint8_t stream[256];
    std::string s(255, '-');
    auto end = toStream(s, stream);
    ASSERT_EQ(end, stream + 256);

    ByteString bs = ByteStringView::fromStream(stream);
    ASSERT_EQ(bs, s);
    checkEqual(std::string(s), s);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
