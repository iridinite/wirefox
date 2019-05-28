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
        
        class RpcController {
        public:
            RpcController() = default;

            void Slot(const std::string& identifier, RpcCallbackAsync_t fn);
            void RemoveSlot(const std::string& identifier);

            void Signal(const std::string& identifier, IPeer& peer, PeerID sender, BinaryStream& params) const;

        private:
            std::unordered_map<size_t, std::vector<RpcCallbackAsync_t>> m_slots;
        };

    }

}
