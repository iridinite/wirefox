# required to ensure asio does not depend on boost
target_compile_definitions(Wirefox
  PRIVATE
    -DASIO_STANDALONE)

# include paths
target_include_directories(Wirefox
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
