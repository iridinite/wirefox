# Common logic for example projects
add_executable(${LIBRARY_NAME} "")
target_link_libraries(${LIBRARY_NAME} PRIVATE Wirefox)
target_link_libraries(${LIBRARY_NAME} PRIVATE wirefox-compiler-interface)
target_link_libraries(${LIBRARY_NAME} PRIVATE wirefox-platform-interface)
copy_wirefox_library()
wirefox_platform_config(${LIBRARY_NAME})
