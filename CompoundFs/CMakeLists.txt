

project(CompoundFs)

set (Sources 
		BTree.cpp
		CacheManager.cpp
		CommitBlock.cpp
		CommitHandler.cpp
		Composite.cpp
		DirectoryStructure.cpp
		FileSystem.cpp
		Hasher.cpp
		MemoryFile.cpp
		PageAllocator.cpp
		PosixFile.cpp
		TempFile.cpp
		TreeValue.cpp
		Path.cpp
		RollbackHandler.cpp
		WrappedFile.cpp
	)

if(WIN32)
set (PlatformSources
		WindowsFile.cpp
		FileLockWindows.cpp
)
else()
set (PlatformSources
		FileLockLinux.cpp
)
endif()

	
set (Headers
		BTree.h
		ByteString.h
		Cache.h
		CacheManager.h
		CommitBlock.h
		CommitHandler.h
		Composite.h
		DirectoryStructure.h
		FileDescriptor.h
		FileInterface.h
		FileIo.h
		FileLockPosition.h
		FileReader.h
		FileSystem.h
		FileTable.h
		FileWriter.h
		FreeStore.h
		Hasher.h
		InnerNode.h
		Interval.h
		IntervalSequence.h
		Leaf.h
		Lock.h
		LockProtocol.h
		LogPage.h
		MemoryFile.h
		Node.h
		FileLockLinux.h
		PageAllocator.h
		PageDef.h
		PageMetaData.h
		Path.h
		PosixFile.h
		ReadOnlyFile.h
		RollbackHandler.h
		SmallBufferStack.h
		TableKeyCompare.h
		TempFile.h
		TreeValue.h
		TypedCacheManager.h
		WrappedFile.h
		TxFs.natvis
		Board.txt
	)
	
if(WIN32)
set (PlatformHeaders
		WindowsFile.h
		FileLockWindows.h
)
else()
set (PlatformHeaders
		FileLockLinux.h
)
endif()


add_library(${PROJECT_NAME} ${Sources} ${PlatformSources} ${Headers} ${PlatformHeaders})

target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_compile_options(${PROJECT_NAME} PRIVATE
     $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
          -Wall>
     $<$<CXX_COMPILER_ID:MSVC>:
          /W4>)

target_link_libraries(${PROJECT_NAME} PRIVATE xxhash)
