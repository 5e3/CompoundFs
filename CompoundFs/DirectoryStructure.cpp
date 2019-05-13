

#include "DirectoryStructure.h"
#include "DirectoryObjects.h"

using namespace TxFs;

DirectoryStructure::DirectoryStructure(const std::shared_ptr<CacheManager>& cacheManager, FileDescriptor freeStore,
                                       PageIndex rootIndex, uint32_t maxFolderId)
    : m_cacheManager(cacheManager)
    , m_btree(cacheManager, rootIndex)
    , m_maxFolderId(maxFolderId)
    , m_freeStore(cacheManager, freeStore)
{}

std::optional<Folder> DirectoryStructure::makeSubFolder(const DirectoryKey& dkey)
{
    auto value = ByteStringOps::toByteString(Folder { m_maxFolderId });
    auto res = m_btree.insert(dkey.asByteStringView(), value, [](const ByteStringView&) { return false; });

    auto inserted = std::get_if<BTree::Inserted>(&res);
    if (inserted)
        return Folder { m_maxFolderId++ };

    auto unchanged = std::get<BTree::Unchanged>(res);
    if (ByteStringOps::getType(unchanged.m_currentValue.value()) != DirectoryObjType::Folder)
        return std::nullopt;

    auto origValue = ByteStringOps::toValue<Folder>(unchanged.m_currentValue.value());
    return origValue;
}

std::optional<Folder> DirectoryStructure::subFolder(const DirectoryKey& dkey) const
{
    auto cursor = m_btree.find(dkey.asByteStringView());
    if (!cursor || ByteStringOps::getType(cursor.value()) != DirectoryObjType::Folder)
        return std::nullopt;

    auto subFolder = ByteStringOps::toValue<Folder>(cursor.value());
    return subFolder;
}

bool DirectoryStructure::addAttribute(const DirectoryKey& dkey, const ByteStringOps::Variant& attribute)
{
    auto value = ByteStringOps::toByteString(attribute);
    auto res = m_btree.insert(dkey.asByteStringView(), value, [](const ByteStringView& rhs) {
        auto type = ByteStringOps::getType(rhs);
        return type != DirectoryObjType::Folder && type != DirectoryObjType::File;
    });
    return !std::holds_alternative<BTree::Unchanged>(res);
}

std::optional<ByteStringOps::Variant> DirectoryStructure::getAttribute(const DirectoryKey& dkey) const
{
    auto cursor = m_btree.find(dkey.asByteStringView());
    if (!cursor)
        return std::nullopt;

    auto type = ByteStringOps::getType(cursor.value());
    if (type == DirectoryObjType::Folder || type == DirectoryObjType::File)
        return std::nullopt;
    return ByteStringOps::toVariant(cursor.value());
}

size_t DirectoryStructure::remove(Folder folder)
{
    MutableByteString key;
    key.pushBack(folder);
    std::vector<ByteString> keysToDelete;
    for (auto cursor = m_btree.begin(key); cursor; cursor = m_btree.next(cursor))
    {
        if (!key.isPrefix(cursor.key()))
            break;
        keysToDelete.push_back(cursor.key());
    }

    for (const auto& k: keysToDelete)
        remove(k);

    return keysToDelete.size();
}

size_t DirectoryStructure::remove(const DirectoryKey& dkey)
{
    return remove(dkey.asByteStringView());
}

size_t DirectoryStructure::remove(ByteStringView key)
{
    auto res = m_btree.remove(key);
    if (!res)
        return 0;

    switch (ByteStringOps::getType(*res))
    {
    case DirectoryObjType::Folder:
        return remove(ByteStringOps::toValue<Folder>(*res)) + 1;

    case DirectoryObjType::File:
        m_freeStore.deleteFile(ByteStringOps::toValue<FileDescriptor>(*res));
        return 1;

    default:
        return 1;
    }
}

std::optional<FileDescriptor> DirectoryStructure::openFile(const DirectoryKey& dkey) const
{
    auto cursor = m_btree.find(dkey.asByteStringView());
    if (!cursor || ByteStringOps::getType(cursor.value()) != DirectoryObjType::File)
        return std::nullopt;

    return ByteStringOps::toValue<FileDescriptor>(cursor.value());
}

bool DirectoryStructure::createFile(const DirectoryKey& dkey)
{
    auto value = ByteStringOps::toByteString(FileDescriptor());
    auto res = m_btree.insert(dkey.asByteStringView(), value, [](const ByteStringView& rhs) {
        return ByteStringOps::getType(rhs) == DirectoryObjType::File;
    });

    if (std::holds_alternative<BTree::Unchanged>(res))
        return false;

    auto replaced = std::get_if<BTree::Replaced>(&res);
    if (replaced)
        m_freeStore.deleteFile(ByteStringOps::toValue<FileDescriptor>(replaced->m_beforeValue));

    return true;
}

std::optional<FileDescriptor> DirectoryStructure::appendFile(const DirectoryKey& dkey)
{
    auto value = ByteStringOps::toByteString(FileDescriptor());
    auto res = m_btree.insert(dkey.asByteStringView(), value, [](const ByteStringView&) { return false; });

    if (std::holds_alternative<BTree::Inserted>(res))
        return FileDescriptor();

    auto cursor = std::get<BTree::Unchanged>(res).m_currentValue;
    if (ByteStringOps::getType(cursor.value()) != DirectoryObjType::File)
        return std::nullopt;
    return ByteStringOps::toValue<FileDescriptor>(cursor.value());
}

bool DirectoryStructure::updateFile(const DirectoryKey& dkey, FileDescriptor desc)
{
    auto value = ByteStringOps::toByteString(desc);
    auto res = m_btree.insert(dkey.asByteStringView(), value, [](const ByteStringView& rhs) {
        return ByteStringOps::getType(rhs) == DirectoryObjType::File;
    });

    if (std::holds_alternative<BTree::Unchanged>(res))
        return false;

    auto inserted = std::get_if<BTree::Inserted>(&res);
    if (!inserted)
        return true;

    remove(dkey);
    return false;
}

void DirectoryStructure::commit()
{
    const auto& freePages = m_btree.getFreePages();
    for (auto page : freePages)
        m_freeStore.deallocate(page);

    auto redirectedPages = m_cacheManager->getRedirectedPages();
    for (auto page : redirectedPages)
        m_freeStore.deallocate(page);

    m_cacheManager->commit();
    auto fileDescriptor = m_freeStore.close();
}

//////////////////////////////////////////////////////////////////////////

std::pair<Folder, std::string_view> DirectoryStructure::Cursor::key() const
{
    auto key = m_cursor.key();
    Folder folder;
    auto name = std::copy(key.begin() + 1, key.begin() + 1 + sizeof(Folder), (uint8_t*)&folder);
    std::string_view nameView((const char*)key.begin() + 1 + sizeof(Folder), key.size() - sizeof(Folder) -1);
    return std::pair(folder, nameView);
}

