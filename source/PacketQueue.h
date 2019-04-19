/**
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#pragma once
#include "Packet.h"
#include "BinaryStream.h"
#include "RemoteAddress.h"
#include "WirefoxTime.h"
#include "AwaitableEvent.h"

namespace wirefox {

    struct Channel;

    namespace detail {

        struct PacketHeader;
        struct RemoteAddress;
        struct RemotePeer;
        class Peer;
        class Socket;

        /**
         * \cond WIREFOX_INTERNAL
         * \brief Represents a packet and reliability manager.
         *
         * Operates around a Socket and set of RemotePeers. It is responsible for queueing read and write operations for peers,
         * fragmenting outgoing Packets into segments if necessary, and reassembling incoming Packets.
         */
        class PacketQueue : public std::enable_shared_from_this<PacketQueue> {
        public:
            /// Represents an outbound packet that is not yet assigned to a datagram.
            struct OutgoingPacket {
                BinaryStream    blob;       ///< A byte blob that contains both the packet header and payload.
                RemoteAddress   addr;       ///< The remote endpoint this packet is addressed to.
                Timestamp       sendNext;   ///< Indicates when this packet should be treated as lost and resent.
                RemotePeer*     remote;     ///< The peer slot associated with this packet.
                PacketID        id;         ///< The ID number of this datagram, used for resending and acknowledgement.
                PacketOptions   options;    ///< Reliability settings associated with this packet.
                unsigned int    sendCount;  ///< Indicates the number of times this packet has been sent.

                /// Returns a value indicating whether the given PacketOptions are set for this OutgoingPacket.
                bool            HasFlag(PacketOptions test) const;
            };

            /// Represents an outbound datagram that is not yet fully delivered.
            struct OutgoingDatagram {
                DatagramID      id;         ///< The ID number of this datagram.
                RemoteAddress   addr;       ///< The remote endpoint this packet is addressed to.
                BinaryStream    blob;       ///< A byte blob that contains both the datagram header and all packets, if any.
                Timestamp       discard;    ///< The timestamp at which this datagram should be removed / cleaned up.
                std::vector<PacketID> packets;  ///< The list of PacketIDs this datagram contains. Used for acking packets.
            };

            /**
             * \brief Constructs a new PacketQueue, starts a worker thread, and regist specified underlying Socket.
             */
            PacketQueue(Peer* peer);

            /**
             * \brief Destroys this PacketQueue, deallocates queued packets, and stops the worker thread.
             */
            ~PacketQueue();

            /**
             * \brief Send an outgoing message.
             *
             * Adds a packet onto the outgoing queue. The packet and its contents will be copied, so you can safely deallocate
             * your own copy of the packet after this function returns.
             *
             * \param[in]   packet      The packet to send out.
             * \param[in]   remote      Which remote peer to send the packet to.
             * \param[in]   options     Reliability settings for this packet.
             * \param[in]   priority    Custom priority setting. Meaning is relative to other packets.
             * \param[in]   channel     Ordered and sequenced packets only wait for packets in the same channel.
             */
            PacketID        EnqueueOutgoing(const Packet& packet, RemotePeer* remote, PacketOptions options, PacketPriority priority, const Channel& channel);

            /**
             * \brief Send a message to a specific remote endpoint.
             *
             * Adds a packet onto the outgoing queue. The packet and its contents will be copied, so you can safely deallocate
             * your own copy of the packet after this function returns.
             *
             * \param[in]   packet      The packet to send out.
             * \param[in]   addr        The raw remote address to send data to.
             */
            void            EnqueueOutOfBand(const Packet& packet, const RemoteAddress& addr);

            /**
             * \brief Send a message to this local socket.
             *
             * Adds a packet onto the incoming queue. The packet and its contents will be copied, so you can safely deallocate
             * your own copy of the packet after this function returns.
             * This function is primarily meant for sending notifications and callbacks to the local socket, while still having
             * one central place for handling them (the place where you call PacketQueue::DequeueIncoming()).
             *
             * \param[in]   packet  The packet to send to yourself.
             */
            void            EnqueueLoopback(const Packet& packet);

            /**
             * \brief Read an incoming message.
             *
             * Removes the next incoming packet from the inbound queue and returns it. If no packet is currently available,
             * this function returns nullptr. Be sure to keep calling this function in a loop until it returns nullptr, as
             * more packets may be received in the span of a single game frame.
             *
             * \returns     An owning pointer to a Packet instance, or nullptr if the queue is empty.
             */
            std::unique_ptr<Packet> DequeueIncoming();

        private:
            using Inbox = std::queue<std::unique_ptr<Packet>>;

            void            ThreadWorker();

            void            DoReadCycle(RemotePeer& remote);
            void            DoWriteCycle(RemotePeer& remote);

            void            OnWriteFinished(RemotePeer* remote, DatagramID id, bool error, size_t transferred);
            void            OnReadFinished(bool error, const RemoteAddress& sender, const uint8_t* buffer, size_t transferred);

            void            HandleIncomingPacket(RemotePeer& remote, const PacketHeader& header, std::unique_ptr<Packet> packet);

            Peer*               m_peer;
            Inbox               m_inbox;

            cfg::LockableMutex  m_lockInbox;
            std::atomic_bool    m_updateThreadAbort;
            std::thread         m_updateThread;
            AwaitableEvent      m_updateNotify;
        };

        /// \endcond

    }

}
