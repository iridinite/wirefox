/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#include "PCH.h"
#include "Peer.h"
#include "PacketQueue.h"
#include "RemotePeer.h"
#include "PacketHeader.h"
#include "WirefoxConfigRefs.h"
#include "EncryptionAuthenticator.h"
#include "DatagramHeader.h"
#include "Channel.h"

using namespace wirefox::detail;

std::unique_ptr<IPeer> IPeer::Factory::Create(size_t maxPeers) {
    return std::make_unique<Peer>(maxPeers);
}

Peer::Peer(size_t maxPeers)
    : m_id(GeneratePeerID())
    , m_remotesMax(maxPeers + 1)
    , m_remotesIncoming(0)
    , m_advertisement(0)
    , m_masterSocket(cfg::DefaultSocket::Create())
    , m_remotes(std::unique_ptr<RemotePeer[]>(new RemotePeer[m_remotesMax]))
    , m_queue(std::make_shared<PacketQueue>(this))
    , m_channels{ChannelMode::UNORDERED}
    , m_crypto_enabled(false) {}

Peer::Peer(Peer&& other) noexcept
    : m_id(0)
    , m_remotesMax(0)
    , m_remotesIncoming(0)
    , m_advertisement(0)
    , m_crypto_enabled(false) {
    *this = std::move(other);
}

Peer::~Peer() {
    // clean shutdown, stop worker threads before deallocating everything
    m_masterSocket->Unbind();
    m_queue->Stop();
}

Peer& Peer::operator=(Peer&& other) noexcept {
    if (this != &other) {
        // steal internal state
        m_id = other.m_id;
        m_remotesMax = other.m_remotesMax;
        m_remotesIncoming = other.m_remotesIncoming;
        other.m_id = 0;
        other.m_remotesMax = 0;
        other.m_remotesIncoming = 0;

#if WIREFOX_ENABLE_NETWORK_SIM
        m_simLossRate = other.m_simLossRate;
        m_simExtraPing = other.m_simExtraPing;
        m_simrng = other.m_simrng;
        m_simqueue = std::move(other.m_simqueue);
        other.m_simLossRate = 0;
        other.m_simExtraPing = 0;
#endif

        m_masterSocket = std::move(other.m_masterSocket);
        m_remotes = std::move(other.m_remotes);
        m_queue = std::move(other.m_queue);
        m_remoteLookup = std::move(other.m_remoteLookup);
        m_channels = std::move(other.m_channels);
    }

    return *this;
}

ConnectAttemptResult Peer::Connect(const std::string& host, uint16_t port, const uint8_t* public_key) {
    auto* slot = GetNextAvailableConnectSlot();
    if (!slot) return ConnectAttemptResult::NO_FREE_SLOTS;

    slot->Setup(this, ConnectionOrigin::SELF);

    if (public_key) {
        // Cannot specify public key while also having crypto disabled, that's dangerous because
        // our only other option here is to silently discard the key, which would lead to the user thinking
        // they're having an encrypted conversation when they really aren't. Therefore, treat as error.
        if (!GetEncryptionEnabled()) {
            slot->Reset();
            return ConnectAttemptResult::INVALID_PARAMETER;
        }

        // tell this remote's crypto layer to expect this exact public key
        BinaryStream public_key_stream(public_key, cfg::DefaultEncryption::GetKeyLength(), BinaryStream::WrapMode::READONLY);
        slot->crypto->ExpectRemoteIdentity(public_key_stream);
    }

    auto ret = m_masterSocket->Connect(host, port, [this, slot](bool error, RemoteAddress addr, std::shared_ptr<Socket> socket, std::string) {
        if (error) {
            // internal error, probably?
            assert(false && "internal error in Socket::Connect");
            DisconnectImmediate(slot);
            return;
        }

        slot->addr = addr;
        slot->socket = std::move(socket);
        SetupRemotePeerCallbacks(slot);
        slot->handshake->Begin();
        slot->active = true;
    });

    // make sure to un-reserve the slot we just prepared, if the connect attempt never went out
    if (ret != ConnectAttemptResult::OK)
        slot->Reset();

    return ret;
}

