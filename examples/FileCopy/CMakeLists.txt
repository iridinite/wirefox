set(LIBRARY_NAME "DemoFileCopy")
include(${CMAKE_SOURCE_DIR}/cmake/SampleProjectSetup.cmake)

target_sources(${LIBRARY_NAME} PRIVATE
    Main.cpp
)
