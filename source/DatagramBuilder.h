/**
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#pragma once
#include "PacketQueue.h"

namespace wirefox {
    
    namespace detail {
        
        /**
         * \cond WIREFOX_INTERNAL
         * \brief Utility class for building outgoing datagrams using a RemotePeer's outbox.
         * 
         * This class provides functions to build PacketQueue::OutgoingDatagram instances from the contents of a
         * RemotePeer's outbox, as well as datagrams that encapsulate one or more ACKs or NAKs.
         */
        class DatagramBuilder final {
        public:
            DatagramBuilder() = delete;
            ~DatagramBuilder() = delete;

            /**
             * \brief Builds a new OutgoingDatagram that contains user data.
             *
             * A number of pending packets will be selected and added to the datagram, so that less overhead is needed
             * to send many small packets. The created datagram, if any, will be assigned to the sentbox of \p remote.
             *
             * \param[in]   remote      The RemotePeer whose pending packets can be assigned to datagrams.
             * \param[in]   master      If an error occurs, this Peer will be notified of a disconnect. Should be the owner of \p remote.
             */
            static PacketQueue::OutgoingDatagram*   MakeDatagram(RemotePeer& remote, Peer* master);

            /**
             * \brief Builds a new OutgoingDatagram that contains some ACKs and/or NAKs. The created datagram,
             * if any, will be assigned to the sentbox of \p remote.
             *
             * \param[in]   remote      The RemotePeer whose pending ACK/NAKs will be packed into a datagram.
             */
            static PacketQueue::OutgoingDatagram*   MakeAckgram(RemotePeer& remote);
        };

        /// \endcond

    }

}
