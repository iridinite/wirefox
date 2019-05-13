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

    namespace detail {

        struct RemotePeer;
        class Peer;

        /**
         * \cond WIREFOX_INTERNAL
         * \brief Represents a small container that tracks and posts message receipts.
         * 
         * If the user requests a receipt with PacketOptions::WITH_RECEIPT, it gets tracked here.
         * 
         * By extent, it is also responsible for cleaning up old and unacked datagrams in a RemotePeer's
         * sentbox, posting negative receipts for them if required.
         */
        class ReceiptTracker {
        public:
            /**
             * \brief Constructs a new ReceiptTracker instance.
             * 
             * \param[in]   master      The Peer to whom any released receipts will be posted.
             * \param[in]   remote      The RemotePeer for whom this ReceiptTracker is responsible.
             */
            ReceiptTracker(Peer* master, RemotePeer& remote);

            /**
             * \brief Requests that a receipt be released later for a specific packet.
             * \param[in]   id          The PacketID for which a receipt was requested.
             */
            void            Track(PacketID id);

            /**
             * \brief Informs the tracker that the specified PacketID was successfully delivered.
             * \param[in]   id          The PacketID that was acknowledged.
             */
            void            Acknowledge(PacketID id);

            /**
             * \brief Registers a container packet with its list of segments, so it can be tracked.
             * 
             * When all IDs listed in \p segments are acknowledged, then \p container will be acknowledged
             * as well. This enables ack receipts for split packets.
             * 
             * \param[in]   container   The ID that represents this group of split packets.
             * \param[in]   segments    The list of sub-packets (segments) making up the split packet.
             */
            void            RegisterSplitPacket(PacketID container, std::set<PacketID> segments);

            /**
             * \brief Prunes old datagrams and posts negative receipts for old, unacked packets.
             */
            void            Update();

        private:
            Peer* m_master;
            RemotePeer& m_remote;

            std::unordered_map<PacketID, std::set<PacketID>> m_splits;
            std::set<PacketID> m_tracker;
        };

        /// \endcond

    }

}
