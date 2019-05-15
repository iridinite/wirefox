/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#include "PCH.h"
#include "RemotePeer.h"
#include "WirefoxConfigRefs.h"
#include "DatagramBuilder.h"
#include "EncryptionAuthenticator.h"
#include "Peer.h"

using namespace detail;

namespace {

    auto GetOutgoingPacketByID(RemotePeer& remote, PacketID packetID) {
        return std::find_if(remote.outbox.begin(), remote.outbox.end(), [packetID](const auto& p) {
            return p.id == packetID;
        });
    }

}

RemotePeer::RemotePeer()
    : assembly(this)
    , id(0)
    , disconnect(0)
    , reserved(false)
    , active(false) {}

bool RemotePeer::IsConnected() const {
    return active && handshake != nullptr && handshake->GetResult() == ConnectResult::OK;
}

void RemotePeer::HandleAcknowledgements(const std::vector<DatagramID>& acklist) {
    // TODO: profile this method; multiple linear searches are being done, is this a problem?
    WIREFOX_LOCK_GUARD(lock);

    for (auto ack : acklist) {
        // First, try to find the datagram that the remote is talking about
        auto d_it = std::find_if(sentbox.begin(), sentbox.end(), [ack](const auto& datagram) {
            return datagram.id == ack;
        });
        if (d_it != sentbox.end()) {
            // Then, try to find all packets that were a part of this datagram
            const auto& datagram = *d_it;
            for (auto packetID : datagram.packets) {
                receipt->Acknowledge(packetID);
                RemovePacketFromOutbox(packetID);
            }

            // Finally remove the datagram itself from the datagram history box
            sentbox.erase(d_it);
        }

        // inform the congestion manager of this ack also
        congestion->NotifyReceivedAck(ack);
    }
}

void RemotePeer::HandleNonAcknowledgements(const std::vector<DatagramID>& naklist) {
    WIREFOX_LOCK_GUARD(lock);

    for (auto nak : naklist) {
        // First, try to find the datagram that the remote is talking about
        auto d_it = std::find_if(sentbox.begin(), sentbox.end(), [nak](const auto& datagram) {
            return datagram.id == nak;
        });
        if (d_it != sentbox.end()) {
            // Then, try to find all packets that were a part of this datagram
            const auto& datagram = *d_it;
            for (auto packetID : datagram.packets) {
                // If this packet exists, cancel any further delay and mark it for immediate resending
                auto p_it = GetOutgoingPacketByID(*this, packetID);
                if (p_it != outbox.end())
                    p_it->sendNext = Time::Now();
            }


            // Finally remove the datagram itself from the datagram history box
            sentbox.erase(d_it);
        }
    }

    congestion->NotifyReceivedNakGroup();
}

PacketQueue::OutgoingDatagram* RemotePeer::GetNextDatagram(Peer* master) {
    // if congestion control wants to send acks, do that now.
    if (congestion->GetNeedsToSendAcks()) {
        // gather the list of desired (n)acks from the congestion manager.
        assert(IsConnected());
        return DatagramBuilder::MakeAckgram(*this);
    }

    // otherwise, look for packets to send and build a datagram out of them
    return DatagramBuilder::MakeDatagram(*this, master);
}

ChannelBuffer* RemotePeer::GetChannelBuffer(const IPeer* master, ChannelIndex index) {
    // channel zero is always unbuffered and unordered
    if (index == 0 || master->GetChannelModeByIndex(index) == ChannelMode::UNORDERED) return nullptr;

    // lazy instantiation
    if (!channels.count(index))
        channels.emplace(index, std::make_unique<ChannelBuffer>(master, index));

    return channels.at(index).get();
}

void RemotePeer::RemovePacketFromOutbox(PacketID remove) {
    auto it = GetOutgoingPacketByID(*this, remove);
    if (it != outbox.end())
        outbox.erase(it);
}

void RemotePeer::Setup(Peer* master, ConnectionOrigin origin) {
    reserved = true;
    congestion = std::make_unique<cfg::DefaultCongestionControl>();
    receipt = std::make_unique<ReceiptTracker>(master, *this);

    // used by remote #0 to stop handshake from being instantiated, as out-of-band comms should not do handshakes
    if (origin != ConnectionOrigin::INVALID) {
        crypto = std::make_shared<cfg::DefaultEncryption>();
        crypto->SetLocalIdentity(master->GetEncryptionIdentity());
        handshake = std::make_unique<cfg::DefaultHandshaker>(master, this, origin);
    }
}

void RemotePeer::Reset() {
    WIREFOX_LOCK_GUARD(lock);

    active = false;
    disconnect = 0;
    id = 0;
    addr = RemoteAddress();
    socket = nullptr;
    handshake = nullptr;
    congestion = nullptr;
    crypto = nullptr;
    receipt = nullptr;
    assembly = ReassemblyBuffer(this);
    outbox.clear();
    sentbox.clear();
    channels.clear();

    reserved = false;
}
