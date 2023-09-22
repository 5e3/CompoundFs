

project(TestDriver)

set (Sources
		main.cpp
		TestBTree.cpp
		TestCacheManager.cpp
		TestCommitHandler.cpp
		TestByteString.cpp
		TestComposite.cpp
		TestDirectoryStructure.cpp
		TestFileInterface.cpp
		TestFileReaderWriter.cpp
		TestFileSystem.cpp
		TestFileSystemHelper.cpp
		TestFileSystemVisitor.cpp
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
		TestPosixFile.cpp
		TestReadOnlyFile.cpp
		TestSmallBufferStack.cpp
		TestTreeValue.cpp
		TestTypedCacheManager.cpp
	)
	
if(WIN32)
set (PlatformSources
		TestWindowsFile.cpp
		TestFileLockWindows.cpp
)
elseif(LINUX)
set (PlatformSources
		TestFileLockLinux.cpp
)
endif()

set (Headers
		DiskFileTester.h
		MinimalTreeBuilder.h
		FileInterfaceTester.h
		FileLockingTester.h
		FileSystemUtility.h
	)
	

source_group("" FILES ${Sources} ${PlatformSources} ${Headers})

add_executable(${PROJECT_NAME} ${Sources} ${PlatformSources} ${Headers})
target_link_libraries(${PROJECT_NAME} PUBLIC CompoundFs gtest)