bool Peer::Bind(SocketProtocol family, uint16_t port) {
    // Remote 0 is a special reserved slot, where out-of-band (i.e. unconnected) communication is performed.
    // We do it this way so PacketQueue requires no extra logic when iterating over the remotes array.
    m_remotes[0].Setup(this, ConnectionOrigin::INVALID);
    m_remotes[0].socket = m_masterSocket;
    m_remotes[0].active = true;

    return m_masterSocket->Bind(family, port);
}

void Peer::Disconnect(PeerID who, Timespan linger) {
    auto* remote = GetRemoteByID(who);
    if (remote == nullptr) return;

    // send out a request to disconnect
    Packet request(PacketCommand::DISCONNECT_REQUEST, nullptr, 0);
    m_queue->EnqueueOutgoing(request, remote, PacketOptions::RELIABLE, PacketPriority::CRITICAL, Channel());

    // this prevents further packets from being queued
    remote->disconnect = Time::Now() + linger;
}

void Peer::DisconnectImmediate(PeerID who) {
    auto* remote = GetRemoteByID(who);
    if (remote == nullptr) return;

    DisconnectImmediate(remote);
}

void Peer::DisconnectImmediate(RemotePeer* remote) {
    // if there is no handshake component, then this is not a connection, but the OOB socket.
    // and we really can't reset the OOB socket, that makes no sense.
    if (!remote || !remote->handshake) return;

    if (remote->handshake->IsDone() && remote->handshake->GetResult() == ConnectResult::OK)
        // connection was already established, so it was lost now
        this->OnDisconnect(*remote, remote->IsDisconnecting()
            ? PacketCommand::NOTIFY_DISCONNECTED
            : PacketCommand::NOTIFY_CONNECTION_LOST);
    else
        // if handshake was not yet complete, then treat the remote endpoint as unreachable
        remote->handshake->Complete(ConnectResult::CONNECT_FAILED);

    // ... but in any case, kill the connection
    remote->Reset();
}

void Peer::Stop(Timespan linger) {
    for (size_t i = 1 /* skip oob socket */; i < m_remotesMax; i++) {
        auto& slot = m_remotes[i];
        if (slot.IsConnected() && linger > 0)
            // if connection was established, perform graceful disconnect
            Disconnect(slot.id, linger);
        else
            // otherwise (or if graceful is disabled), at least prevent new outgoing messages
            slot.disconnect = Time::Now() + linger;
    }

    // block the main thread for the specified linger duration, so that the network
    // threads have the time they need to perform all graceful disconnections
    if (linger > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(Time::ToMilliseconds(linger)));

    // finally, actually kill all connections and clean up the entire remotes array
    for (size_t i = 1 /* skip oob socket */; i < m_remotesMax; i++)
        m_remotes[i].Reset();
    // and stop network activity
    m_masterSocket->Unbind();
}

PacketID Peer::Send(const Packet& packet, PeerID recipient, PacketOptions options, PacketPriority priority, const Channel& channel) {
    // sanity check, hard cap on data length
    if (packet.GetLength() > cfg::PACKET_MAX_LENGTH) return 0;

    // translate peerID to remote
    auto* remote = GetRemoteByID(recipient);
    if (remote == nullptr) return 0;

    return m_queue->EnqueueOutgoing(packet, remote, options, priority, channel);
}

void Peer::SendLoopback(const Packet& packet) {
    m_queue->EnqueueLoopback(packet);
}

void Peer::Ping(const std::string& hostname, uint16_t port) const {
    RemoteAddress addr;
    if (!m_masterSocket->Resolve(hostname, port, addr)) return;

    Packet ping(PacketCommand::PING, nullptr, 0);
    m_queue->EnqueueOutOfBand(ping, addr, nullptr);
}

