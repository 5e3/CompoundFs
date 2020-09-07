

#include "DirectoryStructure.h"
#include "ByteString.h"
#include "TreeValue.h"
#include "CommitBlock.h"
#include "CommitHandler.h"
#include <assert.h>

using namespace TxFs;

namespace
{
    struct ValueStream
    {
        ValueStream(const TreeValue& value) { value.toStream(m_byteStringStream); }

        operator ByteStringView() const { return static_cast<ByteStringView>(m_byteStringStream); }

        ByteStringStream m_byteStringStream;
    };

    // ------------------------------------------------------------------------

    constexpr Folder            SystemFolder { 1 };
    constexpr std::string_view CompositeSizeAttributeName { "CompositeSize" };
    constexpr std::string_view FreeStoreFirstAttributeName { "FreeStore.first" };
    constexpr std::string_view FreeStoreLastAttributeName { "FreeStore.last" };
    constexpr std::string_view FreeStoreSizeAttributeName { "FreeStore.size" };
    constexpr std::string_view CommitBlockAttributeName { "CommitBlock" };
    }

DirectoryStructure::DirectoryStructure(const std::shared_ptr<CacheManager>& cacheManager, PageIndex freeStoreIndex,
                                       PageIndex rootIndex, uint32_t maxFolderId)
    : m_cacheManager(cacheManager)
    , m_btree(cacheManager, rootIndex)
    , m_maxFolderId(maxFolderId)
    , m_freeStore(cacheManager, FileDescriptor(freeStoreIndex))
{
    assert(static_cast<Folder>(m_maxFolderId) > SystemFolder);
    connectFreeStore();
}

DirectoryStructure::DirectoryStructure(DirectoryStructure&& ds)
    : m_cacheManager(std::move(ds.m_cacheManager))
    , m_btree(std::move(ds.m_btree))
    , m_maxFolderId(std::move(ds.m_maxFolderId))
    , m_freeStore(std::move(ds.m_freeStore))
{
    connectFreeStore();
}

void DirectoryStructure::connectFreeStore()
{
    m_cacheManager->setPageIntervalAllocator([fs = &m_freeStore](size_t maxPages) { return fs->allocate(maxPages); });
}

std::optional<Folder> DirectoryStructure::makeSubFolder(const DirectoryKey& dkey)
{
    ValueStream value(Folder { m_maxFolderId }); 
    auto res = m_btree.insert(dkey, value, [](ByteStringView) { return false; });

    auto inserted = std::get_if<BTree::Inserted>(&res);
    if (inserted)
        return Folder { m_maxFolderId++ };

    auto unchanged = std::get<BTree::Unchanged>(res);
    auto origValue = TreeValue::fromStream(unchanged.m_currentValue.value());
    if (origValue.getType() != TreeValue::Type::Folder)
        return std::nullopt;

    return origValue.toValue<Folder>();
}

std::optional<Folder> DirectoryStructure::subFolder(const DirectoryKey& dkey) const
{
    auto cursor = m_btree.find(dkey);
    if (!cursor)
        return std::nullopt;

    auto treeValue = TreeValue::fromStream(cursor.value());
    if (treeValue.getType() != TreeValue::Type::Folder)
        return std::nullopt;

    return treeValue.toValue<Folder>();
}

bool DirectoryStructure::addAttribute(const DirectoryKey& dkey, const TreeValue& attribute)
{
    ValueStream value(attribute);
    auto res = m_btree.insert(dkey, value, [](ByteStringView bsv) {
        auto type = TreeValue::fromStream(bsv).getType();
        return type != TreeValue::Type::Folder && type != TreeValue::Type::File;
    });
    return !std::holds_alternative<BTree::Unchanged>(res);
}

std::optional<TreeValue> DirectoryStructure::getAttribute(const DirectoryKey& dkey) const
{
    auto cursor = m_btree.find(dkey);
    if (!cursor)
        return std::nullopt;

    auto attribute = TreeValue::fromStream(cursor.value());
    if (attribute.getType() == TreeValue::Type::Folder || attribute.getType() == TreeValue::Type::File)
        return std::nullopt;
    return attribute;
}

size_t DirectoryStructure::remove(Folder folder)
{
    ByteStringStream key; // TODO: use DirectoryKey here
    key.push(folder);
    std::vector<ByteString> keysToDelete;
    for (auto cursor = m_btree.begin(key); cursor; cursor = m_btree.next(cursor))
    {
        if (!key.isPrefix(cursor.key()))
            break;
        keysToDelete.emplace_back(cursor.key());
    }

    for (const auto& k: keysToDelete)
        remove(k);

    return keysToDelete.size();
}

