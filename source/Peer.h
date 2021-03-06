/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#pragma once
#include "PeerAbstract.h"
#include "PacketQueue.h"
#include "EncryptionLayer.h"
#include "RpcController.h"

namespace wirefox {

    class BinaryStream;
    class Packet;

    namespace detail {

        struct PacketHeader;
        struct RemotePeer;

        /**
         * \cond WIREFOX_INTERNAL
         * \brief Represents a concrete implementation of an IPeer interface.
         */
        class Peer final : public IPeer {
        public:
            /**
             * \brief Default constructor.
             * \param[in]   maxPeers    Specifies the maximum number of remotes this Peer can be connected to.
             */
            Peer(size_t maxPeers = 1);
            /// Copy constructor.
            Peer(const Peer&) = delete;
            /// Move constructor.
            Peer(Peer&&) noexcept;
            /// Destructor.
            ~Peer();

            /// Copy assignment operator.
            Peer& operator=(const Peer&) = delete;
            /// Move assignment operator.
            Peer& operator=(Peer&&) noexcept;

            ConnectAttemptResult        Connect(const std::string& host, uint16_t port, const uint8_t* public_key) override;
            bool                        Bind(SocketProtocol family, uint16_t port) override;
            void                        Disconnect(PeerID who, Timespan linger) override;
            void                        DisconnectImmediate(PeerID who) override;
            void                        Stop(Timespan linger) override;

            /**
             * \brief Immediately disconnects a remote peer, as if a communication error had occurred.
             *
             * The remote peer will receive no notice that the connection has been terminated. If they keep sending
             * data, it will be ignored by this local peer, unless a new connection request is sent.
             *
             * \param[in]   remote      The RemotePeer that must be kicked and cleaned up.
             */
            void                        DisconnectImmediate(RemotePeer* remote);

            PacketID                    Send(const Packet& packet, PeerID recipient, PacketOptions options, PacketPriority priority, const Channel& channel) override;
            void                        SendLoopback(const Packet& packet) override;
            std::unique_ptr<Packet>     Receive() override;

            void                        SetOfflineAdvertisement(const BinaryStream& data) override;
            void                        DisableOfflineAdvertisement() override;
            void                        Ping(const std::string& hostname, uint16_t port) const override;
            void                        PingLocalNetwork(uint16_t port) const override;

            void                        SetEncryptionEnabled(bool enabled) override;
            void                        SetEncryptionIdentity(const uint8_t* key_secret, const uint8_t* key_public) override;
            void                        GenerateIdentity(uint8_t* key_secret, uint8_t* key_public) const override;
            size_t                      GetEncryptionKeyLength() const override;
            bool                        GetEncryptionEnabled() const override;

            /**
             * \brief Returns the cryptographic identity of this Peer.
             */
            std::shared_ptr<EncryptionLayer::Keypair> GetEncryptionIdentity() const;

            /**
             * \brief Sends an unconnected packet to an arbitrary remote endpoint.
             * 
             * \param[in]   packet      The Packet to queue to the inbox. The data will be \b copied.
             * \param[in]   addr        The socket address of the recipient.
             */
            void                        SendOutOfBand(const Packet& packet, const RemoteAddress& addr);

            size_t                      GetMaximumPeers() const override;
            size_t                      GetMaximumIncomingPeers() const override;
            void                        SetMaximumIncomingPeers(size_t incoming) override;
            PeerID                      GetMyPeerID() const override;

            Channel                     MakeChannel(ChannelMode mode) override;
            ChannelMode                 GetChannelModeByIndex(ChannelIndex index) const override;
            void                        GetAllConnectedPeers(std::vector<PeerID>& output) const override;
            bool                        GetPingAvailable(PeerID who) const override;
            unsigned                    GetPing(PeerID who) const override;
            const PeerStats*            GetStats(PeerID who) const override;

            /**
             * \brief Retrieves a RemotePeer using its slot index.
             * \param[in]   index       The slot index. Range [0, maxPeers].
             */
            RemotePeer&                 GetRemoteByIndex(size_t index) const;

            /**
             * \brief Retrieves the RemotePeer that represents the specified PeerID.
             * 
             * \param[in]   id          The PeerID of the remote you want to look up.
             * \returns     RemotePeer pointer if found, nullptr if not found.
             */
            RemotePeer*                 GetRemoteByID(PeerID id);

            /**
             * \brief Retrieves the RemotePeer that represents the specified PeerID. Const version.
             * \copydoc GetRemoteByID()
             */
            RemotePeer*                 GetRemoteByID(PeerID id) const;

