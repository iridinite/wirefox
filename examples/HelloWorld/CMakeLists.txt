set(LIBRARY_NAME "DemoHelloWorld")
include(${CMAKE_SOURCE_DIR}/cmake/SampleProjectSetup.cmake)

target_sources(${LIBRARY_NAME} PRIVATE
    Main.cpp
)
