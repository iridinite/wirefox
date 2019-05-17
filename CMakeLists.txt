cmake_minimum_required(VERSION 3.9)
cmake_policy(VERSION 3.9)

if (POLICY CMP0076)
  cmake_policy(SET CMP0076 NEW) # target_sources makes paths absolute
endif()

option(ENABLE_ENCRYPTION "Enable cryptographic features. Requires libsodium." OFF)
option(BUILD_SHARED_LIBS "Make shared library instead of static library" OFF)
option(BUILD_C_BINDINGS "Build the C bindings library. You need this if you want to use C#." ON)
option(BUILD_CSHARP_BINDINGS "Build the C# bindings library. Requires a C# compiler, obviously." OFF)
option(BUILD_EXAMPLES "Include the examples in the generated project files" OFF)
option(BUILD_EXAMPLES_CSHARP "Also build examples that require a C# compiler" OFF)
option(BUILD_TESTS "Include the unit tests in the generated project files" OFF)
option(WARNINGS_PEDANTIC "Turn the warnings up to 11 (as they should be)" ON)
option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

set(WIREFOX_PLATFORM "DESKTOP" CACHE STRING "Target platform for Wirefox library")
set_property(CACHE WIREFOX_PLATFORM PROPERTY STRINGS DESKTOP NX)

string(TOLOWER ${WIREFOX_PLATFORM} WIREFOX_PLATFORM)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

project(Wirefox CXX C)
include(${CMAKE_SOURCE_DIR}/cmake/Macros.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/PrecompiledHeader.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/CompilerSpecific.cmake)
include(${CMAKE_SOURCE_DIR}/source/platform/${WIREFOX_PLATFORM}/PlatformConfig.cmake)

# ------- main project -------

# require C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(Wirefox "")

# setup for shared library compilation
if(BUILD_SHARED_LIBS)
  add_definitions(-DWIREFOX_SHARED_LIB)
  target_compile_definitions(Wirefox PRIVATE -DWIREFOX_COMPILING -D_DLL)
endif()
if(BUILD_C_BINDINGS)
  add_definitions(-DWIREFOX_BUILD_C_BINDINGS)
endif()

add_subdirectory(external)
add_subdirectory(include)
add_subdirectory(source)

# import compiler configurations
target_link_libraries(Wirefox PRIVATE wirefox-compiler-interface)
target_link_libraries(Wirefox PRIVATE wirefox-platform-interface)
wirefox_platform_config(Wirefox)

# import native threading library
find_package(Threads REQUIRED)
target_link_libraries(Wirefox PRIVATE Threads::Threads)

# import libsodium for cryptography
if(ENABLE_ENCRYPTION)
  find_package(sodium REQUIRED)
  target_link_libraries(Wirefox PRIVATE sodium)
  add_definitions(-DWIREFOX_ENABLE_ENCRYPTION)
endif()

# can't get pch to work on gcc/clang for now so just leave it I guess
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  add_precompiled_header(Wirefox PCH.h FORCEINCLUDE)
endif()

# ------- optional extensions -------

if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
if(BUILD_CSHARP_BINDINGS OR BUILD_EXAMPLES_CSHARP)
  add_subdirectory(extras/WirefoxCSharp)
endif()
if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