void Peer::PingLocalNetwork(uint16_t port) const {
    // get an appropriate multicast address based on the socket family
    std::string multicast;
    switch (m_masterSocket->GetProtocol()) {
    case SocketProtocol::IPv4:
        multicast = "255.255.255.255";
        break;
    case SocketProtocol::IPv6:
        multicast = "FF02::1";
        break;
    }

    // queue as normal OOB ping packet
    Ping(multicast, port);
}

void Peer::SetEncryptionEnabled(bool enabled) {
    // settings cannot be changed after the socket is bound
    if (m_masterSocket->IsOpenAndReady()) return;

#ifdef WIREFOX_ENABLE_ENCRYPTION
    if (enabled) {
        // generate a random keypair and enable crypto
        m_crypto_identity = cfg::DefaultEncryption::Keypair::CreateIdentity();
        m_crypto_enabled = true;
    } else {
        // release keypair
        m_crypto_identity = nullptr;
        m_crypto_enabled = false;
    }
#else
    (void)enabled;
    m_crypto_enabled = false;
#endif
}

void Peer::SetEncryptionIdentity(const uint8_t* key_secret, const uint8_t* key_public) {
    if (!m_crypto_enabled) return;

#ifdef WIREFOX_ENABLE_ENCRYPTION
    m_crypto_identity = std::make_shared<cfg::DefaultEncryption::Keypair>(key_secret, key_public);
#else
    (void)key_secret;
    (void)key_public;
#endif
}

void Peer::GenerateIdentity(uint8_t* key_secret, uint8_t* key_public) const {
#ifdef WIREFOX_ENABLE_ENCRYPTION
    auto keypair = cfg::DefaultEncryption::Keypair::CreateIdentity();
    keypair->CopyTo(key_secret, key_public);
#else
    (void)key_secret;
    (void)key_public;
#endif
}

size_t Peer::GetEncryptionKeyLength() const {
    return cfg::DefaultEncryption::GetKeyLength();
}

bool Peer::GetEncryptionEnabled() const {
    return m_crypto_enabled;
}

std::shared_ptr<EncryptionLayer::Keypair> Peer::GetEncryptionIdentity() const {
    return m_crypto_identity;
}

void Peer::SendOutOfBand(const Packet& packet, const RemoteAddress& addr) {
    m_queue->EnqueueOutOfBand(packet, addr, nullptr);
}

std::unique_ptr<Packet> Peer::Receive() {
#if WIREFOX_ENABLE_NETWORK_SIM
    // put received packets into a delay queue
    while (auto p = m_queue->DequeueIncoming()) {
        m_simqueue.push_back(std::make_pair(std::move(p), Time::Now() + Time::FromMilliseconds(m_simExtraPing)));
    }
    // release packets from the queue that have had their extra ping delay elapsed.
    // I expect this to be turned off in release builds, so this overhead should be acceptable during debug.
    auto itr = m_simqueue.begin();
    while (itr != m_simqueue.end()) {
        if (Time::Elapsed(itr->second)) {
            auto p = std::move(itr->first);
            m_simqueue.erase(itr);
            return p;
        }
        ++itr;
    }

    return nullptr;

#else
    return m_queue->DequeueIncoming();
#endif
}

void Peer::SetOfflineAdvertisement(const BinaryStream& data) {
    m_advertisement = data;
}

void Peer::DisableOfflineAdvertisement() {
    m_advertisement.Reset();
}

Channel Peer::MakeChannel(ChannelMode mode) {
    // Register a new channel if we still have space left for one
    if (m_channels.size() < std::numeric_limits<ChannelIndex>::max()) {
        m_channels.push_back(mode);
        return {static_cast<ChannelIndex>(m_channels.size() - 1), mode};
    }

    // Otherwise return unordered basic channel 0 (as a failsafe)
    return {0, ChannelMode::UNORDERED};
}

ChannelMode Peer::GetChannelModeByIndex(ChannelIndex index) const {
    return m_channels.at(index);
}

void Peer::GetAllConnectedPeers(std::vector<PeerID>& output) const {
    for (size_t i = 1 /* skip oob socket */; i < m_remotesMax; i++) {
        auto& slot = m_remotes[i];
        if (slot.IsConnected())
            output.push_back(slot.id);
    }
}

