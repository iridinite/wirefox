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

    class BinaryStream;

    namespace detail {
        
        /**
         * \brief Manages a collection of registered RPCs, and can invoke them on request.
         */
        class RpcController {
        public:
            RpcController() = default;

            /**
             * \brief Registers a new RPC callback.
             * 
             * \param[in]   identifier  The shared identifier that can be used to invoke this callback.
             * \param[in]   fn          The callback to invoke if \p identifier is signalled.
             */
            void Slot(const std::string& identifier, RpcCallbackAsync_t fn);

            /**
             * \brief Removes all RPC callbacks with a specific identifier.
             * 
             * \param[in]   identifier  The shared identifier that is to be erased.
             */
            void RemoveSlot(const std::string& identifier);

            /**
             * \brief Invokes all RPC callbacks with the specified identifier.
             * 
             * \param[in]   identifier  The shared identifier to signal.
             * \param[in]   peer        Passed on to callback. The IPeer that owns this RpcController.
             * \param[in]   sender      Passed on to callback. The PeerID of the remote peer who sent this signal.
             * \param[in]   params      Passed on to callback. Arbitrary user data.
             */
            void Signal(const std::string& identifier, IPeer& peer, PeerID sender, BinaryStream& params) const;

        private:
            std::unordered_map<size_t, std::vector<RpcCallbackAsync_t>> m_slots;
        };

    }

}
