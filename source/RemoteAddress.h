#pragma once

namespace wirefox {

    namespace detail {

        /**
         * \cond WIREFOX_INTERNAL
         * \brief Represents the network address of a remote endpoint.
         */
        struct RemoteAddress {
            friend class SocketUDP;

            RemoteAddress() = default;
            ~RemoteAddress() = default;

            /// Equality operator.
            bool operator==(const RemoteAddress& rhs) const;

            /// Inequality operator.
            bool operator!=(const RemoteAddress& rhs) const;

            /// Returns a string that describes this RemoteAddress. Meant for debugging.
            std::string ToString() const;

        private:
            asio::ip::udp::endpoint endpoint_udp;
        };

        /// \endcond

    }

}
