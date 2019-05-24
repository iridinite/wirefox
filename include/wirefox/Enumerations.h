/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 * 
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#pragma once

/**
 * \file Enumerations.h
 * \brief Contains common enums used throughout the project.
 */

/**
 * \brief Namespace that encapsulates the entire Wirefox API.
 */
namespace wirefox {

    /// Describes the result of starting a connection attempt using Socket::Connect().
    enum class ConnectAttemptResult {
        /// The connection attempt was initiated successfully.
        /// \note This does not imply that the connection was actually opened, only that the \a attempt was started.
        OK,
        /// Invalid settings were specified. Hostname must be non-empty, port must be non-zero, and explicit public key requires enabling crypto.
        INVALID_PARAMETER,
        /// The resolver could not resolve the host name into an endpoint.
        INVALID_HOSTNAME,
        /// The socket was not ready to begin a connection. Make sure it is successfully bound to a port.
        INVALID_STATE,
        /// You are already trying to connect to this endpoint. Wait for a NOTIFY_CONNECT_* notification.
        ALREADY_CONNECTING,
        /// You are already connected to this endpoint.
        ALREADY_CONNECTED,
        /// The local peer has no free slots left to connect with. You need to allocate more slots when creating the Peer, or disconnect someone.
        NO_FREE_SLOTS
    };

    /// Describes the final result of a connection attempt.
    enum class ConnectResult {
        /// \internal [Internal use only.] Connect attempt is not yet complete, try again later.
        IN_PROGRESS,
        /// The connection was opened successfully.
        OK,
        /// The remote endpoint could not be contacted.
        CONNECT_FAILED,
        /// The remote endpoint is not running Wirefox. (Note: this situation may also result in a CONNECT_FAILED error.)
        INCOMPATIBLE_PROTOCOL,
        /// The remote endpoint is not running the same version of Wirefox.
        INCOMPATIBLE_VERSION,
        /// The remote endpoint has different security settings than we do, or an error occurred during encryption or decryption.
        INCOMPATIBLE_SECURITY,
        /// The identity of the remote endpoint could not be verified. Likely causes are a configuration error, or an active MITM-attack.
        INCORRECT_REMOTE_IDENTITY,
        /// The remote endpoint rejected the password.
        INCORRECT_PASSWORD,
        /// The remote endpoint has no free slots left to connect with us (or does not accept any connections at all). Try again later.
        NO_FREE_SLOTS,
        /// The remote endpoint is already connected with us (or with someone else with the same PeerID).
        ALREADY_CONNECTED,
        /// The remote endpoint rejected the connection because our IP address is sending too many requests. Try again later.
        IP_RATE_LIMITED,
        /// The remote endpoint rejected the connection because we are banned.
        BANNED
    };

    /// Indicates which Internet Protocol version is to be used.
    enum class SocketProtocol {
        /// IP version 4: 32 bits address.
        IPv4,
        /// IP version 6: 128 bits address.
        IPv6
    };

    /// Indicates which party initiated a handshake (or connection).
    enum class ConnectionOrigin {
        INVALID,    ///< Invalid setting. Used by Remote #0 to indicate no handshake.
        SELF,       ///< We initiated the handshake.
        REMOTE      ///< A remote party initiated the handshake.
    };

    /// Indicates how packets in the same channel should be delivered relative to each other.
    enum class ChannelMode {
        /// Packets are delivered as they arrived.
        UNORDERED,
        /// Packets are delivered in order. Packets will be withheld indefinitely until missing packets arrive.
        ORDERED,
        /// Packets are delivered in order. Any packets that arrive out of order are discarded.
        SEQUENCED
    };

    /// Indicates additional settings for how a packet should be sent.
    enum class PacketOptions : uint8_t {
        /// The packet is unreliable, meaning there are no guarantees about delivery or order.
        UNRELIABLE      = 0,
        /// The packet is reliable, meaning it will be automatically resent if considered lost in transit.
        RELIABLE        = 1 << 0,
        /// Indicates that the local peer should be notified when the packet is acked, or deemed lost.
        WITH_RECEIPT    = 1 << 1
    };

