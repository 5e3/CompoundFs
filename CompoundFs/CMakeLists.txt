

project(CompoundFs)

set (Sources 
		BTree.cpp
		CacheManager.cpp
		CommitHandler.cpp
		Composit.cpp
		DirectoryObjects.cpp
		DirectoryStructure.cpp
		FileSharedMutex.cpp
		FileSystem.cpp
		MemoryFile.cpp
		PageAllocator.cpp
		Path.cpp
	)
	
set (Headers
		BTree.h
		ByteString.h
		Cache.h
		CacheManager.h
		CommitHandler.h
		Composit.h
		DirectoryObjects.h
		DirectoryStructure.h
		FileDescriptor.h
		FileReader.h
		FileSharedMutex.h
		FileSystem.h
		FileTable.h
		FileWriter.h
		FreeStore.h
		InnerNode.h
		Interval.h
		IntervalSequence.h
		Leaf.h
		Lock.h
		LockProtocol.h
		LogPage.h
		MemoryFile.h
		Node.h
		PageAllocator.h
		PageDef.h
		PageMetaData.h
		Path.h
		RawFileInterface.h
		ReadOnlyFile.h
		TypedCacheManager.h
		TxFs.natvis
	)
	

add_library(${PROJECT_NAME} ${Sources} ${Headers})