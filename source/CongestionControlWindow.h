/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

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

            void            Update(PeerStats& stats) override;

            size_t          GetTransmissionBudget() const override;
            size_t          GetRetransmissionBudget() const override;
            Timespan        GetRetransmissionRTO(unsigned retries) const override;

            bool            GetNeedsToSendAcks() const override;

            void            NotifyReceivedAck(DatagramID recv) override;
            void            NotifyReceivedNakGroup() override;

        private:
            bool            GetIsSlowStart() const;

            size_t                  m_window;
            size_t                  m_threshold;
        };

        /// \endcond

    }

}
