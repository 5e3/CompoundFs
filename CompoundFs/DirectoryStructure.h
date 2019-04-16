
#pragma once

#include "CacheManager.h"
#include "FreeStore.h"
#include "BTree.h"
#include "DirectoryObjects.h"
#include <memory>
#include <cstdint>

namespace TxFs
{

enum class Folder : uint32_t;

///////////////////////////////////////////////////////////////////////////////

class DirectoryKey
{
public:
    static constexpr Folder Root { 0 };

public:
    DirectoryKey(std::string_view name)
    {
        m_key.pushBack(Root);
        m_key.pushBack(name);
    }

    DirectoryKey(Folder folder, std::string_view name)
    {
        m_key.pushBack(folder);
        m_key.pushBack(name);
    }

    DirectoryKey(Folder folder) { m_key.pushBack(folder); }

    ByteStringView getByteStringView() const { return m_key; }

private:
    MutableByteString m_key;
};

///////////////////////////////////////////////////////////////////////////////

class DirectoryStructure
{

public:
    DirectoryStructure(const std::shared_ptr<CacheManager>& cacheManager, FileDescriptor freeStore,
                       PageIndex rootIndex = PageIdx::INVALID, uint32_t maxFolderId = 1);

    std::optional<Folder> makeSubFolder(const DirectoryKey& dkey);
    std::optional<Folder> subFolder(const DirectoryKey& dkey) const;

    bool addAttribute(const DirectoryKey& dkey, const ByteStringOps::Variant& attribute);
    std::optional<ByteStringOps::Variant> getAttribute(const DirectoryKey& dkey) const;

    size_t remove(const DirectoryKey& dkey);
    size_t remove(ByteStringView key);
    size_t remove(Folder folder);

    std::optional<FileDescriptor> openFile(const DirectoryKey& dkey) const;
    std::optional<FileDescriptor> createFile(const DirectoryKey& dkey);
    std::optional<FileDescriptor> appendFile(const DirectoryKey& dkey);
    bool updateFile(const DirectoryKey& dkey);


private:
    std::shared_ptr<CacheManager> m_cacheManager;
    BTree m_btree;
    uint32_t m_maxFolderId;
    FreeStore m_freeStore;
};
}
