/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#include "PCH.h"
#include "ReceiptTracker.h"
#include "Peer.h"
#include "RemotePeer.h"

using namespace detail;

ReceiptTracker::ReceiptTracker(Peer* master, RemotePeer& remote)
    : m_master(master)
    , m_remote(remote) {
    assert(m_master);
}

void ReceiptTracker::Track(PacketID id) {
    m_tracker.emplace(id);
}

void ReceiptTracker::Acknowledge(PacketID id) {
    auto it = m_splits.begin();
    while (it != m_splits.end()) {
        // remove this PacketID from the split group
        it->second.erase(id);

        // if no more entries exist in the split group, the entire thing was delivered
        if (it->second.empty()) {
            PacketID container = it->first;
            it = m_splits.erase(it);
            Acknowledge(container);
        } else {
            ++it;
        }
    }

    if (!m_tracker.count(id)) return;

    m_tracker.erase(id);
    m_master->OnMessageReceipt(id, true);
}

void ReceiptTracker::RegisterSplitPacket(PacketID container, std::set<PacketID> segments) {
    m_splits.emplace(container, std::move(segments));
}

void ReceiptTracker::Update() {
    WIREFOX_LOCK_GUARD(m_remote.lock);

    // find datagrams that are very old but still never (n)acked
    auto d_it = std::remove_if(m_remote.sentbox.begin(), m_remote.sentbox.end(), [this](const auto& datagram) {
        if (!Time::Elapsed(datagram.discard)) return false;

        // Probably not the most elegant way, putting the additional (lost-receipt) logic in the predicate,
        // but otherwise I'd have to iterate over the elements a second time, which just seems wasteful.
        for (auto packetID : datagram.packets)
            // notify the Peer that this message is considered lost; if the user requested such a receipt
            if (m_tracker.count(packetID)) {
                m_tracker.erase(packetID);
                m_master->OnMessageReceipt(packetID, false);
            }

        return true;
    });

    // clean them up
    m_remote.sentbox.erase(d_it, m_remote.sentbox.end());
}
