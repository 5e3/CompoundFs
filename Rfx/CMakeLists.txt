

project(Rfx)

set(CMAKE_CXX_STANDARD 20)

set (Sources
	test.cpp
	)

if(WIN32)
set (PlatformSources
)
else()
set (PlatformSources
)
endif()

	
set (Headers
	Blob.h
	CompressedInteger.h
	FixupTable.h
	Stream.h
	StreamRule.h
	PushBits.h
	Rfx.natvis
	)
	
if(WIN32)
set (PlatformHeaders
)
else()
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

target_link_libraries(${PROJECT_NAME} PRIVATE xxhash)
