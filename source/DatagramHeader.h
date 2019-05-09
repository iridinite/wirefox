/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#pragma once
#include "WirefoxConfig.h"

namespace wirefox {

    class Packet;
    class BinaryStream;

    namespace detail {

        struct PacketHeader;

        /**
         * \cond WIREFOX_INTERNAL
         * \brief Represents a header for a full datagram.
         * 
         * A datagram may contain zero or more PacketHeaders, each of which is followed by a Packet.
         */
        struct DatagramHeader {
            /// Indicates whether or not this datagram includes a payload (i.e. any packets).
            bool        flag_data = false;

            /// Indicates whether the sender thinks it is connected to the receiver.
            bool        flag_link = false;

            /// Collection of DatagramIDs originally sent by receiver, which sender has succesfully received.
            std::vector<DatagramID> acks;

            /// Collection of DatagramIDs originally sent by receiver, which never arrived at sender.
            std::vector<DatagramID> nacks;

            /// The sequence number of this datagram. Used for acking.
            DatagramID  datagramID = 0;

            /// The length of this datagram's payload. Zero if flag_data unset.
            size_t      dataLength = 0;

            DatagramHeader() = default;

            /**
             * \brief Serializes this DatagramHeader to an output stream.
             * 
             * \param[out]  outstream   The stream to write this header to.
             */
            void        Serialize(BinaryStream& outstream) const;

            /**
             * \brief Reads a DatagramHeader from an input stream and fills in this instance.
             * 
             * \param[in]   instream    The stream to read the header from.
             * \returns     True if a valid header was successfully read, false otherwise.
             */
            bool        Deserialize(BinaryStream& instream);
        };

        /// \endcond

    }

}
