

#pragma once

#include "FileSystem.h"
#include "ReadOnlyFile.h"

namespace TxFs
{

class Composit
{
public:
    template <typename TFile, typename... TArgs>
    static FileSystem open(TArgs&&... args)
    {
        std::unique_ptr<FileInterface> file = std::make_unique<TFile>(std::forward(args)...);
        if (fi->currentSize() == 0)
            return initializeNew(file);

        return initializeExisting(file);
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