set(LIBRARY_NAME "DemoChat")
include(${CMAKE_SOURCE_DIR}/cmake/SampleProjectSetup.cmake)

target_sources(${LIBRARY_NAME} PRIVATE
    Main.cpp
    Shared.h
    ChatServer.h
    ChatServer.cpp
    ChatClient.h
    ChatClient.cpp
)
