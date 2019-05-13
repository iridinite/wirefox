/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#pragma once
#include "BinaryStream.h"
#include "Packet.h"

namespace wirefox {

    class IPeer;

    namespace detail {

        struct PacketHeader;
        struct RemotePeer;

        /**
         * \cond WIREFOX_INTERNAL
         * \brief Represents a buffer for storing partially received segmented packets.
         */
        class ReassemblyBuffer {
            /// Represents a partially received split packet.
            struct SplitPacket {
                /// The collection of indices of segments that have been received so far.
                std::set<uint32_t> received;
                /// The data buffer containing a partially reassembled packet.
                BinaryStream blob;
                /// The index of the last segment. Zero if unknown.
                uint32_t last = 0;
            };

        public:
            /// Default constructor.
            ReassemblyBuffer(RemotePeer* master);

            /// Copy constructor.
            ReassemblyBuffer(const ReassemblyBuffer&) = delete;
            /// Move constructor.
            ReassemblyBuffer(ReassemblyBuffer&&) noexcept;
            /// Destructor.
            ~ReassemblyBuffer() = default;

            /// Copy assignment operator.
            ReassemblyBuffer& operator=(const ReassemblyBuffer&) = delete;
            /// Move assignment operator.
            ReassemblyBuffer& operator=(ReassemblyBuffer&&) noexcept;

            /**
             * \brief Inserts a received segment into the correct location in the buffer.
             * 
             * \param[in]   header      The header associated with the received segment.
             * \param[in]   instream    A stream that contains the segment to be read.
             */
            void Insert(const PacketHeader& header, BinaryStream& instream);

            /**
             * \brief Attempts to reassemble a Packet with the specified container ID.
             * 
             * \param[in]   container   The packet ID that represents this group of segments.
             * \returns     An owning pointer to a reassembled packet, or nullptr if not yet ready.
             */
            std::unique_ptr<Packet> Reassemble(PacketID container);

        private:
            std::unordered_map<PacketID, SplitPacket> m_backlog;
            RemotePeer* m_remote;
        };

        /// \endcond

    }

}
