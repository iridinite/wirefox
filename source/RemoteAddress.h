/**
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

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
