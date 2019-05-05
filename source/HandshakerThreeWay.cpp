/**
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#include "PCH.h"
#include "RemotePeer.h"
#include "Peer.h"
#include "WirefoxConfigRefs.h"

using namespace wirefox::detail;

static constexpr size_t HANDSHAKE_HEADER_LEN =
    sizeof(cfg::WIREFOX_MAGIC) +    // magic number //-V119
    sizeof(uint8_t) +               // protocol version
    sizeof(PeerID) +                // peerID exchange
    sizeof(uint8_t);                // handshake stage

void HandshakerThreeWay::Begin() {
    // Begin should only be called if this local socket is the one initiating the connection
    assert(GetOrigin() == Origin::SELF);

    // write a connection request and send it
    BinaryStream hello(HANDSHAKE_HEADER_LEN);
    WriteReplyHeader(hello, m_peer->GetMyPeerID());
    hello.WriteByte(INITIAL_CLIENT);
    hello.WriteBool(m_peer->GetEncryptionEnabled());

    m_expectedOpcode = INITIAL_SERVER;
    Reply(std::move(hello));
}

void HandshakerThreeWay::Handle(const Packet& packet) {
    if (IsDone()) return;

    WIREFOX_LOCK_GUARD(m_remote->lock);

    // prepare the leading part of the reply
    BinaryStream instream = packet.GetStream();
    BinaryStream reply(HANDSHAKE_HEADER_LEN);
    WriteReplyHeader(reply, m_peer->GetMyPeerID());

    if (instream.IsEOF(HANDSHAKE_HEADER_LEN)) {
        // handshake header not long enough, don't bother parsing it
        Complete(ConnectResult::INCOMPATIBLE_PROTOCOL);
    }


    // read and compare the magic number, to make sure we're talking to a Wirefox endpoint
    uint8_t magic[sizeof cfg::WIREFOX_MAGIC] = {0};
    instream.ReadBytes(magic, sizeof cfg::WIREFOX_MAGIC);
    if (std::memcmp(cfg::WIREFOX_MAGIC, magic, sizeof cfg::WIREFOX_MAGIC) != 0) {
        // Could call ReplyWithError here, but it's probably useless, the remote endpoint won't understand the message anyway
        Complete(ConnectResult::INCOMPATIBLE_PROTOCOL);
        return;
    }

    // and check the protocol version number
    const uint8_t version = instream.ReadByte();
    if (version != cfg::WIREFOX_PROTOCOL_VERSION) {
        ReplyWithError(reply, ConnectResult::INCOMPATIBLE_VERSION);
        Complete(ConnectResult::INCOMPATIBLE_VERSION);
        return;
    }

    // now that that's out of the way, read some state data
    const PeerID remoteID = instream.ReadUInt64();
    const uint8_t opcode = instream.ReadByte();

    auto* competitor = m_peer->GetRemoteByID(remoteID);
    if (competitor && competitor != m_remote) {
        // there is already a peer with the same ID number
        ReplyWithError(reply, ConnectResult::ALREADY_CONNECTED);
        Complete(ConnectResult::ALREADY_CONNECTED);
        return;
    }

    m_remote->id = remoteID;

    // discard packets that mismatch with our expected stage; they're probably resends or delayed arrivals
    if (opcode == ERROR_OCCURRED) {
        // stage 2 is an error report & indicates failure
        const auto problem = static_cast<ConnectResult>(instream.ReadByte());
        assert(problem != ConnectResult::OK && problem != ConnectResult::IN_PROGRESS);
        Complete(problem);
        return;
    }

    // for clarity: even though this is p2p networking, below I refer to the 'client' as the one who initiated
    // the connection request, and the 'server' as the one who the client wants to connect to.

    if (m_expectedOpcode == NOT_STARTED && opcode == INITIAL_CLIENT) {
        // we're the server, and client just sent first part of handshake
        //assert(opcode == INITIAL_CLIENT);
        assert(GetOrigin() == Origin::REMOTE);
        //if (opcode != INITIAL_CLIENT) return;

        // make sure that both sides agree whether they want security or not
        assert(m_remote->crypto);
        bool remoteWantsCrypto = instream.ReadBool();
        if (remoteWantsCrypto != m_peer->GetEncryptionEnabled()) {
            ReplyWithError(reply, ConnectResult::INCOMPATIBLE_SECURITY);
            Complete(ConnectResult::INCOMPATIBLE_SECURITY);
            return;
        }

        m_expectedOpcode = m_peer->GetEncryptionEnabled() ? AUTH_MSG : UNENCRYPTED_ACK;
        reply.WriteByte(INITIAL_SERVER);
        reply.WriteBool(m_peer->GetEncryptionEnabled());
        Reply(std::move(reply));

    } else if (m_expectedOpcode == UNENCRYPTED_ACK && opcode == m_expectedOpcode) {
        // we're the server, and client just sent part 3 of the handshake
        assert(GetOrigin() == Origin::REMOTE);
        // three-way handshake completed
        Complete(ConnectResult::OK);

    } else if (m_expectedOpcode == INITIAL_SERVER && opcode == m_expectedOpcode) {
        // we're the client, and server just replied to our initial request
        assert(GetOrigin() == Origin::SELF);

        if (m_peer->GetEncryptionEnabled()) {
            // basic handshake is now finished, begin crypto key exchange
            reply.WriteByte(AUTH_MSG);
            m_expectedOpcode = AUTH_MSG;
            m_auth->Begin(reply);
        } else {
            reply.WriteByte(UNENCRYPTED_ACK);
            // TODO: not strictly speaking done yet! kinda need to wait for an ack first. If we now start sending data
            // TODO: (flag_link) and that data arrives *before* the handshake, then the server kills the connection.
            Complete(ConnectResult::OK);
        }

        Reply(std::move(reply));

    } else if (m_expectedOpcode == AUTH_MSG && opcode == m_expectedOpcode) {
        reply.WriteByte(AUTH_MSG);
        auto authresult = m_auth->Handle(instream, reply);

        // send intermediate messages to remote party
        if ((authresult == ConnectResult::IN_PROGRESS) ||
            (authresult == ConnectResult::OK && GetOrigin() == Origin::SELF)) {
            Reply(std::move(reply));
        }
        // mark handshake result
        if (authresult != ConnectResult::IN_PROGRESS) {
            Complete(authresult);
        }
    }
}

void HandshakerThreeWay::WriteOutOfBandErrorReply(BinaryStream& outstream, PeerID myID, ConnectResult reply) {
    WriteReplyHeader(outstream, myID);
    outstream.WriteByte(ERROR_OCCURRED);
    outstream.WriteByte(static_cast<uint8_t>(reply));
}

void HandshakerThreeWay::WriteReplyHeader(BinaryStream& outstream, PeerID myID) {
    outstream.WriteBytes(cfg::WIREFOX_MAGIC, sizeof cfg::WIREFOX_MAGIC);
    outstream.WriteByte(cfg::WIREFOX_PROTOCOL_VERSION);
    outstream.WriteInt64(myID);
}

void HandshakerThreeWay::ReplyWithError(BinaryStream& outstream, ConnectResult problem) {
    outstream.WriteByte(ERROR_OCCURRED); // stage 2 indicates error
    outstream.WriteByte(static_cast<uint8_t>(problem));
    Reply(std::move(outstream));
}
