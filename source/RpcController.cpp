/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#include "PCH.h"
#include "RpcController.h"
#include "BinaryStream.h"

using namespace detail;

namespace {

    // I strongly considered using std::hash<std::string>, however I need all hashes to be identical
    // across compilers and platforms, which the standard does not guarantee.

    constexpr size_t djb2hash(const char* str) {
        size_t hash = 5381;
        int c = 0;

        while ((c = *str++))
            hash = ((hash << 5) + hash) + c; // hash * 33 + c

        return hash;
    }

}

void RpcController::Slot(const std::string& identifier, RpcCallbackAsync_t fn) {
    const auto hash = djb2hash(identifier.c_str());

    // find the slot group by its string name, or create it if it doesn't exist
    auto group_itr = m_slots.find(hash);
    if (group_itr == m_slots.end())
        group_itr = m_slots.insert(m_slots.cend(), std::make_pair(hash, decltype(m_slots)::mapped_type()));
    assert(group_itr != m_slots.end());

    // add the callback to this slot group
    group_itr->second.push_back(std::move(fn));
}

void RpcController::RemoveSlot(const std::string& identifier) {
    const auto hash = djb2hash(identifier.c_str());
    auto group_itr = m_slots.find(hash);
    if (group_itr != m_slots.end())
        m_slots.erase(group_itr);
}

void RpcController::Signal(const std::string& identifier, IPeer& peer, PeerID sender, BinaryStream& params) const {
    const auto hash = djb2hash(identifier.c_str());
    auto group_itr = m_slots.find(hash);
    if (group_itr == m_slots.end())
        return;

    for (const auto& fn : group_itr->second) {
        // invoke the callback with a new readonly copy of the stream, so callbacks are unaffected by other's reads/seeks
        BinaryStream instreamCopy(params.GetBuffer(), params.GetLength(), BinaryStream::WrapMode::READONLY);
        fn(peer, sender, instreamCopy);
    }
}
