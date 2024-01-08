

project(TestRfx)

set(CMAKE_CXX_STANDARD 20)

set (Sources
	TestCompressedInteger.cpp
	TestBlob.cpp
	TestPushBits.cpp
	TestStream.cpp
	)
	
if(WIN32)
set (PlatformSources
)
elseif(LINUX)
set (PlatformSources
)
endif()

set (Headers
	)
	

source_group("" FILES ${Sources} ${PlatformSources} ${Headers})

add_executable(${PROJECT_NAME} ${Sources} ${PlatformSources} ${Headers})
target_link_libraries(${PROJECT_NAME} PUBLIC Rfx gtest gtest_main)
