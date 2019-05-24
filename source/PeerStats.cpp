/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#include "PCH.h"
#include "PeerStats.h"

PeerStats::StatValue PeerStats::Get(PeerStatID id) const {
    auto itr = m_stats.find(id);
    if (itr == m_stats.end()) return 0;

    return itr->second;
}

void PeerStats::Add(PeerStatID id, StatValue delta) {
    // find iterator or create if not existing yet
    auto itr = m_stats.find(id);
    if (itr == m_stats.end())
        itr = m_stats.emplace(id, 0).first;

    // add delta
    itr->second += delta;
}

void PeerStats::Set(PeerStatID id, StatValue value) {
    // find iterator or create if not existing yet
    auto itr = m_stats.find(id);
    if (itr == m_stats.end())
        itr = m_stats.emplace(id, 0).first;

    // set total
    itr->second = value;
}
