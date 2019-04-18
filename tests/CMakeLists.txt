set(LIBRARY_NAME "Tests")
add_executable(Tests
	Main.cpp
	BinaryStream.Tests.cpp
	Peer.Tests.cpp
)
target_include_directories(${LIBRARY_NAME}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/external/catch2/include
)
target_link_libraries(${LIBRARY_NAME} PRIVATE Wirefox)
include(${CMAKE_SOURCE_DIR}/cmake/CompilerSpecific.cmake)

copy_wirefox_library()

add_test("WirefoxTests" $<TARGET_FILE:Tests>)
