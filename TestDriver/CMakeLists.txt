

project(TestDriver)

set (Sources 
		stdafx.cpp
		TestBTree.cpp
		TestCacheManager.cpp
		TestCommitHandler.cpp
		TestDirectoryObjects.cpp
		TestDirectoryStructure.cpp
		TestDriver.cpp
		TestFile.cpp
		TestFileSharedMutex.cpp
		TestFileSystem.cpp
		TestFileTable.cpp
		TestFreeStore.cpp
		TestIntervalSequence.cpp
		TestLock.cpp
		TestLockProtocol.cpp
		TestLogPage.cpp
		TestMemoryFile.cpp
		TestNode.cpp
		TestPageAllocator.cpp
		TestPageMetaData.cpp
		TestPath.cpp
		TestReadOnlyFile.cpp
		TestTypedCacheManager.cpp
	)
	
set (Headers
		MinimalTreeBuilder.h
		stdafx.h
		targetver.h
		Test.h
	)
	

source_group("" FILES ${Sources} ${Headers})

add_executable(${PROJECT_NAME} ${Sources} ${Headers})
target_link_libraries(${PROJECT_NAME} PUBLIC CompoundFs)