            /**
             * \brief Retrieves the RemotePeer that is associated with a RemoteAddress.
             * 
             * If possible, it is preferable to use GetRemoteByID(), for faster lookup.
             * 
             * \param[in]   addr        The RemoteAddress of the remote you want to look up.
             * \returns     RemotePeer pointer if found, nullptr if not found.
             */
            RemotePeer*                 GetRemoteByAddress(const RemoteAddress& addr) const;

            void                        SetNetworkSimulation(float packetLoss, unsigned additionalPing) override;


            void                        RpcRegisterSlot(const std::string& identifier, RpcCallbackAsync_t handler) override;

            void                        RpcUnregisterSlot(const std::string& identifier) override;

            void                        RpcSignal(const std::string& identifier, PeerID recipient, const BinaryStream& params) override;

#if WIREFOX_ENABLE_NETWORK_SIM
            /**
             * \brief Rolls a random chance indicating whether or not a packet should be artifically dropped.
             * 
             * \returns True if packet should be discarded, false if not.
             */
            bool                        PollArtificialPacketLoss();
#endif

            /**
             * \brief Informs the Peer that a new, unverified sender is requesting connection.
             * 
             * This function will assign a RemotePeer slot for this unverified sender, and begin a handshake.
             * 
             * \param[in]   addr        The RemoteAddress of the new, unverified sender.
             * \param[in]   packet      The message that they sent to us. Should be a connection request.
             */
            void                        OnNewIncomingPeer(const RemoteAddress& addr, const Packet& packet);

            /**
             * \brief Informs the Peer that a remote has disconnected somehow.
             * 
             * \param[in]   remote      The remote peer that disconnected from us.
             * \param[in]   cmd         The PacketCommand of the notification that we will post for the user.
             */
            void                        OnDisconnect(RemotePeer& remote, PacketCommand cmd);

            /**
             * \brief Handles a system notification, e.g. a message that isn't meant for the end user.
             *
             * \param[in]   remote      The remote peer that sent us a packet.
             * \param[in]   packet      The message they sent to us.
             */
            void                        OnSystemPacket(RemotePeer& remote, std::unique_ptr<Packet> packet);

            /**
             * \brief Handles an unconnected message (i.e. LNK flag unset): deals with the contained Packet.
             *
             * \param[in]   addr        The RemoteAddress of the message's sender.
             * \param[in]   instream    An input stream that should start at a PacketHeader.
             */
            void                        OnUnconnectedMessage(const RemoteAddress& addr, BinaryStream& instream);

            /**
             * \brief Handles an unconnected message (i.e. LNK flag unset): verifies the DatagramHeader.
             *
             * \param[in]   addr        The RemoteAddress of the message's sender.
             * \param[in]   msg         A pointer to the raw buffer that contains this datagram.
             * \param[in]   msglen      The length of the buffer pointed to by \p msg.
             */
            void                        OnUnconnectedMessage(const RemoteAddress& addr, const uint8_t* msg, size_t msglen);

            /**
             * \brief Requests that the Peer posts a message receipt.
             *
             * \param[in]   id          The PacketID for which a receipt was requested by the user.
             * \param[in]   acked       If true, the message was delivered successfully, if false, then not.
             */
            void                        OnMessageReceipt(PacketID id, bool acked);

        private:
            void                        SetupRemotePeerCallbacks(RemotePeer* remote);
            void                        SendHandshakePart(RemotePeer* remote, BinaryStream&& outstream);
            void                        SendHandshakeCompleteNotification(RemotePeer* remote, Packet&& notification);

            static PeerID               GeneratePeerID();
            RemotePeer*                 GetNextAvailableConnectSlot() const;
            RemotePeer*                 GetNextAvailableIncomingSlot() const;

            PeerID m_id;
            size_t m_remotesMax;
            size_t m_remotesIncoming;
            BinaryStream m_advertisement;
            RpcController m_rpc;

#if WIREFOX_ENABLE_NETWORK_SIM
            float                       m_simLossRate {0};
            unsigned                    m_simExtraPing {0};
            std::mt19937_64             m_simrng {static_cast<std::mt19937_64::result_type>(Time::Now())};
            std::vector<std::pair<std::unique_ptr<Packet>, Timestamp>>
                                        m_simqueue;
#endif

            std::shared_ptr<Socket>         m_masterSocket;
            std::unique_ptr<RemotePeer[]>   m_remotes;
            std::shared_ptr<PacketQueue>    m_queue;
            std::map<PeerID, RemotePeer*>   m_remoteLookup;
            std::vector<ChannelMode>        m_channels;

            std::shared_ptr<EncryptionLayer::Keypair> m_crypto_identity;
            bool m_crypto_enabled;
        };

        /// \endcond

    }

}
