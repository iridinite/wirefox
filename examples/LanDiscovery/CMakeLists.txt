set(LIBRARY_NAME "DemoLAN")
include(${CMAKE_SOURCE_DIR}/cmake/SampleProjectSetup.cmake)

target_sources(${LIBRARY_NAME} PRIVATE
    Main.cpp
)
