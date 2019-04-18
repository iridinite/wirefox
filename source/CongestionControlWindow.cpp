#include "PCH.h"
#include "CongestionControlWindow.h"

using namespace detail;

CongestionControlWindow::CongestionControlWindow()
    : m_nextPacket(0)
    , m_nextDatagram(0)
    , m_remoteDatagram(0)
    , m_nextUpdate(Time::Now())
    , m_window(cfg::MTU)
    , m_threshold(cfg::CONGESTION_WINDOW_SSTHRESH)
    , m_bytesInFlight(0)
    , m_rttMin(0)
    , m_rttMax(0)
    , m_rttAvg(0) {}

void CongestionControlWindow::Update() {
    // Many calls in rapid succession aren't necessary; this might save us some time wasted doing
    // list traversals if we already cleaned them up 1 ms ago
    if (!Time::Elapsed(m_nextUpdate)) return;
    m_nextUpdate = Time::Now() + Time::FromMilliseconds(20);

    // Traverse outgoing entries, and discard those that are so old they're probably irrelevant.
    // This ensures that even un-acked packets (ack never sent, or got lost) will be removed.
#if _DEBUG
    size_t bytesStillOutgoing = 0;
#endif
    auto decayTime = m_rttAvg * 16; // somewhat arbitrary long time
    auto itr_flight = m_outgoing.begin();
    while (itr_flight != m_outgoing.end()) {
        const auto& pif = itr_flight->second;
        if (Time::Elapsed(pif.sent + decayTime)) {
            m_bytesInFlight -= pif.bytes;
            itr_flight = m_outgoing.erase(itr_flight);
        } else {
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

    // prefer caching this over Elapsed(), because Elapsed() wraps Now(), and getting clock is somewhat expensive
    auto expire = Time::Now() + Time::FromSeconds(10);

    for (auto itr_datagram = m_datagramHistory.begin(), itend = m_datagramHistory.end(); itr_datagram != itend;) {
        if (expire >= itr_datagram->second)
            itr_datagram = m_datagramHistory.erase(itr_datagram);
        else
            ++itr_datagram;
    }
    for (auto itr_packet = m_packetHistory.begin(), itend = m_packetHistory.end(); itr_packet != itend;) {
        if (expire >= itr_packet->second)
            itr_packet = m_packetHistory.erase(itr_packet);
        else
            ++itr_packet;
    }
}

PacketID CongestionControlWindow::GetNextPacketID() {
    return m_nextPacket++;
}

DatagramID CongestionControlWindow::GetNextDatagramID() {
    return m_nextDatagram++;
}

DatagramID CongestionControlWindow::PeekNextDatagramID() const {
    return m_nextDatagram;
}

size_t CongestionControlWindow::GetTransmissionBudget() const {
    return std::min(
        m_bytesInFlight >= m_window ? 0 : m_window - m_bytesInFlight,
        cfg::MTU - GetRetransmissionBudget());
}

size_t CongestionControlWindow::GetRetransmissionBudget() const {
    return std::min(
        m_bytesInFlight,
        cfg::MTU);
}

Timespan CongestionControlWindow::GetRetransmissionRTO(unsigned retries) const {
    constexpr Timespan baseDelay = Time::FromMilliseconds(cfg::THREAD_SLEEP_PACKETQUEUE_TICK);

    // a little measure so a new connection won't have a silly RTT of zero
    if (GetRTTHistoryAvailable())
        return baseDelay * 4;

    Timespan variance = m_rttMax - m_rttMin;
    Timespan rto = m_rttAvg + (2 * variance) + baseDelay;

    return rto * (retries + 1);
}

bool CongestionControlWindow::GetRTTHistoryAvailable() const {
    return m_rttHistory.size() >= 2; // a few samples
}

unsigned CongestionControlWindow::GetAverageRTT() const {
    auto ms = Time::ToMilliseconds(m_rttAvg);
    assert(ms >= 0 && "RTT going back in time would be rather impressive");

    return static_cast<unsigned>(ms);
}

bool CongestionControlWindow::GetNeedsToSendAcks() const {
    if (m_acks.empty() && m_nacks.empty()) return false;

    Timespan delay = Time::FromMilliseconds(10);

    return (m_acks.size() + m_nacks.size() > 10) || // has a whole bunch of acks to send?
        Time::Elapsed(m_oldestUnsentAck + delay); // has waited a little bit at least?
}

void CongestionControlWindow::MakeAckList(std::vector<DatagramID>& acks, std::vector<DatagramID>& nacks) {
    assert(acks.empty());
    assert(nacks.empty());
    acks.swap(m_acks);
    nacks.swap(m_nacks);
}

void CongestionControlWindow::RecalculateRTT() {
    m_rttAvg = 0;
    m_rttMax = 0;
    m_rttMin = std::numeric_limits<Timespan>::max();

    for (const auto entry : m_rttHistory) {
        m_rttAvg += entry;
        m_rttMax = std::max(m_rttMax, entry);
        m_rttMin = std::min(m_rttMin, entry);
    }

    m_rttAvg /= m_rttHistory.size();

    //std::string msg = "CongestionControl: RTT: Avg " + std::to_string(m_rttAvg) + " ns, Min " + std::to_string(m_rttMin) + " ns, Max " + std::to_string(m_rttMax) + " ns";
    //std::cout << msg << std::endl;
}

void CongestionControlWindow::NotifySendingBytes(DatagramID outgoing, size_t bytes) {
    m_bytesInFlight += bytes;

    DatagramInFlight tracker;
    tracker.bytes = bytes;
    tracker.sent = Time::Now();
    m_outgoing.emplace(outgoing, tracker);
}

void CongestionControlWindow::NotifyReceivedAck(DatagramID recv) {
    // look for this packet ID in our records
    auto it = std::find_if(m_outgoing.begin(), m_outgoing.end(), [recv](const auto& pair) {
        return pair.first == recv;
    });
    if (it != m_outgoing.end()) {
        // calculate and record RTT
        Timespan rtt = Time::Between(Time::Now(), it->second.sent);
        m_rttHistory.push_back(rtt);
        if (m_rttHistory.size() > cfg::CONGESTION_RTT_HISTORY_LEN)
            m_rttHistory.pop_front();
        RecalculateRTT();

        m_bytesInFlight -= it->second.bytes;
        m_outgoing.erase(it);
    }

    // increase congestion window size
    if (GetIsSlowStart())
        m_window += cfg::MTU;
    else
        m_window += ((cfg::MTU * cfg::MTU) / m_window) + cfg::MTU / 8;
}

void CongestionControlWindow::NotifyReceivedNakGroup() {
    // should bring us back to slow start phase
    m_threshold = std::max(m_window / 2, cfg::MTU * 2);
    m_window = cfg::MTU;
}

CongestionControl::RecvState CongestionControlWindow::NotifyReceivedDatagram(DatagramID recv, bool isAckDatagram) {
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

CongestionControl::RecvState CongestionControlWindow::NotifyReceivedPacket(PacketID recv) {
    auto it = m_packetHistory.find(recv);
    if (it != m_packetHistory.end()) return RecvState::DUPLICATE;

    m_packetHistory.emplace(recv, Time::Now());
    return RecvState::NEW;
}

bool CongestionControlWindow::GetIsSlowStart() const {
    return m_window <= m_threshold;
}
