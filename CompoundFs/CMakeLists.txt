

project(CompoundFs)

set (Sources 
		BTree.cpp
		CacheManager.cpp
		CommitBlock.cpp
		CommitHandler.cpp
		Composit.cpp
		DirectoryStructure.cpp
		FileSystem.cpp
		MemoryFile.cpp
		PageAllocator.cpp
		TreeValue.cpp
		Path.cpp
		WrappedFile.cpp
	)

if(WIN32)
set (PlatformSources
		File.cpp
		FileSharedMutex.cpp
		PosixFile.cpp
)
elseif(LINUX)
set (PlatformSources
)
endif()

	
set (Headers
		BTree.h
		ByteString.h
		Cache.h
		CacheManager.h
		CommitBlock.h
		CommitHandler.h
		Composit.h
		DirectoryStructure.h
		FileDescriptor.h
		FileInterface.h
		FileReader.h
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
		ReadOnlyFile.h
		TableKeyCompare.h
		TreeValue.h
		TypedCacheManager.h
		WrappedFile.h
		TxFs.natvis
		Board.txt
	)
	
if(WIN32)
set (PlatformHeaders
		File.h
		FileSharedMutex.h
		PosixFile.h
)
elseif(LINUX)
set (PlatformHeaders
)
endif()


add_library(${PROJECT_NAME} ${Sources} ${PlatformSources} ${Headers} ${PlatformHeaders})

target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_compile_options(${PROJECT_NAME} PRIVATE
     $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
          -Wall>
     $<$<CXX_COMPILER_ID:MSVC>:
          /W4>)
