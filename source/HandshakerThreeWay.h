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
             * \param[in]   myID    The PeerID of the local peer, used for identification purposes.
             * \param[in]   origin  Indicates which endpoint initiated this handshake attempt.
             */
            HandshakerThreeWay(PeerID myID, Origin origin)
                : Handshaker(myID, origin)
                , m_status(NOT_STARTED) {}

            void            Begin(const RemotePeer* remote) override;
            void            Handle(RemotePeer* remote, const Packet& packet) override;

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
        };

        /// \endcond

    }

}
