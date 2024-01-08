
cmake_minimum_required(VERSION 3.15)

project(CompoundFs)
enable_testing()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 17)
set(gtest_force_shared_crt ON CACHE BOOL "Override option" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "Override option" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "Override option" FORCE)
#set(LZ4_BUILD_LEGACY_LZ4C OFF CACHE BOOL "Override option" FORCE)
add_definitions ( -D_UNICODE )

if(NDEBUG)
	add_definitions ( -D_ITERATOR_DEBUG_LEVEL=0 )
else()
	add_definitions ( -D_ITERATOR_DEBUG_LEVEL=2 )
	#add_definitions ( /fsanitize=address )
endif()

add_subdirectory(CompoundFs)
add_subdirectory(TestDriver)
add_subdirectory(Rfx)
add_subdirectory(TestRfx)
add_subdirectory(Sandbox)

source_group("" FILES 
	.clang-format
	.gitignore
	README.md
	)

include(FetchContent)

#------------------------------------------------------------------------------

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)

FetchContent_GetProperties(googletest)
if(NOT googletest_POPULATED)
  FetchContent_Populate(googletest)
  add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
endif()

#------------------------------------------------------------------------------
if (WIN32)
FetchContent_Declare(
  pybind11
  GIT_REPOSITORY https://github.com/pybind/pybind11
  GIT_TAG        v2.11.1
)

FetchContent_GetProperties(pybind11)
if(NOT pybind11_POPULATED)
  FetchContent_Populate(pybind11)
  add_subdirectory(${pybind11_SOURCE_DIR} ${pybind11_BINARY_DIR})
  #add_library(pybind11 INTERFACE)
  #target_include_directories(pybind11 INTERFACE .)
endif()
endif()

#------------------------------------------------------------------------------

# FetchContent_Declare(
  # lz4
  # GIT_REPOSITORY https://github.com/lz4/lz4.git
  # GIT_TAG        v1.9.2
# )

# FetchContent_GetProperties(lz4)
# if(NOT lz4_POPULATED)
  # FetchContent_Populate(lz4)
  # add_subdirectory(${lz4_SOURCE_DIR}/contrib/cmake_unofficial ${lz4_BINARY_DIR})
  # target_include_directories(lz4_static INTERFACE ${lz4_SOURCE_DIR})
# endif()

#------------------------------------------------------------------------------

FetchContent_Declare(
  xxhash
  GIT_REPOSITORY https://github.com/Cyan4973/xxHash.git
  GIT_TAG        v0.8.0
)

FetchContent_GetProperties(xxhash)
if(NOT xxhash_POPULATED)
  FetchContent_Populate(xxhash)
  add_library(xxhash ${xxhash_SOURCE_DIR}/xxhash.c ${xxhash_SOURCE_DIR}/xxhash.h)
  target_include_directories(xxhash INTERFACE ${xxhash_SOURCE_DIR})
endif()

#------------------------------------------------------------------------------


#set_target_properties(gtest gtest_main lz4_static lz4cli xxhash PROPERTIES FOLDER External)
set_target_properties(gtest gtest_main xxhash PROPERTIES FOLDER External)

if(WIN32)
add_subdirectory(py_txfs)
endif()

add_test(NAME CompoundFsUnitTests COMMAND TestDriver)

