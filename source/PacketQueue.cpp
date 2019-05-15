/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#include "PCH.h"
#include "PacketQueue.h"
#include "Socket.h"
#include "RemotePeer.h"
#include "DatagramHeader.h"
#include "PacketHeader.h"
#include "Peer.h"
#include "ChannelBuffer.h"

using namespace detail;

static_assert(cfg::THREAD_SLEEP_PACKETQUEUE_TICK > 0, "wirefox::cfg::THREAD_SLEEP_PACKETQUEUE_TICK must be greater than zero");

PacketQueue::PacketQueue(Peer* peer)
    : m_peer(peer)
    , m_updateThreadAbort(false) {
    // start up I/O thread
    m_updateThread = std::thread(std::bind(&PacketQueue::ThreadWorker, this));
}

PacketQueue::~PacketQueue() {
	Stop();
}

void PacketQueue::Stop() {
	// stop thread
	m_updateThreadAbort.store(true);
	m_updateNotify.Signal();
	if (m_updateThread.joinable())
		m_updateThread.join();
}

PacketID PacketQueue::EnqueueOutgoing(const Packet& packet, RemotePeer* remote, PacketOptions options, PacketPriority priority, const Channel& channel) {
    assert(remote);
    assert(packet.GetLength() < cfg::PACKET_MAX_LENGTH);
    (void)priority; // TODO

    // if disconnect is in progress, disallow queueing of more packets
    if (remote->IsDisconnecting()) return 0;

    // serialize the packet to an opaque blob (only prepends the cmdid, but that's an implementation detail we should ignore here)
    BinaryStream fullPacketStream(packet.GetDatagramLength());
    packet.ToDatagram(fullPacketStream);
    assert(fullPacketStream.GetLength() == packet.GetDatagramLength());

    // compute how many MTU-sized blocks we need for the full blob
    const size_t CHUNK_SIZE = cfg::MTU - 100;
    const size_t segments = (fullPacketStream.GetLength() - 1) / CHUNK_SIZE + 1;

    WIREFOX_LOCK_GUARD(remote->lock);


    SequenceID containerSequenceID = 0;
    PacketID containerPacketID = remote->congestion->GetNextPacketID();
    std::set<PacketID> segmentIDs;

    // assign sequence number for ordered packets
    if (auto* chbuf = remote->GetChannelBuffer(m_peer, channel.id))
        containerSequenceID = chbuf->GetNextOutgoing();

    for (size_t i = 0; i < segments; i++) {
        OutgoingPacket meta;
        meta.id = containerPacketID;
        meta.addr = remote->addr;
        meta.remote = remote;
        meta.crypto = nullptr;
        meta.options = options;
        meta.sendNext = Time::Now();
        meta.sendCount = 0;

        // if packet is segmented, upgrade reliability, because if any of those segments get lost,
        // then the entire transmission is rendered useless
        if (segments > 1) {
            meta.id = remote->congestion->GetNextPacketID();
            meta.options = meta.options | PacketOptions::RELIABLE;
            segmentIDs.emplace(meta.id);
        }

        // build and write a packet header for this message
        size_t bufferLength = std::min(fullPacketStream.GetLength() - (i * CHUNK_SIZE), CHUNK_SIZE);
        PacketHeader header;
        header.flag_segment = i < (segments - 1); // not the last segment?
        header.flag_jumbo = packet.GetLength() >= std::numeric_limits<uint16_t>::max();
        header.id = meta.id;
        header.options = options;
        header.channel = channel.id;
        header.sequence = containerSequenceID;
        header.length = static_cast<uint32_t>(bufferLength);
        header.offset = static_cast<uint32_t>(i * CHUNK_SIZE);
        header.splitContainer = containerPacketID;
        header.splitIndex = static_cast<uint32_t>(i);

        // build the full transmissible packet, including serialized header
        header.Serialize(meta.blob);
        meta.blob.WriteBytes(fullPacketStream.GetBuffer() + header.offset, bufferLength);
        //std::cout << "Queued split packet " << header.splitContainer << "." << header.splitIndex << ", size = " << meta.blob.GetLength() << ", wrapped by " << header.id << std::endl;

        // and queue the packet
        remote->outbox.push_back(std::move(meta));
    }

    // if a receipt was requested, add this new packet id to the tracker
    if (options & PacketOptions::WITH_RECEIPT) {
        remote->receipt->Track(containerPacketID);

        if (segments > 1)
            remote->receipt->RegisterSplitPacket(containerPacketID, std::move(segmentIDs));
    }

    return containerPacketID;
}