bool Peer::GetPingAvailable(PeerID who) const {
    auto* remote = GetRemoteByID(who);
    if (!remote) return false;

    return remote->congestion->GetRTTHistoryAvailable();
}

unsigned Peer::GetPing(PeerID who) const {
    auto* remote = GetRemoteByID(who);
    if (!remote) return 0;

    return remote->congestion->GetAverageRTT();
}

const PeerStats* Peer::GetStats(PeerID who) const {
    auto* remote = GetRemoteByID(who);
    if (!remote) return nullptr;

    return &remote->stats;
}

void SendRejectionReply(Peer* peer, const RemoteAddress& addr, ConnectResult reason) {
    // build handshake error message
    BinaryStream reply;
    cfg::DefaultHandshaker::WriteOutOfBandErrorReply(reply, peer->GetMyPeerID(), reason);

    // send error response on OOB socket
    Packet packet(PacketCommand::CONNECT_ATTEMPT, std::move(reply));
    peer->SendOutOfBand(packet, addr);
}

void Peer::OnNewIncomingPeer(const RemoteAddress& addr, const Packet& packet) {
    // TODO: IP rate limit check, as countermeasure against SYN flood-style DoS attack

    // looks like this is going somewhere; let's reserve a RemotePeer slot for this new fellow
    auto* remote = GetNextAvailableIncomingSlot();
    if (!remote) {
        // no slots available :(
        SendRejectionReply(this, addr, ConnectResult::NO_FREE_SLOTS);
        return;
    }

    // configure the new remote
    remote->Setup(this, ConnectionOrigin::REMOTE);
    SetupRemotePeerCallbacks(remote);
    remote->socket = m_masterSocket; // TODO: FIX ME! Incompatible with future TCP implementation. TCP needs to hand us a new socket from an acceptor!
    remote->addr = addr;
    remote->active = true;
    remote->handshake->Handle(packet);
}

void Peer::OnDisconnect(RemotePeer& remote, PacketCommand cmd) {
    // make sure to unblock all RPCs that were waiting on this
    m_rpc.SignalAllBlockingReplies(remote.id);

    // remove from lookup tables
    m_remoteLookup.erase(remote.id);

    // no notification if we're already expecting the disconnect
    //if (remote.IsDisconnecting()) return;

    // post a notification for the local user
    Packet notification(cmd, nullptr, 0);
    notification.SetSender(remote.id);
    SendLoopback(notification);
}

void Peer::OnSystemPacket(RemotePeer& remote, std::unique_ptr<Packet> packet) {
    switch (packet->GetCommand()) {
    case PacketCommand::DISCONNECT_REQUEST: {
        OnDisconnect(remote, PacketCommand::NOTIFY_DISCONNECTED);

        Packet dc_ack(PacketCommand::DISCONNECT_ACK, nullptr, 0);
        m_queue->EnqueueOutOfBand(dc_ack, remote.addr, &remote);

        remote.Reset();
        break;
    }
    case PacketCommand::RPC_SIGNAL: {
        // get the RPC identifier and its payload back out of the packet
        auto instream = packet->GetStream();
        auto ident = instream.ReadString();
        auto blocking = instream.ReadBool();

        auto paramslen = instream.Read7BitEncodedInt();
        BinaryStream params(paramslen);
        if (paramslen > 0)
            instream.ReadBytesIntoStream(params, paramslen);

        if (blocking) {
            // run the synchronous RPC and record the response
            BinaryStream outstream;
            outstream.WriteString(ident);
            m_rpc.SignalBlocking(ident, *this, packet->GetSender(), params, outstream);

            // enqueue response notification
            Packet rpcReply(PacketCommand::RPC_BLOCKING_REPLY, std::move(outstream));
            Send(rpcReply, packet->GetSender(), PacketOptions::RELIABLE, PacketPriority::MEDIUM, Channel());

        } else {
            // invoke the RPC as a fire-and-forget
            m_rpc.Signal(ident, *this, packet->GetSender(), params);
        }

        break;
    }
    case PacketCommand::RPC_BLOCKING_REPLY: {
        // received response to blocking RPC, read identifier from packet
        auto instream = packet->GetStream();
        auto ident = instream.ReadString();

        // unblock the waiting thread
        m_rpc.SignalBlockingReply(ident, packet->GetSender(), instream);
        break;
    }
    default:
        assert(false && "unexpected system packet received");
        break;
    }
}

