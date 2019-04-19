#include "PCH.h"
#include "Handshaker.h"
#include "Packet.h"

using namespace wirefox::detail;

static_assert(cfg::CONNECT_RETRY_COUNT >= 1, "CONNECT_RETRY_COUNT must be at least 1");
static_assert(cfg::CONNECT_RETRY_DELAY >= 1, "CONNECT_RETRY_DELAY must be at least 1");

Handshaker::Handshaker(PeerID myID, Origin origin)
    : m_myID(myID)
    , m_replyHandler(nullptr)
    , m_origin(origin)
    , m_result(ConnectResult::IN_PROGRESS)
    , m_lastReply(0)
    , m_resendAttempts(0) {}

void Handshaker::SetReplyHandler(ReplyHandler_t handler) {
    m_replyHandler = handler;
}

void Handshaker::SetCompletionHandler(CompletionHandler_t handler) {
    m_completionHandler = handler;
}

void Handshaker::Reply(BinaryStream&& outstream, bool isRetry) {
    assert(m_replyHandler && "Handshaker::Reply has no reply handler");

    m_lastReply = outstream;

    if (!isRetry)
        m_resendAttempts = 1;
    m_resendNext = Time::Now() + Time::FromMilliseconds(cfg::CONNECT_RETRY_DELAY);

    if (m_replyHandler)
        m_replyHandler(std::move(outstream));
}

void Handshaker::Complete(ConnectResult result) {
    m_result = result;

    // free some memory
    m_lastReply.Reset();

    if (m_completionHandler) {
        BinaryStream payload(sizeof(uint8_t));
        payload.WriteByte(static_cast<uint8_t>(result));

        // pick the appropriate command id: if the connection was locally started, then the user expects a success/failure,
        // otherwise if it was remote then the user only expects a 'peer incoming' notification
        PacketCommand notifyCmd;
        if (m_origin == Origin::SELF)
            notifyCmd = result == ConnectResult::OK
                ? PacketCommand::NOTIFY_CONNECT_SUCCESS
                : PacketCommand::NOTIFY_CONNECT_FAILED;
        else
            // NOTE: we still invoke the completion handler even if the connection was remotely initiated AND failed,
            // because Peer needs to reset this RemotePeer instance. See Peer::SendHandshakeCompleteNotification.
            notifyCmd = PacketCommand::NOTIFY_CONNECTION_INCOMING;

        Packet notification(notifyCmd, std::move(payload));
        m_completionHandler(std::move(notification));
    }
}

void Handshaker::Update() {
    if (IsDone() || !Time::Elapsed(m_resendNext)) return;

    // There is one edge case where m_lastReply might be empty: if a new remote was reserved for an incoming
    // handshake part, but that handshake was a stage mismatch, so it was silently ignored and no Reply was recorded.
    // In that case, this function should essentially do nothing but count down until connection timeout.
    //assert(m_lastReply.GetBuffer() != nullptr && "Handshaker::Update was ran before any message was posted");

    if (++m_resendAttempts > cfg::CONNECT_RETRY_COUNT) {
        // all attempts failed, drop this connection
        Complete(ConnectResult::CONNECT_FAILED);
        return;
    }

    if (!m_lastReply.IsEmpty()) {
        // send the same message again as last time
        BinaryStream replyCopy(m_lastReply);
        Reply(std::move(replyCopy), true);
    }
}
