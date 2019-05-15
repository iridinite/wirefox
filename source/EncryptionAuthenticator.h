/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#pragma once
#include "BinaryStream.h"
#include "EncryptionLayer.h"

namespace wirefox {

    namespace detail {

        /**
         * \brief Represents a helper class to be used from a Handshaker, that performs crypto setup.
         * 
         * Once the basic handshake is complete, this object can be used to send key exchange and
         * cryptographic authentication packets to a remote party.
         */
        class EncryptionAuthenticator {
        public:
            /**
             * \brief Construct a new instance of EncryptionAuthenticator.
             * \param[in]   origin      Specifies which party initiated this connection.
             * \param[in]   crypto      The EncryptionLayer instance that must be configured.
             */
            EncryptionAuthenticator(ConnectionOrigin origin, EncryptionLayer& crypto);
            EncryptionAuthenticator(const EncryptionAuthenticator&) = delete;
            EncryptionAuthenticator(EncryptionAuthenticator&&) = delete;
            ~EncryptionAuthenticator() = default;

            EncryptionAuthenticator& operator=(const EncryptionAuthenticator&) = delete;
            EncryptionAuthenticator& operator=(EncryptionAuthenticator&&) = delete;

            /**
             * \brief Initiates the cryptographic handshake.
             * 
             * Meant to be used by the initiating peer (ConnectionOrigin::SELF) after the basic handshake
             * is complete. This function will initiate the key exchange.
             * 
             * \param[out]  outstream   Stream to be sent to the remote endpoint.
             */
            void Begin(BinaryStream& outstream);

            /**
             * \brief Handles an incoming message from a remote EncryptionAuthenticator.
             * 
             * \param[in]   instream    Stream that contains the message written by the remote endpoint.
             * \param[out]  outstream   Response to be sent to the remote endpoint.
             */
            ConnectResult Handle(BinaryStream& instream, BinaryStream& outstream);

            /**
             * \brief Finalizes handling of a message.
             * 
             * This should be called after Handle(), and the response written by it has been queued for sending.
             */
            void PostHandle();

        private:
            ConnectResult HandleKeyExchange(BinaryStream& instream, BinaryStream& outstream);
            ConnectResult HandleAuth(BinaryStream& instream, BinaryStream& outstream);

            EncryptionLayer& m_crypto;
            ConnectionOrigin m_origin;

            enum {
                STATE_KEY_EXCHANGE,
                STATE_AUTHENTICATION,
                STATE_DONE
            } m_state;

            bool m_enableCryptoAfterReply;
        };

    }

}