void PacketQueue::EnqueueOutOfBand(const Packet& packet, const RemoteAddress& addr, RemotePeer* forceCryptoBy) {
    // somewhat arbitrary number, but the point is that out-of-band packets cannot be split
    assert(packet.GetLength() < cfg::MTU - 100);

    OutgoingPacket meta;
    meta.id = 0;
    meta.addr = addr;
    meta.options = PacketOptions::UNRELIABLE;
    meta.remote = &m_peer->GetRemoteByIndex(0);
    meta.crypto = (forceCryptoBy != nullptr) ? forceCryptoBy->crypto : nullptr;
    meta.sendNext = Time::Now();
    meta.sendCount = 0;

    PacketHeader header;
    header.options = PacketOptions::UNRELIABLE;
    header.length = static_cast<uint32_t>(packet.GetDatagramLength());
    header.offset = 0;
    header.Serialize(meta.blob);

    // concat the packet header and packet payload, then queue the merged blob for sending
    packet.ToDatagram(meta.blob);

    WIREFOX_LOCK_GUARD(meta.remote->lock);
    meta.remote->outbox.push_back(std::move(meta));
}

void PacketQueue::EnqueueLoopback(const Packet& packet) {
    WIREFOX_LOCK_GUARD(m_lockInbox);

    // copy packet to the back of the inbox queue
    m_inbox.push(Packet::Factory::Create(packet));
}

std::unique_ptr<Packet> PacketQueue::DequeueIncoming() {
    WIREFOX_LOCK_GUARD(m_lockInbox);

    if (m_inbox.empty())
        return nullptr;

    auto next = std::move(m_inbox.front());
    m_inbox.pop();
    return next;
}

bool PacketQueue::OutgoingPacket::HasFlag(PacketOptions test) const {
    using num_t = std::underlying_type<decltype(test)>::type;
    return (static_cast<num_t>(options) & static_cast<num_t>(test)) == static_cast<num_t>(test);
}

void PacketQueue::ThreadWorker() {
    while (!m_updateThreadAbort) {
        const size_t remotesMax = m_peer->GetMaximumPeers();
        for (size_t i = 0; i <= remotesMax; i++) {
            auto& remote = m_peer->GetRemoteByIndex(i);
            if (!remote.active) continue;

            WIREFOX_LOCK_GUARD(remote.lock);

            // give the handshaker the opportunity to resend possibly lost packets
            if (remote.handshake && !remote.handshake->IsDone())
                remote.handshake->Update();
            // various periodic updates
            if (remote.congestion)
                remote.congestion->Update();
            if (remote.receipt)
                remote.receipt->Update();

            // disconnection timeout
            auto timeout = remote.disconnect.load();
            if (timeout.IsValid() && Time::Elapsed(timeout)) {
                m_peer->DisconnectImmediate(&remote);
                return;
            }

            // skip cycle if socket hasn't fully initialized yet
            if (remote.socket == nullptr || !remote.socket->IsOpenAndReady()) continue;

            DoReadCycle(remote);
            DoWriteCycle(remote);
        }

        // sleep a maximum of x milliseconds, but wake up earlier if requested
        m_updateNotify.WaitFor(Time::FromMilliseconds(cfg::THREAD_SLEEP_PACKETQUEUE_TICK));
    }
}

void PacketQueue::DoReadCycle(RemotePeer& remote) {
    using namespace std::placeholders;

    if (remote.socket->IsReadPending()) return;

    // dispatch a new read op for this remote
    remote.socket->BeginRead(std::bind(&PacketQueue::OnReadFinished, shared_from_this(), _1, _2, _3, _4));
}

