# Common logic for example projects
add_executable(${LIBRARY_NAME} "")
target_link_libraries(${LIBRARY_NAME} PRIVATE Wirefox)
include(${CMAKE_SOURCE_DIR}/cmake/CompilerSpecific.cmake)
copy_wirefox_library()