size_t DirectoryStructure::remove(ByteStringView key)
{
    auto res = m_btree.remove(key);
    if (!res)
        return 0;

    auto deletedValue = TreeValue::fromStream(*res);
    switch (deletedValue.getType())
    {
    case TreeValue::Type::Folder:
        return remove(deletedValue.toValue<Folder>()) + 1;

    case TreeValue::Type::File:
        m_freeStore.deleteFile(deletedValue.toValue<FileDescriptor>());
        return 1;

    default:
        return 1;
    }
}

std::optional<FileDescriptor> DirectoryStructure::openFile(const DirectoryKey& dkey) const
{
    auto cursor = m_btree.find(dkey);
    if (!cursor)
        return std::nullopt;

    auto treeValue = TreeValue::fromStream(cursor.value());
    if (treeValue.getType() != TreeValue::Type::File)
        return std::nullopt;

    return treeValue.toValue<FileDescriptor>();
}

bool DirectoryStructure::createFile(const DirectoryKey& dkey)
{
    ValueStream value(FileDescriptor{});
    auto res = m_btree.insert(dkey, value, [](ByteStringView bsv) {
        return TreeValue::fromStream(bsv).getType() == TreeValue::Type::File;
    });

    if (std::holds_alternative<BTree::Unchanged>(res))
        return false;

    auto replaced = std::get_if<BTree::Replaced>(&res);
    if (!replaced)
        return true;

    auto beforeFile = TreeValue::fromStream(replaced->m_beforeValue);
    m_freeStore.deleteFile(beforeFile.toValue<FileDescriptor>());
    return true;
}

std::optional<FileDescriptor> DirectoryStructure::appendFile(const DirectoryKey& dkey)
{
    ValueStream value(FileDescriptor {});
    auto res = m_btree.insert(dkey, value, [](ByteStringView bsv) { return false; });

    if (std::holds_alternative<BTree::Inserted>(res))
        return FileDescriptor{};

    auto cursor = std::get<BTree::Unchanged>(res).m_currentValue;
    auto currentValue = TreeValue::fromStream(cursor.value());
    if (currentValue.getType() != TreeValue::Type::File)
        return std::nullopt;

    return currentValue.toValue<FileDescriptor>();
}

bool DirectoryStructure::updateFile(const DirectoryKey& dkey, FileDescriptor desc)
{
    ValueStream value = desc;
    auto res = m_btree.insert(dkey, value, [](ByteStringView bsv) {
        return TreeValue::fromStream(bsv).getType() == TreeValue::Type::File;
    });

    if (std::holds_alternative<BTree::Unchanged>(res))
        return false;

    auto replaced = std::get_if<BTree::Replaced>(&res);
    if (replaced)
        return true;

    remove(dkey);
    return false;
}

void DirectoryStructure::commit()
{
    const auto& freePages = m_btree.getFreePages();
    for (auto page : freePages)
        m_freeStore.deallocate(page);

    auto commitHandler = m_cacheManager->getCommitHandler();

    auto divertedPageIds = commitHandler.getDivertedPageIds();
    for (auto page: divertedPageIds)
        m_freeStore.deallocate(page);

    CommitBlock cb;
    cb.m_freeStoreDescriptor = m_freeStore.close();
    cb.m_compositSize = commitHandler.getCompositeSize();
    cb.m_maxFolderId = m_maxFolderId;
    StoreCommitBlock(cb);
    commitHandler.commit();
    assert(commitHandler.empty());

    m_freeStore = FreeStore(m_cacheManager, cb.m_freeStoreDescriptor);
    connectFreeStore();
    assert(cb.m_compositSize == commitHandler.getCompositeSize());
}

void DirectoryStructure::rollback()
{
    auto commitBlock = retrieveCommitBlock();
    m_maxFolderId = commitBlock.m_maxFolderId;
    m_cacheManager->rollback(commitBlock.m_compositSize);

    m_freeStore = FreeStore(m_cacheManager, commitBlock.m_freeStoreDescriptor);
    connectFreeStore();
}


void DirectoryStructure::StoreCommitBlock(const CommitBlock& cb)
{
    auto str = cb.toString();
    addAttribute(DirectoryKey(SystemFolder, CompositeSizeAttributeName), str);
}

CommitBlock DirectoryStructure::retrieveCommitBlock()
{
    auto str = getAttribute(DirectoryKey(SystemFolder, CompositeSizeAttributeName))->toValue<std::string>();
    return CommitBlock::fromString(str);
}

//////////////////////////////////////////////////////////////////////////

std::pair<Folder, std::string_view> DirectoryStructure::Cursor::key() const
{
    auto key = m_cursor.key();
    Folder folder;
    auto name = ByteStringStream::pop(folder, key); //TODO: fix ByteStringStream to consume std::string_views
    std::string_view nameView ( reinterpret_cast<const char*> (name.data()), name.size() );
    return std::pair(folder, nameView);
}

