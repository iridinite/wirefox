#pragma once
#include "Handshaker.h"
#include "RemoteAddress.h"
#include "PacketQueue.h"
#include "CongestionControl.h"
#include "ChannelBuffer.h"
#include "ReceiptTracker.h"

namespace wirefox {

    namespace detail {

        /**
         * \cond WIREFOX_INTERNAL
         * \brief Represents and contains state for a remote network peer.
         */
        struct RemotePeer {
            RemotePeer();
            ~RemotePeer() = default;

            /// Indicates whether this slot is currently in use.
            std::atomic_bool reserved;

            /// Indicates whether this slot should receive socket callbacks, and should have read/write cycles invoked.
            std::atomic_bool active;

            /// Indicates whether an async write operation on the socket is still pending.
            std::atomic_bool pendingWrite;

            /// Indicates when the disconnect grace period ends. If IsDisconnecting() == false, this value has no meaning.
            std::atomic<Timestamp> disconnect;

            /// The unique ID number of this remote endpoint. May be zero if handshake incomplete.
            PeerID id;

            /// The address of this remote endpoint. Datagrams will be sent here.
            RemoteAddress addr;

            /// A handle to the Socket that should be used to send datagrams to \p addr.
            std::shared_ptr<Socket> socket;

            /// A handle to the Handshaker that is to be used for this connection.
            std::unique_ptr<Handshaker> handshake;

            /// A handle to the congestion manager that is to be used for this connection.
            std::unique_ptr<CongestionControl> congestion;

            /// A handle to an object that services requests for delivery receipts.
            std::unique_ptr<ReceiptTracker> receipt;

            /// A collection of packets (plus headers) that haven't yet been fully delivered.
            std::vector<PacketQueue::OutgoingPacket> outbox;

            /// A collection of datagrams that haven't yet been fully delivered.
            std::vector<PacketQueue::OutgoingDatagram> sentbox;

            /// A sparse array of channel backlogs. Ordered / sequenced packets may be temporarily held here.
            std::unordered_map<ChannelIndex, std::unique_ptr<ChannelBuffer>> channels;

            /// Represents a synchronization primitive. This is used to sync access to the outbox and sentbox.
            cfg::LockableMutex lock;

            /**
             * \brief Reserves this RemotePeer, and randomizes the packet ID sequence.
             * 
             * \param[in]   master      The Peer that owns this RemotePeer instance.
             * \param[in]   origin      Sets who initiated the connection request.
             */
            void        Setup(Peer* master, Handshaker::Origin origin);

            /// Clears the state of this RemotePeer, and makes it available for new connections again.
            void        Reset();

            /// Returns a value indicating whether this socket is fully connected to a remote endpoint.
            bool        IsConnected() const;

            /// Returns a value indicating whether this socket is never connected to an endpoint, but rather
            /// used for sending messages to unconnected peers.
            bool        IsOutOfBand() const { return id == 0 || handshake == nullptr; }

            // Returns a value indicating whether a graceful disconnect is currently in progress.
            bool        IsDisconnecting() const { return disconnect.load().IsValid(); }

            /**
             * \brief Handle an incoming acknowledgement.
             * \param[in]   acklist     A list of datagram IDs that the remote endpoint acknowledges.
             */
            void        HandleAcknowledgements(const std::vector<DatagramID>& acklist);

            /**
             * \brief Handle an incoming non-acknowledgement.
             * \param[in]   naklist     A list of datagram IDs that the remote endpoint has not received.
             */
            void        HandleNonAcknowledgements(const std::vector<DatagramID>& naklist);

            /**
             * \brief Creates and returns a datagram to be sent to this remote peer.
             * 
             * One OutgoingDatagram will be constructed and populated with data, if any is available. This will
             * automatically poll the congestion manager and send outgoing acks, if required, otherwise queued
             * user data will be sent.
             * 
             * \param[in]   master  The Peer who owns this RemotePeer. If an error occurs, this Peer will be notified.
             * 
             * \returns A pointer to the newly queued OutgoingDatagram in this RemotePeer's sentbox; or nullptr
             *          if no data was ready to be sent.
             */
            PacketQueue::OutgoingDatagram*  GetNextDatagram(Peer* master);

            /**
             * Returns a non-owning pointer to a ChannelBuffer that represents the specified ChannelIndex.
             * 
             * A ChannelBuffer may be lazily constructed if required.
             * 
             * \param[in]   master      The IPeer whose channel settings will be retrieved.
             * \param[in]   index       The unique identifier of the desired channel.
             */
            ChannelBuffer*  GetChannelBuffer(const IPeer* master, ChannelIndex index);

            /**
             * \brief Erases an OutgoingPacket from the outbox, using its PacketID.
             * \param[in]   remove      The unique ID of the packet to remove.
             */
            void        RemovePacketFromOutbox(PacketID remove);
        };

        /// \endcond

    }

}