void PacketQueue::DoWriteCycle(RemotePeer& remote) {
    using namespace std::placeholders;
    if (remote.socket->IsWritePending()) return;

    OutgoingDatagram* datagram = remote.GetNextDatagram(m_peer);
    if (!datagram) return;

    // encrypt this datagram if that's enabled
    if (m_peer->GetEncryptionEnabled()) {
        // get correct crypto layer for this packet -- if DatagramBuilder set an explicit crypto layer, use that one
        EncryptionLayer* crypto = (datagram->crypto != nullptr)
            ? datagram->crypto.get()
            : remote.crypto.get();

        // if key exchange was completed already, replace the datagram blob with a ciphertext
        const bool encryptionDesired = crypto && crypto->GetCryptoEstablished();
        if (encryptionDesired) {
            datagram->blob = crypto->Encrypt(datagram->blob);

            // I don't know why this would happen, but I guess encryption could fail?
            if (crypto->GetNeedsToBail()) {
                m_peer->DisconnectImmediate(&remote);
                return;
            }
        }
    }

    // dispatch an async write op for this remote
    remote.congestion->NotifySendingBytes(datagram->id, datagram->blob.GetLength());
    remote.socket->BeginWrite(datagram->addr, datagram->blob.GetBuffer(), datagram->blob.GetLength(),
        std::bind(&PacketQueue::OnWriteFinished, shared_from_this(), &remote, datagram->id, _1, _2));
}

void PacketQueue::OnWriteFinished(RemotePeer* remote, DatagramID, bool error, size_t) {
    // would've liked to pass remote by reference, but changing the param type to RemotePeer& seems to cause a rather vague compiler error?
    //   C2661 'std::tuple<wirefox::detail::PacketQueue *,wirefox::detail::RemotePeer,std::_Ph<1>,std::_Ph<2>>::tuple': no overloaded function takes 4 arguments
    // EDIT: apparently need to use std::ref(), but I don't think directly referencing the temporary in ThreadWorker is a good idea...
    assert(remote);

    if (error)
        m_peer->DisconnectImmediate(remote);
}

