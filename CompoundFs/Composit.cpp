
#include "Composit.h"

using namespace TxFs;




FileSystem Composit::initializeNew(std::unique_ptr<FileInterface> file)
{
    auto cacheManager = std::make_shared<CacheManager>(std::move(file));
    TypedCacheManager tcm(cacheManager);
    auto freeStorePage = tcm.newPage<FileTable>();
    auto fileSystem = FileSystem(cacheManager, freeStorePage.m_index);
    fileSystem.commit();
    return fileSystem;
}

FileSystem Composit::initializeExisting(std::unique_ptr<FileInterface> fileInterface)
{
    auto cacheManager = std::make_shared<CacheManager>(std::move(fileInterface));
    auto logs = cacheManager->readLogs();
    auto* file = cacheManager->getFileInterface();
    for (auto [orig, cpy]: logs)
        TxFs::copyPage(file, cpy, orig);
    file->flushFile();

    auto fileSystem = FileSystem(cacheManager, 0, 1);
    fileSystem.rollback();
    return fileSystem;
}

FileSystem Composit::initializeReadOnly(std::unique_ptr<FileInterface> file)
{
    throw std::runtime_error("not implemented");
}

