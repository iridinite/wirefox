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

    constexpr size_t djb2hash(const char* str) {
        size_t hash = 5381;

        do {
            int c = *str++;
            hash = ((hash << 5) + hash) + c; // hash * 33 + c
        } while (*str != 0);

        return hash;
    }

}

void RpcController::SlotAsync(const std::string& identifier, RpcCallbackAsync_t fn) {
    const auto hash = djb2hash(identifier.c_str());

    // find the slot group by its string name, or create it if it doesn't exist
    auto group_itr = m_slotsAsync.find(hash);
    if (group_itr == m_slotsAsync.end())
        group_itr = m_slotsAsync.insert(m_slotsAsync.cend(), std::make_pair(hash, decltype(m_slotsAsync)::mapped_type()));
    assert(group_itr != m_slotsAsync.end());

    // add the callback to this slot group
    group_itr->second.push_back(std::move(fn));
}

void RpcController::SlotBlocking(const std::string& identifier, RpcCallbackBlocking_t fn) {
    const auto hash = djb2hash(identifier.c_str());

    // update or insert
    m_slotsBlocking[hash] = std::move(fn);
}

void RpcController::RemoveSlot(const std::string& identifier) {
    const auto hash = djb2hash(identifier.c_str());

    auto itr1 = m_slotsAsync.find(hash);
    if (itr1 != m_slotsAsync.end())
        m_slotsAsync.erase(itr1);

    auto itr2 = m_slotsBlocking.find(hash);
    if (itr2 != m_slotsBlocking.end())
        m_slotsBlocking.erase(itr2);
}

void RpcController::Signal(const std::string& identifier, IPeer& peer, PeerID sender, BinaryStream& params) const {
    const auto hash = djb2hash(identifier.c_str());
    auto group_itr = m_slotsAsync.find(hash);
    if (group_itr == m_slotsAsync.end())
        return;

    for (const auto& fn : group_itr->second) {
        // invoke the callback with a new readonly copy of the stream, so callbacks are unaffected by other's reads/seeks
        BinaryStream instreamCopy(params.GetBuffer(), params.GetLength(), BinaryStream::WrapMode::READONLY);
        fn(peer, sender, instreamCopy);
    }
}

void RpcController::SignalBlocking(const std::string& identifier, IPeer& peer, PeerID sender, BinaryStream& params, BinaryStream& outstream) const {
    const auto hash = djb2hash(identifier.c_str());
    auto itr = m_slotsBlocking.find(hash);
    if (itr == m_slotsBlocking.end())
        return;

    itr->second(peer, sender, params, outstream);
}

bool RpcController::AwaitBlockingReply(const std::string& identifier, PeerID recipient, BinaryStream& outstream) {
    // maintain a reference to the AwaitedReply in this function body, so it can be removed from the list
    std::shared_ptr<AwaitedReply> entry;

    // try to find an existing awaited reply with the same recipient and ID hash
    const auto hash = djb2hash(identifier.c_str());
    {
        WIREFOX_LOCK_GUARD(m_slotsAwaitingMutex);
        auto itr = std::find_if(m_slotsAwaiting.begin(), m_slotsAwaiting.end(), [hash, recipient](const decltype(m_slotsAwaiting)::value_type& candidate) {
            return candidate->hash == hash && candidate->recipient == recipient;
        });

        // if none found, create one
        if (itr == m_slotsAwaiting.end()) {
            entry = std::make_shared<AwaitedReply>();
            entry->hash = hash;
            entry->recipient = recipient;
            //itr = m_slotsAwaiting.insert(m_slotsAwaiting.end(), entry);
            m_slotsAwaiting.push_back(entry);
        } else {
            entry = *itr;
        }
    }

    // await response, see Peer::OnSystemPacket
    entry->awaiter.Wait();

    outstream = std::move(entry->response);
    return !entry->interrupted;
}

void RpcController::SignalBlockingReply(const std::string& identifier, PeerID recipient, BinaryStream& response) {
    const auto hash = djb2hash(identifier.c_str());

    // find the awaited reply with matching ID and recipient
    WIREFOX_LOCK_GUARD(m_slotsAwaitingMutex);
    auto itr = std::find_if(m_slotsAwaiting.begin(), m_slotsAwaiting.end(), [hash, recipient](const decltype(m_slotsAwaiting)::value_type& candidate) {
        return candidate->hash == hash && candidate->recipient == recipient;
    });
    if (itr == m_slotsAwaiting.end()) return;

    // unblock it
    auto entry = *itr;
    entry->response = std::move(response);
    entry->interrupted = false;
    entry->awaiter.Signal();
    m_slotsAwaiting.erase(itr);
}

void RpcController::SignalAllBlockingReplies(PeerID recipient) {
    WIREFOX_LOCK_GUARD(m_slotsAwaitingMutex);
    auto itr = std::remove_if(m_slotsAwaiting.begin(), m_slotsAwaiting.end(), [recipient](const decltype(m_slotsAwaiting)::value_type& candidate) {
        return candidate->recipient == recipient;
    });

    // unblock all waiting threads
    auto itr_copy = itr;
    while (itr_copy != m_slotsAwaiting.end()) {
        auto entry = *itr_copy;
        entry->interrupted = true;
        entry->awaiter.Signal();
        
        ++itr_copy;
    }

    // erase the elements from the container
    m_slotsAwaiting.erase(itr, m_slotsAwaiting.end());
}