void Peer::OnUnconnectedMessage(const RemoteAddress& addr, BinaryStream& instream) {
    PacketHeader header;

    // for this unconnected (internal) packet to make sense, it must have data (a command for us), and must not be a split packet.
    // if this assert trips, the packet is malformed. in a debug env, this should not happen, but in the wild it's possible of course
    if (!header.Deserialize(instream) || header.flag_segment) {
        assert(false);
        return;
    }

    Packet packet = Packet::FromDatagram(0, instream, header.length);
    switch (packet.GetCommand()) {
    case PacketCommand::PING: {
        // check if we have an advert configured, otherwise ignore the ping
        if (m_advertisement.IsEmpty()) break;

        // send back our ad
        Packet pong(PacketCommand::ADVERTISEMENT, m_advertisement);
        SendOutOfBand(pong, addr);

        break;
    }
    case PacketCommand::CONNECT_ATTEMPT: {
        // this is part of a connection setup

        RemotePeer* remote = GetRemoteByAddress(addr);
        if (!remote) {
            // new address we haven't seen before, treat as a new connection request
            OnNewIncomingPeer(addr, packet);
            break;
        }
        // if handshake already complete, ignore this message, it's likely a duplicate or delayed (re)send
        if (remote->handshake->IsDone())
            break;

        remote->handshake->Handle(packet);

        // if handshake failed, discard this remote endpoint
        // edit: Handshaker::Complete() already calls remote->Reset() on failure
        //if (remote->handshake->IsDone() && remote->handshake->GetResult() != ConnectResult::OK)
        //remote->Reset();

        break;
    }
    case PacketCommand::DISCONNECT_ACK: {
        // disconnect completed on remote end, hence flag_link is unset on this packet

        RemotePeer* remote = GetRemoteByAddress(addr);
        if (!remote) return;

        // finish the disconnect locally as well
        DisconnectImmediate(remote);

        break;
    }
    case PacketCommand::ADVERTISEMENT: {
        // copy the advert into a new buffer, and prefix it with the sender's IP address
        std::string address = addr.ToString();
        BinaryStream original = packet.GetStream();
        BinaryStream prefixed(original.GetLength() + address.size() + sizeof(int));
        prefixed.WriteString(address);
        prefixed.WriteBytes(original);

        // send this notification to the local peer
        Packet notification(PacketCommand::NOTIFY_ADVERTISEMENT, std::move(prefixed));
        m_queue->EnqueueLoopback(notification);

        break;
    }
    default:
        std::cerr << "Peer::OnUnconnectedMessage: received command " << int(packet.GetCommand()) << " which is unknown or not allowed offline" << std::endl;
        break;
    }
}

void Peer::OnUnconnectedMessage(const RemoteAddress& addr, const uint8_t* msg, size_t msglen) {
    // wrap the raw buffer into a stream.
    // note that we specifically do not deal with split packets here, they shouldn't appear for now.
    BinaryStream instream(msg, msglen, BinaryStream::WrapMode::READONLY);
    DatagramHeader header;
    if (!header.Deserialize(instream)) return;

    // link flag indicates whether the remote thinks it's connected to us. if this flag is set but we don't know about the remote,
    // it is likely we just killed the connection (comm error), but in any case we can't handle this packet anymore.
    if (header.flag_link) return;

    // for this unconnected (internal) packet to make sense, it must have data (a command for us), and must not be a split packet.
    // if this assert trips, the packet is malformed. in a debug env, this should not happen, but in the wild it's possible of course
    if (!header.flag_data) {
        assert(false);
        return;
    }

    OnUnconnectedMessage(addr, instream);
}

