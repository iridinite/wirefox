/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#pragma once

/**
 * \file
 * \brief Global compile-time configuration file for Wirefox.
 */

// allow this file to be included in the C bindings header
#ifdef __cplusplus

namespace wirefox {

    // Forward declarations
    namespace detail {
        class CongestionControlWindow;
        class HandshakerThreeWay;
        class EncryptionLayerSodium;
        class EncryptionLayerNull;

        class SocketNX;
        class SocketUDP;

        struct RemoteAddressNX;
        struct RemoteAddressASIO;
    }

    /// Represents a unique identifier for a packet. This is used for tracking which packets are associated with which datagrams.
    using PacketID = uint32_t;

    /// Represents a unique identifier for a datagram. A remote endpoint acknowledges received datagrams using its ID.
    using DatagramID = uint32_t;

    /// Represents a concrete channel number that will be sent between hosts.
    using ChannelIndex = uint8_t;

    /// Represents a sequencing index. Ordered and sequenced packets use this to determine how they must be processed.
    using SequenceID = uint32_t;

    /// Represents a unique identifier for a specific peer on the network. Use this to address peers when sending them packets.
    using PeerID = uint64_t;

    namespace detail {
        /// Represents a remote socket address.
        using RemoteAddress =
#ifdef WIREFOX_PLATFORM_NX
            RemoteAddressNX;
#else
            RemoteAddressASIO;
#endif
    }

    /**
     * \brief Compile-time configuration tweakables.
     */
    namespace cfg {

        /// The Socket implementation to use. Changing this lets you easily swap out implementations.
#ifdef WIREFOX_PLATFORM_NX
        using DefaultSocket = detail::SocketNX;
#else
        using DefaultSocket = detail::SocketUDP;
#endif

        /// The Handshaker implementation to use. Changing this lets you easily swap out implementations.
        using DefaultHandshaker = detail::HandshakerThreeWay;

        /// The Handshaker implementation to use. Changing this lets you easily swap out implementations.
        using DefaultCongestionControl = detail::CongestionControlWindow;

#ifdef WIREFOX_ENABLE_ENCRYPTION
        /// The EncryptionLayer implementation to use. Changing this lets you easily swap out implementations.
        using DefaultEncryption = detail::EncryptionLayerSodium;
#else
        /// The EncryptionLayer implementation to use. This is a dummy implementation that does nothing.
        using DefaultEncryption = detail::EncryptionLayerNull;
#endif

        /// Specifies a header that Handshaker should send (and receive), to confirm that both endpoints are running Wirefox.
        constexpr static uint8_t WIREFOX_MAGIC[] = {'W', 'I', 'R', 'E', 'F', 'O', 'X'};

        /// Specifies the current protocol version. Peers will reject connections with peers who have a mismatching protocol version.
        constexpr static uint8_t WIREFOX_PROTOCOL_VERSION = 0;

        /**
         * \brief Sets the Maximum Transmission Unit: maximum length of a single outgoing datagram in bytes.
         *
         * Tweak this value with caution: if any intermediate network link cannot handle packets of this size,
         * then the packet may be fragmented (best case, but very slow), or outright discarded (worst case).
         * For reference, the maximum recommended size for Ethernet links is 1500 bytes, INCLUDING headers/overhead.
         */
        constexpr static size_t MTU = 1300;

        /**
         * \brief Sets the length of the incoming stream buffer in bytes.
         *
         * Larger values mean fewer round trips between the network stream and the packet manager,
         * but require more memory for each active connection.
         */
        constexpr static size_t PACKETQUEUE_IN_LEN = MTU;

        /**
         * \brief Sets the slow-start threshold of the window-based congestion manager.
         * 
         * The outgoing buffer size (cwnd) will keep growing exponentially until this threshold is hit,
         * after which it will continue growing at a somewhat linear pace.
         */
        constexpr static size_t CONGESTION_WINDOW_SSTHRESH = 65536;

        /**
         * \brief Sets the length of the round-trip-time history buffer.
         *
         * Every time a datagram is acknowledged, the congestion manager tracks how long it took for that ack
         * to arrive, vs. the moment the packet was sent out. This data is used to determine the speed of the
         * connection.
         */
        constexpr static size_t CONGESTION_RTT_HISTORY_LEN = 32;

