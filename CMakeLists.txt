
cmake_minimum_required(VERSION 3.18)

project(CompoundFs)

set(CMAKE_CXX_STANDARD 17)
add_definitions ( -D_UNICODE )
add_definitions ( -D_ITERATOR_DEBUG_LEVEL=0 )

source_group("" FILES 
	.clang-format
	.gitignore
	README.md
	)


# include(FetchContent)

# FetchContent_Declare(
  # googletest
  # GIT_REPOSITORY https://github.com/google/googletest.git
  # GIT_TAG        release-1.10.0
# )

# set(gtest_force_shared_crt ON)
# FetchContent_MakeAvailable(googletest)

add_subdirectory(CompoundFs)
add_subdirectory(TestDriver)
add_subdirectory(Sandbox)

