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

        class EncryptionAuthenticator {
        public:
            EncryptionAuthenticator(Handshaker::Origin origin, EncryptionLayer& crypto);
            EncryptionAuthenticator(const EncryptionAuthenticator&) = delete;
            EncryptionAuthenticator(EncryptionAuthenticator&&) = delete;
            ~EncryptionAuthenticator() = default;

            EncryptionAuthenticator& operator=(const EncryptionAuthenticator&) = delete;
            EncryptionAuthenticator& operator=(EncryptionAuthenticator&&) = delete;

            void Begin(BinaryStream& outstream);
            ConnectResult Handle(BinaryStream& instream, BinaryStream& outstream);
            void PostHandle();

        private:
            ConnectResult HandleKeyExchange(BinaryStream& instream, BinaryStream& outstream);
            ConnectResult HandleAuth(BinaryStream& instream, BinaryStream& outstream);

            EncryptionLayer& m_crypto;
            Handshaker::Origin m_origin;

            enum {
                STATE_KEY_EXCHANGE,
                STATE_AUTHENTICATION,
                STATE_DONE
            } m_state;

            bool m_enableCryptoAfterReply;
        };

    }

}
