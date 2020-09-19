
cmake_minimum_required(VERSION 3.11)

project(CompoundFs)
enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(gtest_force_shared_crt ON CACHE BOOL "Override option" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "Override option" FORCE)
add_definitions ( -D_UNICODE )
add_definitions ( -D_ITERATOR_DEBUG_LEVEL=0 )

source_group("" FILES 
	.clang-format
	.gitignore
	README.md
	)

include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.10.0
)

FetchContent_GetProperties(googletest)
if(NOT googletest_POPULATED)
  FetchContent_Populate(googletest)
  add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
endif()

FetchContent_Declare(
  lz4
  GIT_REPOSITORY https://github.com/lz4/lz4.git
  GIT_TAG        v1.9.2
)

FetchContent_GetProperties(lz4)
if(NOT lz4_POPULATED)
  FetchContent_Populate(lz4)
  add_subdirectory(${lz4_SOURCE_DIR}/contrib/cmake_unofficial ${lz4_BINARY_DIR})
endif()

add_subdirectory(CompoundFs)
add_subdirectory(TestDriver)
add_subdirectory(Sandbox)

add_test(NAME CompoundFsUnitTests COMMAND TestDriver)

