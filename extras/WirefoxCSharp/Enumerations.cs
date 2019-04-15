using JetBrains.Annotations;

namespace Iridinite.Wirefox {

    [PublicAPI]
    public enum ConnectAttemptResult {
        /// The connection attempt was initiated successfully.
        /// \note This does not imply that the connection was actually opened, only that the \a attempt was started.
        OK,
        /// Invalid settings were passed to Socket::Connect(). The host-name must be non-empty, and the port must be non-zero.
        INVALID_PARAMETER,
        /// The resolver could not resolve the host name into an endpoint.
        INVALID_HOSTNAME,
        /// The socket was not ready to begin a connection. For UDP, the socket must be bound to a local port first.
        INVALID_STATE,
        ///
        ALREADY_CONNECTING,
        ///
        ALREADY_CONNECTED,
        /// The local peer has no free slots left to connect with. You need to allocate more slots when creating the Peer, or disconnect someone.
        NO_FREE_SLOTS
    }

    [PublicAPI]
    public enum ConnectResult {
        /// Connect attempt is not yet complete, try again later. [Internal use only.]
        IN_PROGRESS,
        /// The connection was opened successfully.
        OK,
        /// The remote endpoint could not be contacted.
        CONNECT_FAILED,
        /// The remote endpoint is not running Wirefox.
        INCOMPATIBLE_PROTOCOL,
        /// The remote endpoint is not running the same version of Wirefox.
        INCOMPATIBLE_VERSION,
        /// The remote endpoint has different security settings than we do.
        INCOMPATIBLE_SECURITY,
        /// The remote endpoint rejected the password.
        INVALID_PASSWORD,
        /// The remote endpoint has no free slots left to connect with us (or does not accept any connections at all). Try again later.
        NO_FREE_SLOTS,
        /// The remote endpoint is already connected with us (or with someone else with the same PeerID).
        ALREADY_CONNECTED,
        /// The remote endpoint rejected the connection because our IP address is sending too many requests. Try again later.
        IP_RATE_LIMITED,
        /// The remote endpoint rejected the connection because we are banned.
        BANNED
    }

    /// Indicates which Internet Protocol version is to be used.
    [PublicAPI]
    public enum SocketProtocol {
        /// IP version 4: 32 bits address.
        IPv4,
        /// IP version 6: 128 bits address.
        IPv6
    };

    /// Indicates additional settings for how a packet should be sent.
    [PublicAPI]
    public enum PacketOptions {
        /// The packet is unreliable, meaning there are no guarantees about delivery or order.
        UNRELIABLE      = 0,
        /// The packet is reliable, meaning it will be automatically resent if considered lost in transit.
        RELIABLE        = 1 << 0,
    };

    /// Describes the function and/or meaning of a Packet.
    [PublicAPI]
    public enum PacketCommand : byte {
        // ----- INTERNAL - These will never be returned to you as end-user ----- //

        /// Ping request.
        PING,
        /// Ping reply.
        PONG,
        /// Initiates a connection request, or a part of a handshake sequence.
        CONNECT_ATTEMPT,
        DISCONNECT_REQUEST,
        DISCONNECT_ACK,

        // ----- USER - These may be returned through Peer::Receive() ----- //

        /// Indicates that a connection attempt was successful.
        NOTIFY_CONNECT_SUCCESS,
        /// Indicates that a connection attempt has failed.
        ///     (1) result: ConnectResult, detailed failure reason.
        NOTIFY_CONNECT_FAILED,
        /// Indicates that a remote peer has successfully connected to us.
        ///     No payload.
        NOTIFY_CONNECTION_INCOMING,
        NOTIFY_CONNECTION_LOST,
        NOTIFY_DISCONNECTED,

        USER_PACKET
    }

}