    /// Indicates the relative priority of a packet.
    enum class PacketPriority : uint8_t {
        /// Lowered priority.
        LOW,
        /// Average priority.
        MEDIUM,
        /// Elevated priority.
        HIGH,
        /// Critically elevated priority.
        CRITICAL
    };

    /// Describes the function and/or meaning of a Packet.
    enum class PacketCommand : uint8_t {
        // ----- INTERNAL - These will never be returned to you as end-user ----- //

        /// \internal Ping request.
        PING,
        /// \internal Ping reply.
        PONG,
        /// \internal Initiates a connection request, or a part of a handshake sequence.
        CONNECT_ATTEMPT,
        /// \internal Requests that the connection be closed.
        DISCONNECT_REQUEST,
        /// \internal Confirms that the connection is now closed.
        DISCONNECT_ACK,
        /// \internal Incoming system ad.
        ADVERTISEMENT,

        // ----- USER - These may be returned through Peer::Receive() ----- //

        /// Indicates that a connection attempt was successful.
        ///     No payload.  Sender is newly connected peer.
        NOTIFY_CONNECT_SUCCESS,
        /// Indicates that a connection attempt has failed.
        ///     (1 byte) ConnectResult, detailed failure reason.
        NOTIFY_CONNECT_FAILED,
        /// Indicates that a remote peer has successfully connected to us.
        ///     No payload.  Sender is newly connected peer.
        NOTIFY_CONNECTION_INCOMING,
        /// Indicates that a remote peer has disconnected because they are no longer responding.
        ///     No payload.  Sender is the disconnected peer.
        NOTIFY_CONNECTION_LOST,
        /// Indicates that a remote peer has gracefully disconnected.
        ///     No payload.  Sender is the disconnected peer.
        NOTIFY_DISCONNECTED,

        /// Receive-receipt indicating successful delivery.
        ///     (4 bytes) PacketID, the ID for which a receipt was requested with PacketOptions::WITH_RECEIPT.
        NOTIFY_RECEIPT_ACKED,
        /// Receive-receipt indicating delivery failure.
        ///     (4 bytes) PacketID, the ID for which a receipt was requested with PacketOptions::WITH_RECEIPT.
        ///     Note that delivery failure for reliable packets will cause the connection to be closed.
        NOTIFY_RECEIPT_LOST,

        /// Incoming system advert, sent in response to a PingLocalNetwork() call.
        ///     Payload begins with a string describing the sender's identity. Parse using BinaryStream::ReadString().
        ///     The rest of the payload is user-defined (the contents of the advertisement).
        NOTIFY_ADVERTISEMENT,

        /// \internal Placeholder for the first user packet.
        USER_PACKET
    };

    /// Specifies the first number that you, as user, should use for defining your own PacketCommands.
    constexpr static int USER_PACKET_START = static_cast<int>(PacketCommand::USER_PACKET);



    /// \cond NO_DOC

    /// Conditionally enables bitmasking operators on strongly typed enums.
    template<typename Enum>
    struct enable_bitmask : std::false_type {};

    /// Enables bitmasking operators for PacketOptions.
    template<>
    struct enable_bitmask<::wirefox::PacketOptions> : std::true_type {};

    /// Returns the bitwise OR of two values of the same strongly typed enum.
    template<typename Enum, typename Ret = typename std::enable_if<enable_bitmask<Enum>::value, Enum>::type>
    constexpr Ret operator|(Enum lhs, Enum rhs) {
        using underlying_t = typename std::underlying_type<Enum>::type;
        return static_cast<Enum>(static_cast<underlying_t>(lhs) | static_cast<underlying_t>(rhs));
    }

    /// Returns a boolean indicating whether the bitfield lhs contains the bitmask rhs.
    template<typename Enum, typename Enable = typename std::enable_if<enable_bitmask<Enum>::value, Enum>::type>
    constexpr bool operator&(Enum lhs, Enum rhs) {
        using underlying_t = typename std::underlying_type<Enum>::type;
        return static_cast<underlying_t>(static_cast<underlying_t>(lhs) & static_cast<underlying_t>(rhs)) == static_cast<underlying_t>(rhs);
    }

    /// \endcond

}