void PacketQueue::OnReadFinished(bool error, const RemoteAddress& sender, const uint8_t* buffer, size_t transferred) {
    auto* remote = m_peer->GetRemoteByAddress(sender);

    // error handling: in general, on failure, disconnect
    if (error) {
        m_peer->DisconnectImmediate(remote);
        return;
    }

    if (!remote) {
        // unknown, unconnected sender
        m_peer->OnUnconnectedMessage(sender, buffer, transferred);
        return;
    }

    BinaryStream inbuffer(buffer, transferred, BinaryStream::WrapMode::READONLY);

    // if we know the remote, then the message may be encrypted
    if (remote->crypto && remote->crypto->GetCryptoEstablished()) {
        inbuffer = remote->crypto->Decrypt(inbuffer);

        // the ciphertext might be malformed for several reasons; decryption failure == bad connection
        // TODO: Vulnerability to TCP-reset-style attack: an adversary could intentially inject a corrupt datagram. Is this a problem?
        // TODO: I currently don't think so; if the goal is to kill comms, they could also just intercept & discard all datagrams.
        if (remote->crypto->GetNeedsToBail()) {
            m_peer->DisconnectImmediate(remote);
            return;
        }
    }

    // decode the datagram header if it is complete
    DatagramHeader datagramHeader;
    if (!datagramHeader.Deserialize(inbuffer)) {
        std::cerr << "PacketQueue: [Remote " << std::to_string(remote->id) << "] Received corrupt or incomplete datagram. Killing connection." << std::endl;
        m_peer->DisconnectImmediate(remote);
        return;
    }

    // the payload and header length cannot be bigger than the MTU together, as that makes no sense, and is probably an attack
    if (datagramHeader.flag_data && datagramHeader.dataLength + inbuffer.GetPosition() > cfg::MTU) {
        std::string msg = "PacketQueue: [Remote " + std::to_string(remote->id) + "] Received too big data section ("
            + std::to_string(datagramHeader.dataLength + inbuffer.GetPosition()) + ")! Killing connection.";
        std::cerr << msg << std::endl;
        m_peer->DisconnectImmediate(remote);
        return;
    }

    // handle offline messages separately (unconnected pings, or handshake fragment)
    if (!datagramHeader.flag_link) {
        m_peer->OnUnconnectedMessage(sender, inbuffer);
        return;
    }

    // edge case: remote endpoint is not allowed to send 'connected' messages, until the handshake is complete.
    if (!remote->handshake->IsDone() || remote->handshake->GetResult() != ConnectResult::OK) {
        std::cerr << "PacketQueue: [Remote " << std::to_string(remote->id) << "] Received msg with flag_link, but handshake is incomplete! Killing connection." << std::endl;
        m_peer->DisconnectImmediate(remote);
        return;
    }

#if WIREFOX_ENABLE_NETWORK_SIM
    // random chance to drop packet. Note that we do this specifically AFTER unconnected handling, because handshake isn't quite
    // resistant to packet loss, but BEFORE acks/nacks are handled, so we can also drop some internal datagrams.
    if (m_peer->PollArtificialPacketLoss()) {
        std::cerr << "PacketQueue: [Remote " << std::to_string(remote->id) << "] 'Lost' datagram " + std::to_string(datagramHeader.datagramID) + ", flag_data = " + std::to_string(datagramHeader.flag_data) << std::endl;
        return;
    }
#endif

    // Inform the congestion manager of this packet's arrival: particularly, this may queue NAKs.
    // Also, the congestion manager tells us whether this datagram is a duplicate.
    if (remote->congestion->NotifyReceivedDatagram(datagramHeader.datagramID, !datagramHeader.flag_data) == CongestionControl::RecvState::DUPLICATE) {
        std::string errmsg = "PacketQueue: [Remote " + std::to_string(remote->id) + "] Duplicate datagram recv: " + std::to_string(datagramHeader.datagramID);
        std::cerr << errmsg << std::endl;
        return;
    }

    // deal with incoming (n)acks
    if (!datagramHeader.acks.empty())
        remote->HandleAcknowledgements(datagramHeader.acks);
    if (!datagramHeader.nacks.empty())
        remote->HandleNonAcknowledgements(datagramHeader.nacks);

    // datagram may contain any number of packets, so keep parsing headers until we run out
    PacketHeader packetHeader;
    while (packetHeader.Deserialize(inbuffer)) {
        // catch some obvious corruptions
        assert(packetHeader.offset < cfg::PACKET_MAX_LENGTH);
        assert(packetHeader.length < datagramHeader.dataLength);

        // not a duplicate receive?
        if (remote->congestion->NotifyReceivedPacket(packetHeader.id) == CongestionControl::RecvState::NEW) {
            // split packet?
            if (packetHeader.flag_segment || packetHeader.offset > 0) {
                // this is a split packet, so let's reassemble it
                HandleSplitPacket(*remote, packetHeader, inbuffer);
                continue;
            }

            // construct an actual Packet instance and get moving with it
            auto packet = Packet::Factory::Create(Packet::FromDatagram(remote->id, inbuffer, packetHeader.length));

            if (packet->GetCommand() < PacketCommand::USER_PACKET)
                // don't deliver system packets to user, but handle them immediately
                m_peer->OnSystemPacket(*remote, std::move(packet));
            else
                // user data
                HandleIncomingPacket(*remote, packetHeader, std::move(packet));

        } else {
            inbuffer.Skip(packetHeader.length);
        }
    }

    // request an immediate update, so a new read will be scheduled asap
    m_updateNotify.Signal();
}

void PacketQueue::HandleSplitPacket(RemotePeer& remote, const PacketHeader& header, BinaryStream& instream) {
    remote.assembly.Insert(header, instream);

    if (auto p = remote.assembly.Reassemble(header.splitContainer))
        HandleIncomingPacket(remote, header, std::move(p));
}

void PacketQueue::HandleIncomingPacket(RemotePeer& remote, const PacketHeader& header, std::unique_ptr<Packet> packet) {
    if (ChannelBuffer* channel = remote.GetChannelBuffer(m_peer, header.channel)) {
        // add this packet to the channel buffer
        channel->Enqueue(header.sequence, std::move(packet));

        // dequeue as many packets from the channel as possible
        auto eligiblePacket = channel->Dequeue();
        while (eligiblePacket) {
            {
                WIREFOX_LOCK_GUARD(m_lockInbox);
                m_inbox.push(std::move(eligiblePacket));
            }

            eligiblePacket = channel->Dequeue();
        }

    } else {
        // no channel = no ordering, so just add to inbox and be done with it
        WIREFOX_LOCK_GUARD(m_lockInbox);
        m_inbox.push(std::move(packet));
    }
}
