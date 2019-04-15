#include "PCH.h"

#ifdef WIREFOX_BUILD_C_BINDINGS
#include "WirefoxCBindings.h"
#include "Wirefox.h"

namespace {
    
    IPeer* HandleToPeer(HWirefoxPeer* handle) {
        return reinterpret_cast<IPeer*>(handle);
    }

    HWirefoxPeer* PeerToHandle(IPeer* handle) {
        return reinterpret_cast<HWirefoxPeer*>(handle);
    }

    Packet* HandleToPacket(HPacket* handle) {
        return reinterpret_cast<Packet*>(handle);
    }

    HPacket* PacketToHandle(Packet* packet) {
        return reinterpret_cast<HPacket*>(packet);
    }

    std::vector<std::unique_ptr<IPeer>> m_handlesPeer;
    std::vector<std::unique_ptr<Packet>> m_handlesPacket;

    cfg::LockableMutex m_handleTableMutex;

}

HWirefoxPeer* wirefox_peer_create(size_t maxPeers) {
    auto uptr = IPeer::Factory::Create(maxPeers);

    // store a handle in the global list. this way we can nicely deal with the unique_ptrs,
    // without completely circumventing the API's safety measures
    WIREFOX_LOCK_GUARD(m_handleTableMutex);
    m_handlesPeer.push_back(std::move(uptr));

    return PeerToHandle(m_handlesPeer.back().get());
}

void wirefox_peer_destroy(HWirefoxPeer* handle) {
    if (handle == nullptr) return;

    WIREFOX_LOCK_GUARD(m_handleTableMutex);

    // find the unique_ptr for this handle, and erase() it from the vector to delete it
    IPeer* ptr = HandleToPeer(handle);
    auto it = std::find_if(m_handlesPeer.begin(), m_handlesPeer.end(), [ptr](const auto& uptr) {
        return uptr.get() == ptr;
    });

    if (it != m_handlesPeer.end())
        m_handlesPeer.erase(it);

    //delete HandleToPeer(handle);
}

int wirefox_peer_bind(HWirefoxPeer* handle, ESocketProtocol protocol, uint16_t port) {
    return HandleToPeer(handle)->Bind(static_cast<SocketProtocol>(protocol), port) ? 1 : 0;
}

EConnectAttemptResult wirefox_peer_connect(HWirefoxPeer* handle, const char* host, uint16_t port) {
    return static_cast<EConnectAttemptResult>(HandleToPeer(handle)->Connect(host, port));
}

void wirefox_peer_send_loopback(HWirefoxPeer* handle, HPacket* packet) {
    Packet* typedPacket = HandleToPacket(packet);
    HandleToPeer(handle)->SendLoopback(*typedPacket);
}

int wirefox_peer_send(HWirefoxPeer* handle, HPacket* packet, TPeerID recipient, EPacketOptions options) {
    return HandleToPeer(handle)->Send(*HandleToPacket(packet), static_cast<PeerID>(recipient), static_cast<PacketOptions>(options))
        ? 1 : 0;
}

HPacket* wirefox_peer_receive(HWirefoxPeer* handle) {
    auto uptr = HandleToPeer(handle)->Receive();
    if (uptr == nullptr) return nullptr; // don't add nullptrs to the handle table

    WIREFOX_LOCK_GUARD(m_handleTableMutex);
    m_handlesPacket.push_back(std::move(uptr));

    return PacketToHandle(m_handlesPacket.back().get());
}

HPacket* wirefox_packet_create(uint8_t cmd, const uint8_t* data, size_t len) {
    auto uptr = Packet::Factory::Create(static_cast<PacketCommand>(cmd), data, len);

    WIREFOX_LOCK_GUARD(m_handleTableMutex);
    m_handlesPacket.push_back(std::move(uptr));

    return PacketToHandle(m_handlesPacket.back().get());
}

void wirefox_packet_destroy(HPacket* handle) {
    if (handle == nullptr) return;

    WIREFOX_LOCK_GUARD(m_handleTableMutex);

    // find the unique_ptr for this handle, and erase() it from the vector to delete it
    Packet* ptr = HandleToPacket(handle);
    auto it = std::find_if(m_handlesPacket.begin(), m_handlesPacket.end(), [ptr](const auto& uptr) {
        return uptr.get() == ptr;
    });

    if (it != m_handlesPacket.end())
        m_handlesPacket.erase(it);
}

const uint8_t* wirefox_packet_get_data(HPacket* packet) {
    return HandleToPacket(packet)->GetBuffer();
}

size_t wirefox_packet_get_length(HPacket* packet) {
    return HandleToPacket(packet)->GetLength();
}

uint8_t wirefox_packet_get_cmd(HPacket* packet) {
    return static_cast<uint8_t>(HandleToPacket(packet)->GetCommand());
}

TPeerID wirefox_packet_get_sender(HPacket* packet) {
    return static_cast<TPeerID>(HandleToPacket(packet)->GetSender());
}

#endif
