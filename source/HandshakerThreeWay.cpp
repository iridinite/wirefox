#include "PCH.h"
#include "HandshakerThreeWay.h"
#include "RemotePeer.h"

using namespace wirefox::detail;

static constexpr size_t HANDSHAKE_LENGTH =
    sizeof(cfg::WIREFOX_MAGIC) +    // magic number //-V119
    sizeof(uint8_t) +               // protocol version
    sizeof(PeerID) +                // peerID exchange
    sizeof(uint8_t) +               // handshake stage
    sizeof(uint8_t);                // error code, if stage == 2

void HandshakerThreeWay::Begin(const RemotePeer*) {
    // Begin should only be called if this local socket is the one initiating the connection
    assert(GetOrigin() == Origin::SELF);

    // write a connection request and send it
    BinaryStream hello(HANDSHAKE_LENGTH);
    WriteReplyHeader(hello, m_myID);
    hello.WriteByte(0); // connection phase 0

    m_status = AWAITING_ACK;
    Reply(std::move(hello));
}

void HandshakerThreeWay::Handle(RemotePeer* remote, const Packet& packet) {
    if (IsDone()) return;

    // prepare the leading part of the reply
    BinaryStream instream = packet.GetStream();
    BinaryStream reply(HANDSHAKE_LENGTH);
    WriteReplyHeader(reply, m_myID);


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
    // TODO: check if remoteID is already in use
    remote->id = remoteID;
    const uint8_t stage = instream.ReadByte();

    // discard packets that mismatch with our expected stage; they're probably resends or delayed arrivals
    if (stage == 0 && m_status == AWAITING_ACK) return;
    if (stage == 1 && m_status == NOT_STARTED) return;
    if (stage == 2) {
        // stage 2 is an error report & indicates failure
        const auto problem = static_cast<ConnectResult>(instream.ReadByte());
        assert(problem != ConnectResult::OK && problem != ConnectResult::IN_PROGRESS);
        Complete(problem);
        return;
    }

    // for clarity: even though this is p2p networking, below I refer to the 'client' as the one who initiated
    // the connection request, and the 'server' as the one who the client wants to connect to.

    if (m_status == NOT_STARTED) {
        // we're the server, and client just sent first part of handshake
        m_status = AWAITING_ACK;
        reply.WriteByte(1);
        Reply(std::move(reply));

    } else if (/*m_status == AWAITING_ACK &&*/ GetOrigin() == Origin::REMOTE) {
        // we're the server, and client just sent part 3 of the handshake, we're done
        Complete(ConnectResult::OK);

    } else if (/*m_status == AWAITING_ACK &&*/ GetOrigin() == Origin::SELF) {
        // we're the client, and server just replied to our request, so ack that
        reply.WriteByte(1);
        Reply(std::move(reply));

        // TODO: not strictly speaking done yet! kinda need to wait for an ack first. If we now start sending data
        // TODO: (flag_link) and that data arrives *before* the handshake, then the server kills the connection.
        Complete(ConnectResult::OK);
    }
}

void HandshakerThreeWay::WriteOutOfBandErrorReply(BinaryStream& outstream, PeerID myID, ConnectResult reply) {
    WriteReplyHeader(outstream, myID);
    outstream.WriteByte(2); // stage 2 indicates error
    outstream.WriteByte(static_cast<uint8_t>(reply));
}

void HandshakerThreeWay::WriteReplyHeader(BinaryStream& outstream, PeerID myID) {
    outstream.WriteBytes(cfg::WIREFOX_MAGIC, sizeof cfg::WIREFOX_MAGIC);
    outstream.WriteByte(cfg::WIREFOX_PROTOCOL_VERSION);
    outstream.WriteInt64(myID);
}

void HandshakerThreeWay::ReplyWithError(BinaryStream& outstream, ConnectResult problem) {
    outstream.WriteByte(2); // stage 2 indicates error
    outstream.WriteByte(static_cast<uint8_t>(problem));
    Reply(std::move(outstream));
}
