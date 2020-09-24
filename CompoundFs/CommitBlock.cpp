

#include "CommitBlock.h"
#include "ByteString.h"

using namespace TxFs;

std::string CommitBlock::toString() const
{
    ByteStringStream bss;
    uint8_t version = 0; // make it versionable
    bss.push(version);
    bss.push(m_freeStoreDescriptor.m_fileSize);
    bss.push(m_freeStoreDescriptor.m_first);
    bss.push(m_freeStoreDescriptor.m_last);
    bss.push(m_compositSize);
    bss.push(m_maxFolderId);
    ByteStringView bsv = bss;
    return std::string(bsv.data(), bsv.end());
}

CommitBlock CommitBlock::fromString(std::string_view sv)
{
    ByteStringView bsv = sv;
    CommitBlock cb;
    uint8_t version = 0;
    bsv = ByteStringStream::pop(version, bsv);
    bsv = ByteStringStream::pop(cb.m_freeStoreDescriptor.m_fileSize, bsv);
    bsv = ByteStringStream::pop(cb.m_freeStoreDescriptor.m_first, bsv);
    bsv = ByteStringStream::pop(cb.m_freeStoreDescriptor.m_last, bsv);
    bsv = ByteStringStream::pop(cb.m_compositSize, bsv);
    bsv = ByteStringStream::pop(cb.m_maxFolderId, bsv);
    return cb;
}
