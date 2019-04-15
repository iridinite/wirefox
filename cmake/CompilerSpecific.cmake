# Copyright (C) 2008-2019 TrinityCore <https://www.trinitycore.org/>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

if(WIN32)
  include(${CMAKE_SOURCE_DIR}/cmake/Platform/Windows.cmake)
#elseif(UNIX)
#  include(${CMAKE_SOURCE_DIR}/cmake/Platform/Linux.cmake)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  include(${CMAKE_SOURCE_DIR}/cmake/Compiler/MSVC.cmake)
elseif (CMAKE_CXX_PLATFORM_ID MATCHES "MinGW")
  message(FATAL_ERROR "MingW settings not implemented")
  include(${CMAKE_SOURCE_DIR}/cmake/Compiler/MingW.cmake)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  message(FATAL_ERROR "Clang settings not implemented")
  include(${CMAKE_SOURCE_DIR}/cmake/Compiler/Clang.cmake)
endif()
