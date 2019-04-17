#pragma once
#include "CongestionControl.h"

namespace wirefox {

    namespace detail {

        /**
         * \cond WIREFOX_INTERNAL
         * \brief Represents a congestion avoidance algorithm implemented using a sliding window.
         * 
         * This implementation uses a send window which continually grows as long as ACKs are received.
         * When a NAK is received, the window shrinks, so as to avoid congestion.
         */
        class CongestionControlWindow : public CongestionControl {
        public:
            CongestionControlWindow();
            CongestionControlWindow(const CongestionControlWindow&) = delete;
            CongestionControlWindow(CongestionControlWindow&&) noexcept = delete;
            ~CongestionControlWindow() = default;

            CongestionControlWindow& operator=(const CongestionControlWindow&) = delete;
            CongestionControlWindow& operator=(CongestionControlWindow&&) noexcept = delete;

            void            Update() override;

            PacketID        GetNextPacketID() override;
            DatagramID      GetNextDatagramID() override;
            DatagramID      PeekNextDatagramID() const override;

            size_t          GetTransmissionBudget() const override;
            size_t          GetRetransmissionBudget() const override;
            Timespan        GetRetransmissionRTO(unsigned retries) const override;
            bool            GetRTTHistoryAvailable() const override;
            unsigned        GetAverageRTT() const override;

            bool            GetNeedsToSendAcks() const override;
            void            MakeAckList(std::vector<DatagramID>& acks, std::vector<DatagramID>& nacks) override;

            /**
             * \brief Recalculates the average round-trip-time using the current RTT history buffer.
             */
            void            RecalculateRTT();

            void            NotifySendingBytes(DatagramID outgoing, size_t bytes) override;
            void            NotifyReceivedAck(DatagramID recv) override;
            void            NotifyReceivedNakGroup() override;
            RecvState       NotifyReceivedDatagram(DatagramID recv, bool isAckDatagram) override;
            RecvState       NotifyReceivedPacket(PacketID recv) override;

        private:
            bool            GetIsSlowStart() const;

            struct DatagramInFlight {
                size_t bytes;
                Timestamp sent;
            };

            PacketID                m_nextPacket;
            DatagramID              m_nextDatagram;
            DatagramID              m_remoteDatagram;
            Timestamp               m_nextUpdate;
            Timestamp               m_oldestUnsentAck;
            size_t                  m_window;
            size_t                  m_threshold;
            size_t                  m_bytesInFlight;

            std::list<Timespan>     m_rttHistory;
            Timespan                m_rttMin, m_rttMax, m_rttAvg;

            std::vector<DatagramID> m_acks;
            std::vector<DatagramID> m_nacks;
            std::map<DatagramID, DatagramInFlight> m_outgoing;
            std::map<DatagramID, Timestamp> m_datagramHistory;
            std::map<PacketID, Timestamp> m_packetHistory;
        };

        /// \endcond

    }

}
