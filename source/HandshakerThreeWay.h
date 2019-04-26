/**
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#pragma once
#include "Handshaker.h"

namespace wirefox {

    namespace detail {

        /**
         * \cond WIREFOX_INTERNAL
         * \brief Represents a three-way handshake algorithm.
         * 
         * Works exactly like a TCP handshake. The initiating party sends the first message (request), the
         * remote answers with an acknowledgement, and the initiating party answers that with another ack.
         */
        class HandshakerThreeWay : public Handshaker {
        public:
            /**
             * \brief Constructs a new HandshakerThreeWay instance.
             * 
             * \param[in]   master  The owner of \p remote. Used for checking for duplicate PeerIDs.
             * \param[in]   remote  The RemotePeer controlled by this Handshaker. Some fields are filled in during handshake.
             * \param[in]   origin  Indicates which endpoint initiated this handshake attempt.
             */
            HandshakerThreeWay(Peer* master, RemotePeer* remote, Origin origin)
                : Handshaker(master, remote, origin)
                , m_status(NOT_STARTED)
                , m_keySetupDone(false) {}

            void            Begin() override;
            void            Handle(const Packet& packet) override;

            /**
             * \brief Send an error response without an associated RemotePeer or Handshaker instance.
             * 
             * \param[in]   outstream   The BinaryStream to write the formatted response to.
             * \param[in]   myID        The local peer's PeerID, for identification purposes.
             * \param[in]   reply       The error condition to send to the remote endpoint.
             */
            static void     WriteOutOfBandErrorReply(BinaryStream& outstream, PeerID myID, ConnectResult reply);

        private:
            static void     WriteReplyHeader(BinaryStream& outstream, PeerID myID);
            void            ReplyWithError(BinaryStream& outstream, ConnectResult problem);

            enum {
                NOT_STARTED,
                AWAITING_ACK
            }               m_status;
            bool            m_keySetupDone;
        };

        /// \endcond

    }

}
