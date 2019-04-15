#pragma once
#include "WirefoxTime.h"

namespace wirefox {
    
    namespace detail {
        
        /**
         * \cond WIREFOX_INTERNAL
         * \brief Represents a monitor for predicting and avoiding network congestion.
         * 
         * This class monitors incoming and outgoing network traffic and (non-)acknowledgements, and informs PacketQueue how
         * much data it can safely put on the wire, so as to avoid clogging up the network.
         */
        class CongestionControl {
        public:
            /// Indicates how an incoming datagram should be treated.
            enum class RecvState {
                NEW,        ///< This datagram is new and should be processed.
                DUPLICATE   ///< This datagram has been received before, and should be ignored.
            };

            /// Default constructor.
            CongestionControl() = default;
            /// Copy constructor - not allowed because duplicating this object makes no sense.
            CongestionControl(const CongestionControl&) = delete;
            /// Move constructor.
            CongestionControl(CongestionControl&&) noexcept = delete;
            /// Destructor.
            virtual ~CongestionControl() = default;

            /// Copy assignment operator - not allowed because duplicating this object makes no sense.
            CongestionControl& operator=(const CongestionControl&) = delete;
            /// Move assignment operator.
            CongestionControl& operator=(CongestionControl&&) noexcept = delete;

            /// Runs periodic updates, particularly cleaning up old history.
            virtual void        Update() = 0;

            /// Returns, and increments, the next outgoing PacketID.
            virtual PacketID    GetNextPacketID() = 0;

            /// Returns, and increments, the next outgoing DatagramID.
            virtual DatagramID  GetNextDatagramID() = 0;

            /// Returns, but does not increment, the next outgoing PacketID.
            virtual DatagramID  PeekNextDatagramID() const = 0;

            /// Calculates and returns the estimated bandwidth to be used for sending new packets.
            virtual size_t      GetTransmissionBudget() const = 0;

            /// Calculates and returns the estimated bandwidth to be used for re-sending unacknowledged packets.
            virtual size_t      GetRetransmissionBudget() const = 0;

            /**
             * \brief Calculates and returns the amount of time the peer should wait before resending a packet.
             * 
             * \param[in]   retries     The number of times this packet has been sent so far.
             */
            virtual Timespan    GetRetransmissionRTO(unsigned retries) const = 0;

            /// Returns a value indicating whether the congestion manager wishes to send acks now.
            virtual bool        GetNeedsToSendAcks() const = 0;

            /**
             * \brief Outputs the list of acks and nacks that need to be sent to the remote endpoint.
             * 
             * \param[out]      acks    Will be filled with outgoing ACKs.
             * \param[out]      nacks   Will be filled with outgoing NAKs.
             */
            virtual void        MakeAckList(std::vector<DatagramID>& acks, std::vector<DatagramID>& nacks) = 0;

            /**
             * \brief Informs the manager that a number of bytes is being put on the wire.
             * 
             * \param[in]       outgoing    The DatagramID representing the outgoing data.
             * \param[in]       bytes       The number of bytes the specified datagram is in total (header included).
             */
            virtual void        NotifySendingBytes(DatagramID outgoing, size_t bytes) = 0;

            /**
             * \brief Informs the manager that an ACK has arrived from the remote endpoint.
             * 
             * \param[in]       recv        The DatagramID that was sent out earlier, and now acknowledged.
             */
            virtual void        NotifyReceivedAck(DatagramID recv) = 0;

            /**
             * \brief Informs the manager that a NAK has arrived from the remote endpoint.
             */
            virtual void        NotifyReceivedNakGroup() = 0;

            /**
             * \brief Informs the manager that an incoming datagram has arrived.
             *
             * \param[in]       recv            The ID number of the datagram that was just received.
             * \param[in]       isAckDatagram   Indicates whether this datagram is an ACK/NAK group.
             * \returns         A value indicating whether this datagram should be processed.
             */
            virtual RecvState   NotifyReceivedDatagram(DatagramID recv, bool isAckDatagram) = 0;

            /**
             * \brief Informs the manager that a packet has been decoded from a datagram.
             *
             * \param[in]       recv            The ID number of the packet that was just received.
             * \returns         A value indicating whether this packet should be processed.
             */
            virtual RecvState   NotifyReceivedPacket(PacketID recv) = 0;

        protected:
            /// Indicates whether \p lhs is greater than \p rhs, accounting for unsigned overflow.
            static bool         SequenceGreaterThan(DatagramID lhs, DatagramID rhs);
            /// Indicates whether \p lhs is less than \p rhs, accounting for unsigned overflow.
            static bool         SequenceLessThan(DatagramID lhs, DatagramID rhs);
        };

        /// \endcond

    }

}
