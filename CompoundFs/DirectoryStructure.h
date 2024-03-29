
#pragma once

#include "CacheManager.h"
#include "FreeStore.h"
#include "BTree.h"
#include "TreeValue.h"
#include <memory>
#include <cstdint>

namespace TxFs
{

enum class Folder : uint32_t {Root=0};
struct CommitBlock;

///////////////////////////////////////////////////////////////////////////////

class DirectoryKey final
{
public:
    DirectoryKey(ByteStringView name)
    {
        m_key.push(Folder::Root);
        m_key.push(name); 
    }

    DirectoryKey(Folder folder, ByteStringView name)
    {
        m_key.push(folder);
        m_key.push(name);
    }

    DirectoryKey(Folder folder) noexcept { m_key.push(folder); }

    operator ByteStringView () const noexcept { return m_key; }
    Folder getFolder() const
    {
        Folder folder;
        ByteStringStream::pop(folder, m_key);
        return folder;
    }

    static constexpr size_t maxSize() noexcept { return ByteString::maxSize() - sizeof(Folder); }

private:
    ByteStringStream m_key;
};

///////////////////////////////////////////////////////////////////////////////

class DirectoryStructure final
{
public:
    class Cursor;
    struct Startup
    {
        std::shared_ptr<CacheManager> m_cacheManager;
        PageIndex m_freeStoreIndex;
        PageIndex m_rootIndex;
    };

public:
    DirectoryStructure(const Startup& startup);
    DirectoryStructure(DirectoryStructure&&) noexcept;
    DirectoryStructure& operator=(DirectoryStructure&&) noexcept;

    static Startup initialize(const std::shared_ptr<CacheManager>& cacheManager);
    void init();

    std::optional<Folder> makeSubFolder(const DirectoryKey& dkey);
    std::optional<Folder> subFolder(const DirectoryKey& dkey) const;

    bool addAttribute(const DirectoryKey& dkey, const TreeValue& attribute);
    std::optional<TreeValue> getAttribute(const DirectoryKey& dkey) const;

    bool rename(const DirectoryKey& oldKey, const DirectoryKey& newKey);
    size_t remove(ByteStringView key);
    size_t remove(Folder folder);

    std::optional<FileDescriptor> openFile(const DirectoryKey& dkey) const;
    bool createFile(const DirectoryKey& dkey);
    std::optional<FileDescriptor> appendFile(const DirectoryKey& dkey);
    bool updateFile(const DirectoryKey& dkey, FileDescriptor desc);

    Cursor find(const DirectoryKey& dkey) const;
    Cursor begin(const DirectoryKey& dkey) const;
    Cursor next(Cursor cursor) const;

    void commit();
    void rollback();

    void storeCommitBlock(const CommitBlock&);
    CommitBlock retrieveCommitBlock() const;

private:
    void connectFreeStore();
    void init(const CommitBlock& cb);


private:
    std::shared_ptr<CacheManager> m_cacheManager;
    BTree m_btree;
    uint32_t m_maxFolderId;
    FreeStore m_freeStore;
    PageIndex m_rootIndex;
};

//////////////////////////////////////////////////////////////////////////

class DirectoryStructure::Cursor final
{
    friend class DirectoryStructure;

public:
    constexpr Cursor() noexcept = default;
    Cursor(const BTree::Cursor& cursor) noexcept
        : m_cursor(cursor)
    {}

    constexpr bool operator==(const Cursor& rhs) const noexcept { return m_cursor == rhs.m_cursor; }
    constexpr bool operator!=(const Cursor& rhs) const noexcept { return !(m_cursor == rhs.m_cursor); }

    std::pair<Folder,std::string_view> key() const;
    TreeValue value() const { return TreeValue::fromStream(m_cursor.value());}
    constexpr explicit operator bool() const noexcept { return m_cursor.operator bool(); }

private:
    BTree::Cursor m_cursor;
};

//////////////////////////////////////////////////////////////////////////

inline DirectoryStructure::Cursor DirectoryStructure::find(const DirectoryKey& dkey) const
{
    return m_btree.find(dkey);
}





}
