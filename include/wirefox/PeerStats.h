/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#pragma once

namespace wirefox {

    /**
     * \brief Represents a key for looking up or changing peer statistics.
     */
    enum class PeerStatID {
        /// Total number of bytes sent out. This includes system overhead such as headers and acks.
        BYTES_SENT,
        /// Total number of bytes received. This includes system overhead such as headers and acks.
        BYTES_RECEIVED,
        /// Number of bytes sent but not yet accounted for (i.e. still in transit).
        BYTES_IN_FLIGHT,
        /// Number of Packets the user has queued using Send().
        PACKETS_QUEUED,
        /// Number of Packets that are queued but not yet delivered.
        PACKETS_IN_QUEUE,
        /// Total number of Packets that have been sent out, including retransmissions.
        PACKETS_SENT,
        /// Total number of full Packets received from the remote endpoint.
        PACKETS_RECEIVED,
        /// Total number of times a reliable outgoing Packet was deemed lost and was requeued.
        PACKETS_LOST,
        /// Congestion window size, in bytes. This is essentially the estimated bandwidth of this connection.
        CWND,
    };

    /**
     * \brief Tracks certain statistics about a remote peer.
     * 
     * You can obtain a read-only pointer to a PeerStats regarding a particular connection by calling
     * IPeer::GetStats().
     */
    class PeerStats {
    public:
        /// The underlying type used for stat values.
        using StatValue = size_t;

        /// Default constructor.
        PeerStats() = default;

        /**
         * \brief Retrieves the current value of a statistic.
         * 
         * \param[in]   id      Which stat to query.
         */
        StatValue Get(PeerStatID id) const;

        /**
         * \brief Adds a delta to a statistic.
         * 
         * \param[in]   id      Which stat to edit.
         * \param[in]   delta   The number to add to the stat.
         */
        void Add(PeerStatID id, StatValue delta);

        /**
         * \brief Directly sets the value of a statistic.
         * 
         * \param[in]   id      Which stat to edit.
         * \param[in]   value   The new value to assign to this stat.
         */
        void Set(PeerStatID id, StatValue value);

    private:
        std::unordered_map<PeerStatID, StatValue> m_stats;
    };

}
