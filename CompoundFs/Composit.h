

#pragma once

#include "FileSystem.h"
#include "ReadOnlyFile.h"
#include <utility>
#include <memory>

namespace TxFs
{

class Composit
{
public:
    template <typename TFile, typename... TArgs>
    static FileSystem open(TArgs&&... args)
    {
        std::unique_ptr<FileInterface> file = std::make_unique<TFile>(std::forward<TArgs>(args)...);
        if (file->currentSize() == 0)
            return initializeNew(std::move(file));

        return initializeExisting(std::move(file));
    }
    
    template <typename TFile, typename... TArgs>
    static FileSystem openReadOnly(TArgs&&... args)
    {
        std::unique_ptr<FileInterface> file = std::make_unique<ReadOnlyFile<TFile>>(std::forward(args)...);
        return initializeReadOnly(file);
    }

private:
    static FileSystem initializeNew(std::unique_ptr<FileInterface> file);
    static FileSystem initializeExisting(std::unique_ptr<FileInterface> file);
    static FileSystem initializeReadOnly(std::unique_ptr<FileInterface> file);
};

}