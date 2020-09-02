

project(CompoundFs)

set (Sources 
		BTree.cpp
		CacheManager.cpp
		CommitHandler.cpp
		Composit.cpp
		DirectoryStructure.cpp
		FileIo.cpp
		FileSharedMutex.cpp
		FileSystem.cpp
		MemoryFile.cpp
		PageAllocator.cpp
		TreeValue.cpp
		Path.cpp
	)
	
set (Headers
		BTree.h
		ByteString.h
		Cache.h
		CacheManager.h
		CommitHandler.h
		Composit.h
		DirectoryStructure.h
		FileDescriptor.h
		FileIo.h
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
		FileInterface.h
		ReadOnlyFile.h
		TableKeyCompare.h
		TreeValue.h
		TypedCacheManager.h
		TxFs.natvis
		Board.txt
	)
	

add_library(${PROJECT_NAME} ${Sources} ${Headers})
