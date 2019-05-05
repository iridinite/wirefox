/**
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#include "PCH.h"
#include "DatagramBuilder.h"
#include "DatagramHeader.h"
#include "RemotePeer.h"
#include "Peer.h"

using namespace detail;

namespace {

    /**
     * \brief Returns the first suitable Packet for sending.
     *
     * \param[in]   remote      This RemotePeer's outbox will be inspected.
     * \param[in]   maxLength   No packets longer than this limit will be returned.
     * \param[in]   wantResend  If true, return only resend packets. If false, return only new packets.
     */
    PacketQueue::OutgoingPacket* GetQueuedPacket(RemotePeer& remote, size_t maxLength, bool wantResend) {
        auto& outbox = remote.outbox;
        if (outbox.empty()) return nullptr;

        auto it = std::find_if(outbox.begin(), outbox.end(),
            // To be suitable for sending, packet must meet all three conditions:
            // - not exceeding max length, - in the correct queue, - timeout elapsed
            [=](const auto& outgoing) -> bool {
                return (outgoing.blob.GetLength() <= maxLength)
                    && ((wantResend && outgoing.sendCount > 0) || (!wantResend && outgoing.sendCount == 0))
                    && Time::Elapsed(outgoing.sendNext);
            }
        );

        if (it == outbox.end()) return nullptr;
        return &*it;
    }

    /**
     * \brief Tries to add a resend packet onto a datagram send queue.
     *
     * \param[out]  sendQueue   The packet will be appended to this queue.
     * \param[in]   remote      This RemotePeer's outbox will be inspected for sendable packets.
     * \param[in]   budget      Specifies how many bytes may be sent.
     */
    void Datagram_AddResendPacket(std::vector<PacketQueue::OutgoingPacket*>& sendQueue, RemotePeer& remote, size_t budget) {
        auto* outgoing = GetQueuedPacket(remote, budget, true);
        if (!outgoing) return;

        outgoing->sendNext = Time::Now() + remote.congestion->GetRetransmissionRTO(outgoing->sendCount);
        sendQueue.push_back(outgoing);
    }


    /**
     * \brief Tries to add a new, unsent packet onto a datagram send queue.
     *
     * \param[out]  sendQueue   The packet will be appended to this queue.
     * \param[in]   remote      This RemotePeer's outbox will be inspected for sendable packets.
     * \param[in,out]   budget  Specifies how many bytes may be sent.
     */
    void Datagram_AddNewPacket(std::vector<PacketQueue::OutgoingPacket*>& sendQueue, RemotePeer& remote, size_t& budget) {
        auto* outgoing = GetQueuedPacket(remote, budget, false);
        if (!outgoing) {
            budget = 0;
            return;
        }

        outgoing->sendNext = Time::Now() + remote.congestion->GetRetransmissionRTO(outgoing->sendCount);
        sendQueue.push_back(outgoing);

        assert(budget >= outgoing->blob.GetLength());
        budget -= outgoing->blob.GetLength();
    }

}

PacketQueue::OutgoingDatagram* DatagramBuilder::MakeDatagram(RemotePeer& remote, Peer* master) {
    WIREFOX_LOCK_GUARD(remote.lock);

    // no packets to send
    if (remote.outbox.empty()) return nullptr;

    // calculate our bandwidth budgets for transmission of packets
    auto budgetResend = remote.congestion->GetRetransmissionBudget();
    auto budgetSend = remote.congestion->GetTransmissionBudget();
    assert(budgetSend + budgetResend <= cfg::MTU);
    if (budgetSend == 0 && budgetResend == 0) return nullptr;

    std::vector<PacketQueue::OutgoingPacket*> sendQueue;

    // add some packets to fill up this datagram
    if (budgetResend > 0)
        Datagram_AddResendPacket(sendQueue, remote, budgetResend);

    while (budgetSend > 0) {
        // OOB datagram may never have merged packets, because the RemoteAddresses are not the same
        if (remote.IsOutOfBand() && !sendQueue.empty()) break;

        Datagram_AddNewPacket(sendQueue, remote, budgetSend);
    }

    if (sendQueue.empty()) return nullptr;
    assert(!remote.IsOutOfBand() || sendQueue.size() == 1);

    // now, build a datagram to contain all the packets in sendQueue
    PacketQueue::OutgoingDatagram datagram;
    datagram.id = remote.congestion->GetNextDatagramID();
    datagram.addr = sendQueue[0]->addr; // TODO: is this ok?
    datagram.forceCrypto = sendQueue[0]->forceCrypto; // should only ever be set for some OOB packets
    datagram.discard = Time::Now() + Time::FromSeconds(5);

    DatagramHeader header;
    header.flag_data = true;
    header.flag_link = remote.IsConnected();
    header.datagramID = datagram.id;

    for (auto* outgoing : sendQueue) {
        // check for packet that was resent too many times
        outgoing->sendCount++;
        if (outgoing->sendCount > cfg::SEND_RETRY_COUNT) {
            // notify peer of loss, if that was requested
            if (outgoing->HasFlag(PacketOptions::WITH_RECEIPT))
                master->OnMessageReceipt(outgoing->id, false);

            // unreliable packets should've been removed from the outbox (see below)
            assert(outgoing->HasFlag(PacketOptions::RELIABLE));
            assert(remote.id != 0 && "OOB socket should only send unreliable packets");

            // if this reliable packet hasn't been acked for too long, then consider this connection lost.
            std::cout << "PacketQueue::ThreadWorker: Packet " << outgoing->id << " has timed out!" << std::endl;
            master->DisconnectImmediate(&remote);
            return nullptr;
        }

        // by determining the payload length beforehand, we can write everything in one go, rather than needing another copy of the payload
        header.dataLength += outgoing->blob.GetLength();
        // keep track of which packets belong to this datagram, so we can ack them later
        datagram.packets.push_back(outgoing->id);
    }

    // concatenate the header and all payload blobs into one stream
    header.Serialize(datagram.blob);
    for (const auto* outgoing : sendQueue) {
        datagram.blob.WriteBytes(outgoing->blob);

        // if this packet is marked as unreliable, remove it from the outbox right now
        if (!outgoing->HasFlag(PacketOptions::RELIABLE))
            remote.RemovePacketFromOutbox(outgoing->id);
    }

    remote.sentbox.push_back(std::move(datagram));
    return &remote.sentbox.back();
}

PacketQueue::OutgoingDatagram* DatagramBuilder::MakeAckgram(RemotePeer& remote) {
    WIREFOX_LOCK_GUARD(remote.lock);

    std::vector<DatagramID> acks, nacks;
    remote.congestion->MakeAckList(acks, nacks);

    PacketQueue::OutgoingDatagram ackgram;
    ackgram.addr = remote.addr;
    ackgram.id = remote.congestion->GetNextDatagramID();
    ackgram.discard = Time::Now() + Time::FromSeconds(1);
    ackgram.forceCrypto = nullptr;

    // build and write a datagram containing these acks
    DatagramHeader header;
    header.flag_data = false;
    header.flag_link = true;
    header.datagramID = ackgram.id;
    header.acks.insert(header.acks.begin(), acks.begin(), acks.end()); // copy - can I move these instead maybe?
    header.nacks.insert(header.nacks.begin(), nacks.begin(), nacks.end());
    header.Serialize(ackgram.blob);

    remote.sentbox.push_back(std::move(ackgram));
    return &remote.sentbox.back();
}
