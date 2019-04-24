set(thisfolder ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(Wirefox
  PRIVATE
    ${thisfolder}/Wirefox.h
    ${thisfolder}/wirefox/WirefoxConfig.h
    ${thisfolder}/wirefox/WirefoxCBindings.h
    ${thisfolder}/wirefox/WirefoxTime.h
    ${thisfolder}/wirefox/BinaryStream.h
    ${thisfolder}/wirefox/Channel.h
    ${thisfolder}/wirefox/Enumerations.h
    ${thisfolder}/wirefox/Factory.h
    ${thisfolder}/wirefox/Packet.h
    ${thisfolder}/wirefox/PeerAbstract.h
    ${thisfolder}/wirefox/Serializable.h
)
target_include_directories(Wirefox
  PUBLIC
    ${thisfolder}
  PRIVATE
    ${thisfolder}/wirefox
)
