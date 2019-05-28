set(thisfolder ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(Wirefox
  PRIVATE
    ${thisfolder}/AwaitableEvent.cpp
    ${thisfolder}/AwaitableEvent.h
    ${thisfolder}/BinaryStream.cpp
    ${thisfolder}/Channel.cpp
    ${thisfolder}/ChannelBuffer.cpp
    ${thisfolder}/ChannelBuffer.h
    ${thisfolder}/CongestionControl.cpp
    ${thisfolder}/CongestionControl.h
    ${thisfolder}/CongestionControlWindow.cpp
    ${thisfolder}/CongestionControlWindow.h
    ${thisfolder}/DatagramBuilder.cpp
    ${thisfolder}/DatagramBuilder.h
    ${thisfolder}/DatagramHeader.cpp
    ${thisfolder}/DatagramHeader.h
    ${thisfolder}/EncryptionAuthenticator.cpp
    ${thisfolder}/EncryptionAuthenticator.h
    ${thisfolder}/EncryptionLayer.h
    ${thisfolder}/EncryptionLayerNull.h
    ${thisfolder}/EncryptionLayerSodium.cpp
    ${thisfolder}/EncryptionLayerSodium.h
    ${thisfolder}/Handshaker.cpp
    ${thisfolder}/Handshaker.h
    ${thisfolder}/HandshakerThreeWay.cpp
    ${thisfolder}/HandshakerThreeWay.h
    ${thisfolder}/PCH.cpp
    ${thisfolder}/PCH.h
    ${thisfolder}/Packet.cpp
    ${thisfolder}/PacketHeader.cpp
    ${thisfolder}/PacketHeader.h
    ${thisfolder}/PacketQueue.cpp
    ${thisfolder}/PacketQueue.h
    ${thisfolder}/Peer.cpp
    ${thisfolder}/Peer.h
    ${thisfolder}/PeerStats.cpp
    ${thisfolder}/ReassemblyBuffer.cpp
    ${thisfolder}/ReassemblyBuffer.h
    ${thisfolder}/ReceiptTracker.cpp
    ${thisfolder}/ReceiptTracker.h
    ${thisfolder}/RemotePeer.cpp
    ${thisfolder}/RemotePeer.h
    ${thisfolder}/RpcController.cpp
    ${thisfolder}/RpcController.h
    ${thisfolder}/Socket.h
    ${thisfolder}/WirefoxCBindings.cpp
    ${thisfolder}/WirefoxConfigRefs.h
    ${thisfolder}/WirefoxTime.cpp
)
target_include_directories(Wirefox
  PRIVATE
    ${thisfolder}
)

# Nintendo Switch bindings
if (${WIREFOX_PLATFORM} STREQUAL "nx")
  if (NOT IS_DIRECTORY "${thisfolder}/platform/nx")
    message(FATAL_ERROR "The Nintendo Switch bindings are missing. Due to NDA restrictions, these are not included in the open source project. Please contact mikamolenkamp@hotmail.com to obtain a copy of the required files, or set WIREFOX_PLATFORM back to DESKTOP.")
  endif()
  add_subdirectory(platform/nx)
else()
  add_subdirectory(platform/desktop)
endif()
