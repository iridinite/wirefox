/**
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#pragma once
#include "Enumerations.h"
#include "WirefoxTime.h"
#include "Channel.h"

namespace wirefox {

    class BinaryStream;
    class Packet;

    /**
     * \brief Represents a network peer.
     * 
     * The IPeer interface is your primary network interface. This object maintains connections at a high level and allows
     * exchange of data with remote Peers.
     * 
     * \par Example
     * \code
     * // Create a new Peer and begin connecting to a remote host
     * auto peer = IPeer::Factory::Create();
     * peer->Bind(SocketProtocol::IPv6, 0);
     * peer->Connect( ... );
     * \endcode
     */
    class WIREFOX_API IPeer {
    public:
        // Essentially an explicit instantiation of detail::Factory<T>; we need to create a Peer*, not an IPeer*.
        // However, I don't want to include Peer.h here because the client app shouldn't include most impl details (or ASIO).
        // If someone knows proper syntax for explicit template instantiation, let me know! It was wonky when I tried it...

        /// Represents a factory object that produces new instances of IPeer.
        struct WIREFOX_API Factory {
            Factory() = delete;
            /// Creates a new instance of the IPeer interface.
            static std::unique_ptr<IPeer> Create(size_t maxPeers = 1);
        };

    protected:
        /// [Internal use only.] Default constructor.
        IPeer() = default;

    public:
        /// [Internal use only.] Default destructor.
        virtual ~IPeer() = default;

        /// Copy constructor. Disabled because it is nonsensical to duplicate a Peer.
        IPeer(const IPeer&) = delete;
        /// Move constructor.
        IPeer(IPeer&&) noexcept = default;
        /// Copy assignment operator. Disabled because it is nonsensical to duplicate a Peer.
        IPeer& operator=(const IPeer&) = delete;
        /// Move assignment operator.
        IPeer& operator=(IPeer&&) noexcept = default;

        /**
         * \brief Try to connect to a remote host.
         * \returns A value indicating whether the connection attempt was successfully launched.
         * 
         * \param[in]   host    A hostname, domain name, or IP address, you wish to connect to.
         * \param[in]   port    The port on which to connect. The remote host must be listening on this port.
         */
        virtual ConnectAttemptResult    Connect(const std::string& host, uint16_t port) = 0;

        /**
         * \brief Bind this peer to a local network interface.
         * 
         * This function must be called before initiating or accepting new connections. The Peer cannot receive
         * data from the network unless the peer is bound. After this function is called, you can only use the
         * protocol family specified in \p family for any incoming or outgoing connections.
         * 
         * \returns True on success; false if the specified port is already in use, or if you have insufficient
         * system privileges to bind, or if the system is otherwise being difficult.
         * 
         * \param[in]   family  Indicates the socket family to use for this Peer (either IPv4 or IPv6).
         * \param[in]   port    Specifies the local port to bind to. Specify 0 to have the system assign a random port.
         */
        virtual bool                    Bind(SocketProtocol family, uint16_t port) = 0;

        /**
         * \brief Stops all services of this peer.
         * 
         * \todo Implementation details TBD.
         */
        virtual void                    Stop(Timespan linger = Time::FromMilliseconds(0)) = 0;

        /**
         * \brief Gracefully disconnects a remote peer.
         * 
         * The remote peer will be notified of the disconnection; they will receive a PacketCommand::NOTIFY_DISCONNECTED
         * notification. After this function returns, the connection may linger while the disconnect completes.
         * 
         * \param[in]   who         The ID number of the peer you wish to kick.
         * \param[in]   linger      The maximum amount of time to wait for confirmation from the remote peer.
         */
        virtual void                    Disconnect(PeerID who, Timespan linger = Time::FromMilliseconds(200)) = 0;

        /**
         * \brief Immediately disconnects a remote peer, as if a communication error had occurred.
         * 
         * The remote peer will receive no notice that the connection has been terminated. If they keep sending
         * data, it will be ignored by this local peer, unless a new connection request is sent.
         * 
         * \note In the current implementation, you will receive a PacketCommand::NOTIFY_CONNECTION_LOST
         * notification regarding the remote peer you disconnect here. This may change in the future.
         * 
         * \param[in]   who         The ID number of the peer you wish to kick.
         */
        virtual void                    DisconnectImmediate(PeerID who) = 0;

        /**
         * \brief Send data to a remote host.
         * 
         * \returns The ID number of the packet that was queued as a result of this call. You need this ID if you
         * requested a receipt (see PacketOptions::WITH_RECEIPT), so you can identify this packet again later.
         * 
         * This function fails silently if \p recipient is unknown or if \p packet exceeds the length limit
         * (cfg::PACKET_MAX_LENGTH). In either case, no send is queued.
         * 
         * \param[in]   packet      The data you'd like to send. The data will be \b copied to an internal buffer.
         * \param[in]   recipient   The remote peer who this data is addressed to.
         * \param[in]   options     A bitfield with reliability settings.
         * \param[in]   priority    Optional. Custom priority setting. Meaning is relative to other packets.
         * \param[in]   channel     Optional. Ordered and sequenced packets only wait for packets in the same channel.
         * 
         * \todo Priority is not implemented. Trying to come up with a good algorithm in DatagramBuilder.
         */
        virtual PacketID                Send(const Packet& packet, PeerID recipient, PacketOptions options,
                                             PacketPriority priority = PacketPriority::MEDIUM,
                                             const Channel& channel = Channel()) = 0;

        /**
         * \brief Add a Packet onto the incoming queue.
         * 
         * This is primarily useful for posting notifications to the local peer, or for requeueing a packet you
         * don't want to handle just yet.
         * 
         * \param[in]   packet      The Packet to queue to the inbox. The data will be \b copied.
         */
        virtual void                    SendLoopback(const Packet& packet) = 0;

        /**
         * \brief Retrieves the next incoming Packet from the inbox.
         * 
         * This function will return the next incoming Packet in the queue. It is advised to call this function
         * once per frame, in a loop, and stop the loop only when Receive() returns nullptr.
         * 
         * \returns The next available incoming Packet, as an owning pointer, or nullptr.
         */
        virtual std::unique_ptr<Packet> Receive() = 0;

        /**
         * \brief Enables system advertisements, and sets the specified data as the response.
         * 
         * Use this function, together with PingLocalNetwork(), to perform discovery of peers (game lobbies, etc)
         * on the local network. If this system receives a ping from a remote Wirefox peer, it will respond with
         * the data you specify here (the 'advertisement').
         * 
         * \param[in]   data        The payload to include with offline ping responses. The data is copied.
         * \sa DisableOfflineAdvertisement(), PingLocalNetwork()
         */
        virtual void                    SetOfflineAdvertisement(const BinaryStream& data) = 0;

        /**
         * \brief Disables responding to pings, and discards the previously set advertisement payload.
         * 
         * If you no longer want this Peer to respond to unconnected pings from remote systems, use this method.
         * Does nothing if SetOfflineAdvertisement() was never called before.
         */
        virtual void                    DisableOfflineAdvertisement() = 0;

        /**
         * \brief Sends an offline ping to a specific remote endpoint.
         * 
         * The remote endpoint, if reachable, and if advertising enabled, will respond with a pong and the data
         * that was set with SetOfflineAdvertisement().
         * 
         * \param[in]   hostname    The hostname or IP address to ping.
         * \param[in]   port        The port on which the remote endpoint is expected to listen.
         */
        virtual void                    Ping(const std::string& hostname, uint16_t port) const = 0;

        /**
         * \brief Sends an offline ping to the local network.
         *
         * Any remote peers on the same network that have advertising enabled, will respond with a pong and the
         * data that was set with SetOfflineAdvertisement().
         * 
         * \note This feature relies on the broadcasting capability of the connected router(s). These may be
         * disabled for whatever reason on your network (e.g. by a network administrator).
         * 
         * \param[in]   port        The port on which the remote endpoints are expected to listen.
         */
        virtual void                    PingLocalNetwork(uint16_t port) const = 0;

        /**
         * \brief Registers and returns a new Channel for your packets.
         * 
         * Use Channels to enhance sequenced and ordered packet streams. Packets will only be ordered relative to
         * packets in the same channel, so you could use this to, for example, separate chat and player movement.
         * 
         * \note If too many channels are used, this function may return a default, unordered channel.
         * 
         * \todo Maybe let the user manually specify ChannelIndex? This API feels a bit weird, all peers have to
         * make all the same channels in the exact same order for this to work.
         */
        virtual Channel                 MakeChannel(ChannelMode mode) = 0;

        /**
         * \brief Returns the ChannelMode that was set by MakeChannel().
         * 
         * \throws std::out_of_range if no channel was registered with the specified index.
         * \param[in]   index       The ID number of the channel.
         */
        virtual ChannelMode             GetChannelModeByIndex(ChannelIndex index) const = 0;

        /**
         * \brief Makes a list of all remote peers connected to this Peer.
         * 
         * \param[out]  output      A container that will be filled in with PeerIDs of remote peers.
         */
        virtual void                    GetAllConnectedPeers(std::vector<PeerID>& output) const = 0;

        /**
         * \brief Returns a value indicating whether a ping value is currently available.
         * 
         * For newly connected peers, likely not enough data is available to reliably calculate ping. In such
         * a case, this function will return false, and GetPing() may return an incorrect value.
         * 
         * \param[in]   who         The PeerID for whom to check ping.
         */
        virtual bool                    GetPingAvailable(PeerID who) const = 0;

        /**
         * \brief Returns the ping of a remote peer, in milliseconds.
         * 
         * Ping is the estimated amount of time it takes for a message to travel to the remote peer \p who
         * and back to the local peer. Generally, the lower, the better.
         * 
         * \param[in]   who         The PeerID for whom to check ping.
         */
        virtual unsigned                GetPing(PeerID who) const = 0;

        /**
         * \brief Returns the maximum number of peers this IPeer can be connected to at once.
         * 
         * This is the same value that was passed to \p maxPeers in Factory::Create.
         */
        virtual size_t                  GetMaximumPeers() const = 0;

        /**
         * \brief Returns the maximum number of peers this IPeer can have incoming connections with at once. 
         *
         * A connection is considered 'incoming' if the remote endpoint is the one who initiated it.
         */
        virtual size_t                  GetMaximumIncomingPeers() const = 0;

        /**
         * \brief Sets the maximum number of incoming connections.
         * 
         * If more remote Peers try to connect than this number, they will be rejected with a
         * ConnectResult::NO_FREE_SLOTS error.
         */
        virtual void                    SetMaximumIncomingPeers(size_t incoming) = 0;

        /**
         * \brief Returns the local peer's PeerID.
         */
        virtual PeerID                  GetMyPeerID() const = 0;

        /**
         * \brief Applies network simulation settings to this peer.
         * 
         * In a debug environment, it may be desirable to test poor connections. This function allows you to
         * introduce artificial packet loss and additional ping. Settings are applied only to \b incoming packets,
         * so you may want to call this on both ends of the connection.
         * 
         * \param[in]   packetLoss      Artificial packet loss, as percentage. 0.0 is no extra loss, 1.0 is 100% loss rate.
         * \param[in]   additionalPing  Artificial delay, in milliseconds, applied to incoming packets before they appear in Receive().
         */
        virtual void                    SetNetworkSimulation(float packetLoss, unsigned additionalPing) = 0;
    };

}
