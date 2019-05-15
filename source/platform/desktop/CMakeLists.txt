set(thisfolder ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(Wirefox
  PRIVATE
    ${thisfolder}/SocketUDP.cpp
    ${thisfolder}/SocketUDP.h
    ${thisfolder}/RemoteAddressASIO.cpp
    ${thisfolder}/RemoteAddressASIO.h
)
target_include_directories(Wirefox
  PRIVATE
    ${thisfolder}
)