        /**
         * \brief Sets the maximum length of a single message.
         *
         * No single datagram may ever be bigger than this limit, even when it is split into multiple smaller
         * packets. If you need to send larger messages, either increase this limit (at your own risk), or
         * manually reduce individual packet size (better option). Default is 16 MB.
         */
        constexpr static size_t PACKET_MAX_LENGTH = 16777216;

        /**
         * \brief Sets the default storage capacity, in bytes, for a new BinaryStream.
         * 
         * This value is used if no other value is explicitly specified. Small values are good
         * when you have many small packets. Large values reduce the number of reallocations.
         */
        constexpr static size_t BINARYSTREAM_DEFAULT_CAPACITY = 128;

        /**
         * \brief Sets the sleep time in milliseconds for the packet queue.
         *
         * In between two cycles of the packet queue (dispatching read and write operations), it
         * will sleep this many milliseconds. Lower values may cause outgoing and incoming packets
         * to be processed faster, possibly at the cost of extra CPU time. Be sure to profile.
         *
         * Minimum value is 1 millisecond.
         */
        constexpr static unsigned int THREAD_SLEEP_PACKETQUEUE_TICK = 5;

        /**
         * \brief Sets the maximum number of connection requests that are sent out.
         * 
         * If this number of attempts fail, the remote host is considered unreachable.
         * Minimum value is 1.
         */
        constexpr static unsigned int CONNECT_RETRY_COUNT = 4;

        /**
         * \brief Sets the delay, in milliseconds, between two connection attempts.
         */
        constexpr static unsigned int CONNECT_RETRY_DELAY = 2000;

        /**
         * \brief Sets the maximum number of times a packet can be sent over the network.
         * 
         * If this threshold is exceeded, the connection is treated as lost.
         * Minimum value is 1.
         */
        constexpr static unsigned int SEND_RETRY_COUNT = 6;

        /**
         * \brief Controls whether the network simulator is enabled.
         * 
         * In debug environments, you may wish to introduce artificial packet loss and/or extra latency,
         * to simulate a wonky connection and help test how your application deals with it. In release
         * builds, this should be set to 0, to save a tiny amount of overhead.
         * 
         * Default is 0 (disabled). Set to 1 to enable.
         */
        #define WIREFOX_ENABLE_NETWORK_SIM          0

        /**
         * \brief Declares a lockable mutex.
         * 
         * This alias is expected to expand to a mutex suitable for the WIREFOX_LOCK_GUARD macro.
         */
        using LockableMutex =                       std::mutex;

        /**
         * \brief Declares a recursively lockable mutex.
         *
         * This alias is expected to expand to a mutex suitable for the WIREFOX_LOCK_GUARD macro. This
         * variation should specifically allow multiple locks on the same thread.
         */
        using RecursiveMutex =                      std::recursive_mutex;

        // Helper macros for concatenating two values
        #define WIREFOX_STRINGIFY_INNER(x, y) x ## y
        #define WIREFOX_STRINGIFY(x, y) WIREFOX_STRINGIFY_INNER(x, y)

        /**
         * \brief Declares a lock guard operating on the specified mutex.
         * 
         * This macro is expected to expand to a RAII lock guard. The reference implementation is std::lock_guard.
         * If you want to disable locking for some reason, make this macro expand to nothing.
         * 
         * \attention The lock guard is expected to follow RAII principles. That is, the mutex should be locked
         * the moment the guard is constructed, and unlocked when the guard is destructed.
         */
        #define WIREFOX_LOCK_GUARD(__mtx_name)      ::std::lock_guard<decltype(__mtx_name)> WIREFOX_STRINGIFY(__guard_line, __LINE__)(__mtx_name)

    }

}

#endif // __cplusplus

// Here's a forest of macros for doing a platform-specific dllexport/import
#ifdef WIREFOX_SHARED_LIB
#if defined(_MSC_VER) || defined(__CYGWIN__)
    #ifdef WIREFOX_COMPILING
        #ifdef __GNUC__
            #define WIREFOX_API __attribute__ ((dllexport))
        #else
            #define WIREFOX_API __declspec(dllexport)
        #endif
    #else
        #ifdef __GNUC__
            #define WIREFOX_API __attribute__ ((dllimport))
        #else
            #define WIREFOX_API __declspec(dllimport)
        #endif
    #endif
#else
    #if __GNUC__ >= 4
        #define WIREFOX_API __attribute__ ((visibility ("default")))
    #else
        #define WIREFOX_API
    #endif
#endif
#else
    #define WIREFOX_API
#endif
