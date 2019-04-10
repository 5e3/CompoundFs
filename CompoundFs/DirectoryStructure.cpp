

#include "DirectoryStructure.h"
#include "BlobTransformation.h"

using namespace TxFs;

DirectoryStructure::DirectoryStructure(const std::shared_ptr<CacheManager>& cacheManager, FileDescriptor freeStore,
                                       PageIndex rootIndex, uint32_t maxFolderId)
    : m_cacheManager(cacheManager)
    , m_btree(cacheManager, rootIndex)
    , m_maxFolderId(maxFolderId)
    , m_freeStore(cacheManager, freeStore)
{}

std::optional<Folder> DirectoryStructure::makeSubFolder(std::string_view name, Folder folder)
{
    MutableByteString key;
    key.pushBack(folder);
    key.pushBack(name);
    auto value = BlobTransformation::toBlob(Folder { m_maxFolderId });
    auto res = m_btree.insert(key, value, [](const ByteStringView&, const ByteStringView&) { return false; });

    auto inserted = std::get_if<BTree::Inserted>(&res);
    if (inserted)
        return Folder { m_maxFolderId++ };

    auto unchanged = std::get<BTree::Unchanged>(res);
    if (BlobTransformation::getBlobType(unchanged.m_currentValue.value()) != TransformationTypeEnum::Folder)
        return std::nullopt;

    auto origValue = BlobTransformation::toValue<Folder>(unchanged.m_currentValue.value());
    return origValue;
}

std::optional<Folder> DirectoryStructure::subFolder(std::string_view name, Folder folder) const
{
    MutableByteString key;
    key.pushBack(folder);
    key.pushBack(name);
    auto cursor = m_btree.find(key);
    if (!cursor || BlobTransformation::getBlobType(cursor.value()) != TransformationTypeEnum::Folder)
        return std::nullopt;

    auto subFolder = BlobTransformation::toValue<Folder>(cursor.value());
    return subFolder;
}

bool DirectoryStructure::addAttribute(const BlobTransformation::Variant& attribute, std::string_view name,
                                      Folder folder)
{
    MutableByteString key;
    key.pushBack(folder);
    key.pushBack(name);
    auto value = BlobTransformation::toBlob(attribute);
    auto res = m_btree.insert(key, value, [](const ByteStringView&, const ByteStringView& rhs) {
        auto type = BlobTransformation::getBlobType(rhs);
        return type != TransformationTypeEnum::Folder && type != TransformationTypeEnum::File;
    });
    return !std::holds_alternative<BTree::Unchanged>(res);
}

std::optional<BlobTransformation::Variant> DirectoryStructure::getAttribute(std::string_view name, Folder folder) const
{
    MutableByteString key;
    key.pushBack(folder);
    key.pushBack(name);
    auto cursor = m_btree.find(key);
    if (!cursor)
        return std::nullopt;

    auto type = BlobTransformation::getBlobType(cursor.value());
    if (type == TransformationTypeEnum::Folder || type == TransformationTypeEnum::File)
        return std::nullopt;
    return BlobTransformation::toVariant(cursor.value());
}

size_t DirectoryStructure::remove(Folder folder)
{
    MutableByteString key;
    key.pushBack(folder);
    size_t nof = 0;
    std::vector<ByteString> keysToDelete;
    for (auto cursor = m_btree.begin(key); cursor; cursor = m_btree.next(cursor))
    {
        if (!key.isPrefix(cursor.key()))
            break;
        keysToDelete.push_back(cursor.key());
        nof++;
    }

    for (const auto& k: keysToDelete)
        remove(k);

    return nof;
}

size_t DirectoryStructure::remove(std::string_view name, Folder folder)
{
    MutableByteString key;
    key.pushBack(folder);
    key.pushBack(name);
    return remove(key);
}

size_t DirectoryStructure::remove(ByteStringView key)
{
    auto res = m_btree.remove(key);
    if (!res)
        return 0;

    switch (BlobTransformation::getBlobType(*res))
    {
    case TransformationTypeEnum::Folder:
        return remove(BlobTransformation::toValue<Folder>(*res)) + 1;

    case TransformationTypeEnum::File:
        m_freeStore.deleteFile(BlobTransformation::toValue<FileDescriptor>(*res));
        return 1;

    default:
        return 1;
    }
}
