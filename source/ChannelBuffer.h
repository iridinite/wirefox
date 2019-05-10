/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#pragma once
#include "Packet.h"

namespace wirefox {

    class IPeer;

    namespace detail {

        /**
         * \cond WIREFOX_INTERNAL
         * \brief Represents a buffer for withholding packets that need to be ordered or sequenced.
         */
        class ChannelBuffer {
            using Payload = std::unique_ptr<Packet>;
            using Backlog = std::map<SequenceID, Payload>;

        public:
            /**
             * \brief Constructs a new ChannelBuffer using channel info from an IPeer.
             * 
             * \param[in]   peer    This IPeer will be queried for channel settings.
             * \param[in]   index   The identifier for this channel.
             */
            ChannelBuffer(const IPeer* peer, ChannelIndex index);

            /// Copy constructor.
            ChannelBuffer(const ChannelBuffer&) = default;
            /// Move constructor.
            ChannelBuffer(ChannelBuffer&&) noexcept = default;
            /// Destructor.
            ~ChannelBuffer() = default;

            /// Copy assignment operator.
            ChannelBuffer& operator=(const ChannelBuffer&) = default;
            /// Move assignment operator.
            ChannelBuffer& operator=(ChannelBuffer&&) noexcept = default;

            /**
             * \brief Processes an incoming payload.
             * 
             * The packet will be added to the ordering queue, unless the channel is sequenced and this
             * packet arrived out of order, in which case the packet will be discarded.
             * 
             * \param[in]   sequence    The channel sequence number of this packet.
             * \param[in]   packet      The payload to be queued.
             */
            void            Enqueue(SequenceID sequence, Payload packet);

            /**
             * \brief Returns the next available payload, if any.
             * 
             * This function will return payloads in the correct order. If this channel is ordered and a
             * packet in the sequence is missing, this function will return nullptr.
             * 
             * \returns     Owning pointer to the next queue item, or nullptr if none available.
             */
            Payload         Dequeue();

            /**
             * \brief Returns and increments the next sequence number for this channel.
             */
            SequenceID      GetNextOutgoing();

        private:
            ChannelMode     GetMode() const;
            bool            IsEligible(SequenceID sequence) const;

            Backlog         m_backlog;
            const IPeer*    m_peer;
            SequenceID      m_nextEnqueue;
            SequenceID      m_nextDequeue;
            SequenceID      m_outgoing;
            ChannelIndex    m_index;
        };

        /// \endcond

    }

}
