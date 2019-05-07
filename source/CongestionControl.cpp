/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#include "PCH.h"
#include "CongestionControl.h"

using namespace wirefox::detail;

bool CongestionControl::SequenceGreaterThan(DatagramID lhs, DatagramID rhs) {
    constexpr decltype(lhs) halfSpan = static_cast<decltype(lhs)>(std::numeric_limits<decltype(lhs)>::max()) / 2;
    return lhs != rhs && rhs - lhs > halfSpan;
}

bool CongestionControl::SequenceLessThan(DatagramID lhs, DatagramID rhs) {
    constexpr decltype(lhs) halfSpan = static_cast<decltype(lhs)>(std::numeric_limits<decltype(lhs)>::max()) / 2;
    return lhs != rhs && rhs - lhs < halfSpan;
}

CongestionControl::CongestionControl()
    : m_nextPacket(0)
    , m_nextDatagram(0)
    , m_remoteDatagram(0)
    , m_nextUpdate(Time::Now())
    , m_bytesInFlight(0)
    , m_rttMin(0)
    , m_rttMax(0)
    , m_rttAvg(0) {}

void CongestionControl::Update() {
    // Many calls in rapid succession aren't necessary; this might save us some time wasted doing
    // list traversals if we already cleaned them up 1 ms ago
    if (!Time::Elapsed(m_nextUpdate)) return;
    m_nextUpdate = Time::Now() + Time::FromMilliseconds(20);

    // Prefer caching this over Elapsed(), because Elapsed() wraps Now(), and getting clock is somewhat expensive
    auto now = Time::Now();

    // Traverse outgoing entries, and discard those that are so old they're probably irrelevant.
    // This ensures that even un-acked packets (ack never sent, or got lost) will be removed.
#if _DEBUG
    size_t bytesStillOutgoing = 0;
#endif
    Timespan wireExpireTime = m_rttAvg * 16; // somewhat arbitrary long time
    auto itr_flight = m_outgoing.begin();
    while (itr_flight != m_outgoing.end()) {
        const auto& pif = itr_flight->second;
        if (now >= pif.sent + wireExpireTime) {
            m_bytesInFlight -= pif.bytes;
            itr_flight = m_outgoing.erase(itr_flight);
        }
        else {
#if _DEBUG
            bytesStillOutgoing += pif.bytes;
#endif
            ++itr_flight;
        }
    }

#if _DEBUG
    assert(m_bytesInFlight == bytesStillOutgoing);
#endif

    // Clean up expired history for both packets and datagrams.
    // Unfortunately we can't use remove_if with an std::map... so this is more or less the closest, I think.
    Timespan historyExpireTime = Time::FromSeconds(10);

    for (auto itr_datagram = m_datagramHistory.begin(), itend = m_datagramHistory.end(); itr_datagram != itend;) {
        if (now >= itr_datagram->second + historyExpireTime)
            itr_datagram = m_datagramHistory.erase(itr_datagram);
        else
            ++itr_datagram;
    }
    for (auto itr_packet = m_packetHistory.begin(), itend = m_packetHistory.end(); itr_packet != itend;) {
        if (now >= itr_packet->second + historyExpireTime)
            itr_packet = m_packetHistory.erase(itr_packet);
        else
            ++itr_packet;
    }
}

PacketID CongestionControl::GetNextPacketID() {
    return m_nextPacket++;
}

DatagramID CongestionControl::GetNextDatagramID() {
    return m_nextDatagram++;
}

DatagramID CongestionControl::PeekNextDatagramID() const {
    return m_nextDatagram;
}

bool CongestionControl::GetRTTHistoryAvailable() const {
    return m_rttHistory.size() >= 2; // a few samples
}

unsigned CongestionControl::GetAverageRTT() const {
    auto ms = Time::ToMilliseconds(m_rttAvg);
    assert(ms >= 0 && "RTT going back in time would be rather impressive");

    return static_cast<unsigned>(ms);
}

void CongestionControl::MakeAckList(std::vector<DatagramID>& acks, std::vector<DatagramID>& nacks) {
    assert(acks.empty());
    assert(nacks.empty());
    acks.swap(m_acks);
    nacks.swap(m_nacks);
}

void CongestionControl::RecalculateRTT() {
    m_rttAvg = 0;
    m_rttMax = 0;
    m_rttMin = std::numeric_limits<Timespan>::max();

    for (const auto entry : m_rttHistory) {
        m_rttAvg += entry;
        m_rttMax = std::max(m_rttMax, entry);
        m_rttMin = std::min(m_rttMin, entry);
    }

    m_rttAvg /= m_rttHistory.size();
}

void CongestionControl::NotifySendingBytes(DatagramID outgoing, size_t bytes) {
    m_bytesInFlight += bytes;

    DatagramInFlight tracker;
    tracker.bytes = bytes;
    tracker.sent = Time::Now();
    m_outgoing.emplace(outgoing, tracker);
}

void CongestionControl::NotifyReceivedAck(DatagramID recv) {
    // look for this packet ID in our records
    auto it = std::find_if(m_outgoing.begin(), m_outgoing.end(), [recv](const auto& pair) {
        return pair.first == recv;
    });
    if (it == m_outgoing.end()) return;

    // calculate and record RTT
    Timespan rtt = Time::Between(Time::Now(), it->second.sent);
    m_rttHistory.push_back(rtt);
    if (m_rttHistory.size() > cfg::CONGESTION_RTT_HISTORY_LEN)
        m_rttHistory.pop_front();
    RecalculateRTT();

    m_bytesInFlight -= it->second.bytes;
    m_outgoing.erase(it);
}

CongestionControl::RecvState CongestionControl::NotifyReceivedDatagram(DatagramID recv, bool isAckDatagram) {
    auto it = m_datagramHistory.find(recv);
    if (it != m_datagramHistory.end()) return RecvState::DUPLICATE;

    m_datagramHistory.emplace(recv, Time::Now());

    // if this will be the first new ack we're sending, then this is also immediately the oldest one in the list
    if (m_acks.empty() && m_nacks.empty())
        m_oldestUnsentAck = Time::Now();

    // Detect holes between this packet ID, and the next expected ID. If there's a gap, then
    // those packets must've gotten lost in transit
    const PacketID expected = m_remoteDatagram;
    if (SequenceGreaterThan(recv, expected)) {
        unsigned skipped = recv - expected;
        while (skipped > 0) {
            m_nacks.push_back(recv - skipped);
            skipped--;
        }
    }
    m_remoteDatagram = recv + 1;

    // we do not ack an ack, because that will result in an infinite back and forth conversation of acks
    if (!isAckDatagram)
        m_acks.push_back(recv);

    return RecvState::NEW;
}

CongestionControl::RecvState CongestionControl::NotifyReceivedPacket(PacketID recv) {
    auto it = m_packetHistory.find(recv);
    if (it != m_packetHistory.end()) return RecvState::DUPLICATE;

    m_packetHistory.emplace(recv, Time::Now());
    return RecvState::NEW;
}
