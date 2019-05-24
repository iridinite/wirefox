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
        /// Total number of Packets the user has queued using Send().
        PACKETS_QUEUED,
        /// Number of Packets that are queued by the user but not yet delivered.
        PACKETS_IN_QUEUE,
        /// Total number of user packets that have been attached to datagrams, including retransmissions and individual segments of split packets.
        PACKETS_SENT,
        /// Total number of user packets received from the remote endpoint, including retransmissions and individual segments of split packets.
        PACKETS_RECEIVED,
        /// Total number of times a packet was deemed lost because its parent datagram was not acked in time.
        PACKETS_LOST,
        /// [Meant for debugging.] Total number of datagrams that have been sent out, including retransmissions and system messages.
        DATAGRAMS_SENT,
        /// [Meant for debugging.] Total number of datagrams received from the remote endpoint, including retransmissions and system messages.
        DATAGRAMS_RECEIVED,
        /// [Meant for debugging.] Congestion window size, in bytes. This acts as an upper limit for BYTES_IN_FLIGHT.
        CWND
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
        // because GCC and Clang apparently can't deal with strong enums as unordered_map keys; filed under *shrug*
        struct StrongEnumHash {
            template<typename T>
            size_t operator()(T val) const {
                return static_cast<size_t>(val);
            }
        };

        std::unordered_map<PeerStatID, StatValue, StrongEnumHash> m_stats;
    };

}
