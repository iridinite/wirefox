cmake_minimum_required(VERSION 3.13)
cmake_policy(VERSION 3.13)

# We need the source paths to be absolute (CMP0076) so enforcing 3.13 is by far easiest
cmake_policy(SET CMP0076 NEW)

option(BUILD_SHARED_LIBS "Make shared library instead of static library" OFF)
option(BUILD_C_BINDINGS "Build the C bindings library. You need this if you want to use C#." ON)
option(BUILD_EXAMPLES "Include the examples in the generated project files" OFF)
option(BUILD_EXAMPLES_CSHARP "Also build examples that require a C# compiler" OFF)
option(BUILD_TESTS "Include the unit tests in the generated project files" OFF)
option(WARNINGS_PEDANTIC "Turn the warnings up to 11 (as they should be)" ON)
option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

project(Wirefox CXX)
include(${CMAKE_SOURCE_DIR}/cmake/PrecompiledHeader.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Macros.cmake)

# ------- main project -------

# require C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(LIBRARY_NAME "Wirefox")
add_library(Wirefox "")

# explicitly require threading library (for GCC)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(Wirefox Threads::Threads)

# required to ensure asio does not depend on boost
add_definitions(-DASIO_STANDALONE)

# setup for shared library compilation
if(BUILD_SHARED_LIBS)
  add_definitions(-DWIREFOX_SHARED_LIB)
  target_compile_definitions(Wirefox PRIVATE -DWIREFOX_COMPILING -D_DLL)
endif()
if(BUILD_C_BINDINGS)
  add_definitions(-DWIREFOX_BUILD_C_BINDINGS)
endif()

include(${CMAKE_SOURCE_DIR}/cmake/CompilerSpecific.cmake)
add_subdirectory(include)
add_subdirectory(source)

# can't get pch to work on gcc/clang for now so just leave it I guess
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  add_precompiled_header(Wirefox PCH.h FORCEINCLUDE)
endif()

# ------- examples -------

if(BUILD_EXAMPLES)
  add_subdirectory(examples)
  if(BUILD_EXAMPLES_CSHARP)
    add_subdirectory(extras/WirefoxCSharp)
  endif()
endif()
if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
