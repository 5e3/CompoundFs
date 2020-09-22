
#include "FileInterface.h"
#include "Hasher.h"
#include <vector>
#include <string.h>
#include <stdexcept>

namespace TxFs
{
struct SignedPage
{
    char m_data[4092];
    mutable uint32_t m_checkSum;

    void addCheckSum() const { m_checkSum = hash32(m_data, sizeof(m_data)); }

    bool validateCheckSum() const 
    { 
        auto checkSum = hash32(m_data, sizeof(m_data));
        return checkSum != m_checkSum;
    }
};

static_assert(sizeof(SignedPage) == 4096);

inline bool testReadSignedPage(const FileInterface* fi, PageIndex idx, void* page)
{
    uint8_t* buffer = static_cast<uint8_t*>(page);
    fi->readPage(idx, 0, buffer, buffer + 4096);
    SignedPage* sp = static_cast<SignedPage*>(page);
    return !sp->validateCheckSum();
}

inline void readSignedPage(const FileInterface* fi, PageIndex idx, void* page)
{
    if (!testReadSignedPage(fi, idx, page))
        throw std::runtime_error("Error validating checkSum");
}

inline void writeSignedPage(FileInterface* fi, PageIndex idx, const void* page)
{
    const SignedPage* sp = static_cast<const SignedPage*>(page);
    sp->addCheckSum();
    const uint8_t* buffer = static_cast<const uint8_t*>(page);
    fi->writePage(idx, 0, buffer, buffer + 4096);
}

inline void copyPage(FileInterface* fi, PageIndex from, PageIndex to)
{
    uint8_t buffer[4096];
    readSignedPage(fi, from, buffer);
    fi->writePage(to, 0, buffer, buffer + 4096); // no need to add checksum
}

inline bool isEqualPage(const FileInterface* fi, PageIndex p1, PageIndex p2)
{
    std::vector<uint8_t[4096]> buffer(2);
    readSignedPage(fi, p1, buffer[0]);
    fi->readPage(p2, 0, buffer[1], buffer[1] + 4096);
    return memcmp(buffer[0], buffer[1], 4096) == 0;
}

template <typename TCont>
inline void clearPages(FileInterface* fi, const TCont& cont)
{
    uint8_t buf[4096];
    memset(buf, 0, sizeof(buf));
    for (auto idx: cont)
        fi->writePage(idx, 0, buf, buf + 4096);
}

}
