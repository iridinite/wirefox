/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

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

void wirefox_peer_stop(HWirefoxPeer* handle, unsigned linger) {
    HandleToPeer(handle)->Stop(Time::FromMilliseconds(linger));
}

EConnectAttemptResult wirefox_peer_connect(HWirefoxPeer* handle, const char* host, uint16_t port) {
    return static_cast<EConnectAttemptResult>(HandleToPeer(handle)->Connect(host, port));
}

void wirefox_peer_disconnect(HWirefoxPeer* handle, TPeerID who, unsigned linger) {
    HandleToPeer(handle)->Disconnect(who, Time::FromMilliseconds(linger));
}

void wirefox_peer_disconnect_immediate(HWirefoxPeer* handle, TPeerID who) {
    HandleToPeer(handle)->DisconnectImmediate(who);
}

void wirefox_peer_send_loopback(HWirefoxPeer* handle, HPacket* packet) {
    Packet* typedPacket = HandleToPacket(packet);
    HandleToPeer(handle)->SendLoopback(*typedPacket);
}

TPacketID wirefox_peer_send(HWirefoxPeer* handle, HPacket* packet, TPeerID recipient, EPacketOptions options, EPacketPriority priority, TChannelIndex channelIndex) {
    auto peer = HandleToPeer(handle);
    auto channel = Channel(channelIndex, peer->GetChannelModeByIndex(channelIndex));
    return peer->Send(*HandleToPacket(packet), static_cast<PeerID>(recipient), static_cast<PacketOptions>(options), static_cast<PacketPriority>(priority), channel);
}

HPacket* wirefox_peer_receive(HWirefoxPeer* handle) {
    auto uptr = HandleToPeer(handle)->Receive();
    if (uptr == nullptr) return nullptr; // don't add nullptrs to the handle table

    WIREFOX_LOCK_GUARD(m_handleTableMutex);
    m_handlesPacket.push_back(std::move(uptr));

    return PacketToHandle(m_handlesPacket.back().get());
}

TChannelIndex wirefox_peer_make_channel(HWirefoxPeer* handle, EChannelMode mode) {
    auto channel = HandleToPeer(handle)->MakeChannel(static_cast<ChannelMode>(mode));
    return channel.id;
}

EChannelMode wirefox_peer_get_channel_mode(HWirefoxPeer* handle, TChannelIndex index) {
    return static_cast<EChannelMode>(HandleToPeer(handle)->GetChannelModeByIndex(index));
}

TPeerID wirefox_peer_get_my_id(HWirefoxPeer* handle) {
    return static_cast<TPeerID>(HandleToPeer(handle)->GetMyPeerID());
}

size_t wirefox_peer_get_max_peers(HWirefoxPeer* handle) {
    return HandleToPeer(handle)->GetMaximumPeers();
}

size_t wirefox_peer_get_max_incoming_peers(HWirefoxPeer* handle) {
    return HandleToPeer(handle)->GetMaximumIncomingPeers();
}

void wirefox_peer_set_max_incoming_peers(HWirefoxPeer* handle, size_t incoming) {
    HandleToPeer(handle)->SetMaximumIncomingPeers(incoming);
}

unsigned wirefox_peer_get_ping(HWirefoxPeer* handle, TPeerID who) {
    return HandleToPeer(handle)->GetPing(static_cast<TPeerID>(who));
}

int wirefox_peer_get_ping_available(HWirefoxPeer* handle, TPeerID who) {
    return HandleToPeer(handle)->GetPingAvailable(static_cast<TPeerID>(who)) ? 1 : 0;
}

void wirefox_peer_set_network_sim(HWirefoxPeer* handle, float packetLoss, unsigned additionalPing) {
    HandleToPeer(handle)->SetNetworkSimulation(packetLoss, additionalPing);
}

void wirefox_peer_set_offline_ad(HWirefoxPeer* handle, const uint8_t* data, size_t len) {
    BinaryStream ad(data, len, BinaryStream::WrapMode::READONLY);
    HandleToPeer(handle)->SetOfflineAdvertisement(ad);
}

void wirefox_peer_disable_offline_ad(HWirefoxPeer* handle) {
    HandleToPeer(handle)->DisableOfflineAdvertisement();
}

void wirefox_peer_ping(HWirefoxPeer* handle, const char* host, uint16_t port) {
    HandleToPeer(handle)->Ping(host, port);
}

void wirefox_peer_ping_local_network(HWirefoxPeer* handle, uint16_t port) {
    HandleToPeer(handle)->PingLocalNetwork(port);
}

size_t wirefox_peer_get_crypto_key_length(HWirefoxPeer* handle) {
    return HandleToPeer(handle)->GetEncryptionKeyLength();
}

int wirefox_peer_get_crypto_enabled(HWirefoxPeer* handle) {
    return HandleToPeer(handle)->GetEncryptionEnabled() ? 1 : 0;
}

void wirefox_peer_set_crypto_enabled(HWirefoxPeer* handle, int enabled) {
    HandleToPeer(handle)->SetEncryptionEnabled(enabled != 0);
}

void wirefox_peer_set_crypto_identity(HWirefoxPeer* handle, const uint8_t* key_secret, const uint8_t* key_public) {
    HandleToPeer(handle)->SetEncryptionIdentity(key_secret, key_public);
}

void wirefox_peer_generate_crypto_identity(HWirefoxPeer* handle, uint8_t* key_secret, uint8_t* key_public) {
    HandleToPeer(handle)->GenerateIdentity(key_secret, key_public);
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

#endif // WIREFOX_BUILD_C_BINDINGS