void Peer::OnMessageReceipt(PacketID id, bool acked) {
    // include the packet number in the payload
    BinaryStream outstream(sizeof(PacketID));
    outstream.WriteInt32(id);

    PacketCommand cmd = acked ? PacketCommand::NOTIFY_RECEIPT_ACKED : PacketCommand::NOTIFY_RECEIPT_LOST;
    Packet notification(cmd, std::move(outstream));
    notification.SetSender(id);
    SendLoopback(notification);
}

void Peer::SetupRemotePeerCallbacks(RemotePeer* remote) {
    using namespace std::placeholders;

    remote->handshake->SetReplyHandler(std::bind(&Peer::SendHandshakePart, this, remote, _1));
    remote->handshake->SetCompletionHandler(std::bind(&Peer::SendHandshakeCompleteNotification, this, remote, _1));
}

void Peer::SendHandshakePart(RemotePeer* remote, BinaryStream&& outstream) {
    assert(remote->crypto);

    // check crypto status HERE, not in PacketQueue/DatagramBuilder, because for the S->C key exchange,
    // crypto will be enabled right after this function returns, but that kx packet should not be encrypted yet
    RemotePeer* forceCryptoBy = remote->crypto->GetCryptoEstablished() ? remote : nullptr;

    Packet packet(PacketCommand::CONNECT_ATTEMPT, std::move(outstream));
    m_queue->EnqueueOutOfBand(packet, remote->addr, forceCryptoBy);
}

void Peer::SendHandshakeCompleteNotification(RemotePeer* remote, Packet&& notification) {
    assert(remote);
    const auto result = remote->handshake->GetResult();
    const bool isRemote = remote->handshake->GetOrigin() == ConnectionOrigin::REMOTE;
    const bool isFailure = result != ConnectResult::OK;

    // avoid reporting failed connections if we didn't initiate them; the user doesn't need to know about them
    if (!isRemote || !isFailure) {
        // notify the client application of the completed connection
        notification.SetSender(remote->id);
        SendLoopback(notification);
    }

    if (isFailure)
        // an error occurred, this connection must be dropped
        remote->Reset();
}

RemotePeer* Peer::GetRemoteByID(PeerID id) {
    // first try logarithmic search in the quick lookup map
    auto it = m_remoteLookup.find(id);
    if (it != m_remoteLookup.end()) return it->second;

    // linear search through peers array
    for (size_t i = 1 /* skip oob socket */; i < m_remotesMax; i++) {
        auto& remote = m_remotes[i];
        if (remote.id == id && remote.active) {
            // insert into map for future fast lookup
            m_remoteLookup.emplace(id, &remote);
            return &remote;
        }
    }

    return nullptr;
}

RemotePeer* Peer::GetRemoteByID(PeerID id) const {
    // first try logarithmic search in the quick lookup map
    auto it = m_remoteLookup.find(id);
    if (it != m_remoteLookup.end()) return it->second;

    // linear search through peers array
    for (size_t i = 1 /* skip oob socket */; i < m_remotesMax; i++) {
        auto& remote = m_remotes[i];
        if (remote.id == id && remote.active)
            return &remote;
    }

    return nullptr;
}

RemotePeer& Peer::GetRemoteByIndex(size_t index) const {
    assert(index < m_remotesMax);
    return m_remotes[index];
}

RemotePeer* Peer::GetRemoteByAddress(const RemoteAddress& addr) const {
    for (size_t i = 1 /* skip oob socket */; i < m_remotesMax; i++)
        if (m_remotes[i].active && m_remotes[i].addr == addr)
            return &m_remotes[i];

    return nullptr;
}

size_t Peer::GetMaximumPeers() const {
    return m_remotesMax - 1;
}

size_t Peer::GetMaximumIncomingPeers() const {
    return m_remotesIncoming;
}

void Peer::SetMaximumIncomingPeers(size_t incoming) {
    // clamp to [0, m_remotesMax), as slot 0 is reserved for oob
    m_remotesIncoming = std::min(std::max(incoming, size_t(0)), GetMaximumPeers());
}

