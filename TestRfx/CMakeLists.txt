

project(TestRfx)

set(CMAKE_CXX_STANDARD 20)

set (Sources
	TestCompressedInteger.cpp
	TestBlob.cpp
	TestFixupTable.cpp
	TestPushBits.cpp
	TestRandomGen.cpp
	TestStream.cpp
	TestStreamRule.cpp
	TestTypeUtils.cpp
	TestVersionedStructs.cpp
	)
	
if(WIN32)
set (PlatformSources
)
elseif(LINUX)
set (PlatformSources
)
endif()

set (Headers
	RandomGen.h
	VersionedStructs.h
	)
	

source_group("" FILES ${Sources} ${PlatformSources} ${Headers})

add_executable(${PROJECT_NAME} ${Sources} ${PlatformSources} ${Headers})
target_link_libraries(${PROJECT_NAME} PUBLIC Rfx gtest gtest_main)
