/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#include "PCH.h"
#include "CongestionControlWindow.h"

using namespace detail;

CongestionControlWindow::CongestionControlWindow()
    : m_window(cfg::MTU)
    , m_threshold(cfg::CONGESTION_WINDOW_SSTHRESH) {}

void CongestionControlWindow::Update(PeerStats& stats) {
    // just here to update the CWND stat, mostly for debugging purposes
    CongestionControl::Update(stats);
    stats.Set(PeerStatID::CWND, m_window);
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

    // a conservative little measure so a new connection won't have a silly RTT of zero
    if (!GetRTTHistoryAvailable())
        return Time::FromMilliseconds(100) + baseDelay;

    Timespan variance = m_rttMax - m_rttMin;
    Timespan rto = 2 * m_rttAvg + 4 * variance + baseDelay;

    return rto * (retries + 1);
}

bool CongestionControlWindow::GetNeedsToSendAcks() const {
    if (m_acks.empty() && m_nacks.empty()) return false;

    Timespan delay = Time::FromMilliseconds(10);

    return (m_acks.size() + m_nacks.size() > 10) || // has a whole bunch of acks to send?
        Time::Elapsed(m_oldestUnsentAck + delay); // has waited a little bit at least?
}

void CongestionControlWindow::NotifyReceivedAck(DatagramID recv) {
    CongestionControl::NotifyReceivedAck(recv);

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

bool CongestionControlWindow::GetIsSlowStart() const {
    return m_window <= m_threshold;
}