PeerID Peer::GetMyPeerID() const {
    return m_id;
}

void Peer::SetNetworkSimulation(float packetLoss, unsigned additionalPing) {
#if WIREFOX_ENABLE_NETWORK_SIM
    m_simLossRate = packetLoss;
    m_simExtraPing = additionalPing;
#else
    (void)packetLoss;
    (void)additionalPing;
#endif
}

void Peer::RpcRegisterAsync(const std::string& identifier, RpcCallbackAsync_t handler) {
    m_rpc.SlotAsync(identifier, handler);
}

void Peer::RpcRegisterBlocking(const std::string& identifier, RpcCallbackBlocking_t handler) {
    m_rpc.SlotBlocking(identifier, handler);
}

void Peer::RpcUnregisterSlot(const std::string& identifier) {
    m_rpc.RemoveSlot(identifier);
}

void Peer::RpcSignal(const std::string& identifier, PeerID recipient, const BinaryStream& params) {
    // merge identifier and params into a buffer
    BinaryStream data(params.GetLength() + identifier.length() + 2 * sizeof(size_t));
    data.WriteString(identifier);
    data.WriteBool(false); // blocking flag
    data.Write7BitEncodedInt(static_cast<int>(params.GetLength()));
    data.WriteBytes(params);

    // enqueue notification to remote peer
    Packet rpc(PacketCommand::RPC_SIGNAL, std::move(data));
    Send(rpc, recipient, PacketOptions::RELIABLE, PacketPriority::MEDIUM, Channel());
}

bool Peer::RpcSignalBlocking(const std::string& identifier, PeerID recipient, BinaryStream& response, const BinaryStream& params) {
    // merge identifier and params into a buffer
    BinaryStream data(params.GetLength() + identifier.length() + 2 * sizeof(size_t));
    data.WriteString(identifier);
    data.WriteBool(true); // blocking flag
    data.Write7BitEncodedInt(static_cast<int>(params.GetLength()));
    data.WriteBytes(params);

    // enqueue notification to remote peer
    Packet rpc(PacketCommand::RPC_SIGNAL, std::move(data));
    Send(rpc, recipient, PacketOptions::RELIABLE, PacketPriority::CRITICAL, Channel());

    // wait for response
    return m_rpc.AwaitBlockingReply(identifier, recipient, response);
}

#if WIREFOX_ENABLE_NETWORK_SIM
bool Peer::PollArtificialPacketLoss() {
    return
        m_simLossRate > 0 && // skip expensive RNG if rate is zero anyway
        static_cast<float>(m_simrng()) / static_cast<float>(std::mt19937_64::max()) < m_simLossRate;
}
#endif

PeerID Peer::GeneratePeerID() {
    static_assert(std::is_same<std::mt19937_64::result_type, PeerID>::value, "RNG is expected to return 64-bit number");
    std::mt19937_64 rng(static_cast<uint64_t>(Time::Now()));

    PeerID ret;
    do {
        ret = rng();
    } while (ret == 0);

    return ret;
}

RemotePeer* Peer::GetNextAvailableConnectSlot() const {
    // get first slot that is not reserved
    for (size_t i = 0; i < m_remotesMax; i++) {
        auto& slot = m_remotes[i];
        if (!slot.reserved)
            return &slot;
    }

    return nullptr;
}

RemotePeer* Peer::GetNextAvailableIncomingSlot() const {
    if (!GetMaximumIncomingPeers()) return nullptr;

    // linear search from the top down - as opposed to normal GetNextAvailable, which searches from bottom up.
    // this way we don't have to do any additional counting/searching to keep apart inbound/outbound remotes.
    auto minIndex = m_remotesMax - GetMaximumIncomingPeers();
    for (size_t i = m_remotesMax - 1; i >= minIndex; i--) {
        auto& slot = m_remotes[i];
        if (!slot.reserved)
            return &slot;
    }

    return nullptr;
}
