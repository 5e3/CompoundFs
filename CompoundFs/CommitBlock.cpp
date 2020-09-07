

#include "CommitBlock.h"
#include "ByteString.h"

using namespace TxFs;

std::string CommitBlock::toString() const
{
    ByteStringStream bss;
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
    bsv = ByteStringStream::pop(cb.m_freeStoreDescriptor.m_fileSize, bsv);
    bsv = ByteStringStream::pop(cb.m_freeStoreDescriptor.m_first, bsv);
    bsv = ByteStringStream::pop(cb.m_freeStoreDescriptor.m_last, bsv);
    bsv = ByteStringStream::pop(cb.m_compositSize, bsv);
    bsv = ByteStringStream::pop(cb.m_maxFolderId, bsv);
    return cb;
}
