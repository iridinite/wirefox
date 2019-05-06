/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#pragma once
#include "Enumerations.h"

namespace wirefox {

    class BinaryStream;

    namespace detail {

        /**
         * \cond WIREFOX_INTERNAL
         * \brief Represents a header for an individual Packet inside a datagram.
         */
        struct PacketHeader {
            /// Indicates whether this packet is segmented.
            bool        flag_segment = false;

            /// Indicates whether this is a jumbogram. If set, length and offset widen to 32 bits.
            bool        flag_jumbo = false;

            /// The sequence number of this packet. Used for detecting duplicate packets.
            PacketID    id = 0;

            /// Any additional options assigned to this packet.
            PacketOptions options = PacketOptions::UNRELIABLE; // dummy

            /// The channel index to which this packet was assigned.
            ChannelIndex  channel = 0;

            /// The sequence number inside the channel. May be zero for unordered channels.
            SequenceID  sequence = 0;

            /// The length of the packet's payload segment, in bytes.
            uint32_t    length = 0;

            /// The offset into the complete payload, in bytes. Zero if not segmented packet.
            uint32_t    offset = 0;

            PacketHeader() = default;

            /**
             * \brief Serializes this PacketHeader to an output stream.
             *
             * \param[out]  outstream   The stream to write this header to.
             */
            void        Serialize(BinaryStream& outstream) const;

            /**
             * \brief Reads a PacketHeader from an input stream and fills in this instance.
             *
             * \param[in]   instream    The stream to read the header from.
             * \returns     True if a valid header was successfully read, false otherwise.
             */
            bool        Deserialize(BinaryStream& instream);
        };

        /// \endcond

    }

}
