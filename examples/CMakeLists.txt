add_subdirectory(HelloWorld)
add_subdirectory(Chat)
add_subdirectory(FileCopy)
add_subdirectory(LanDiscovery)

if(BUILD_EXAMPLES_CSHARP)
  add_subdirectory(Battleships)
endif()
