/**
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#pragma once
#include "EncryptionLayer.h"

#ifdef WIREFOX_ENABLE_ENCRYPTION

namespace wirefox {

    namespace detail {

        /**
         * \cond WIREFOX_INTERNAL
         * \brief Represents a cryptography layer implemented using libsodium.
         */
        class EncryptionLayerSodium : public EncryptionLayer {
        public:
            EncryptionLayerSodium();
            EncryptionLayerSodium(const EncryptionLayerSodium&) = delete;
            EncryptionLayerSodium(EncryptionLayerSodium&&) noexcept;
            virtual ~EncryptionLayerSodium();

            EncryptionLayerSodium& operator=(const EncryptionLayerSodium&) = delete;
            EncryptionLayerSodium& operator=(EncryptionLayerSodium&&) noexcept;

            bool GetNeedsToBail() const override;
            size_t GetOverhead() const override;

            BinaryStream GetPublicKey() const override;
            void SetRemotePublicKey(Handshaker::Origin origin, BinaryStream& pubkey) override;

            BinaryStream Encrypt(const BinaryStream& plaintext) override;
            BinaryStream Decrypt(BinaryStream& ciphertext) override;

        private:
            static constexpr size_t KEY_LENGTH = 32;

            /// Key exchange key, secret part.
            uint8_t m_kx_secret[KEY_LENGTH];
            /// Key exchange key, public part.
            uint8_t m_kx_public[KEY_LENGTH];

            /// Session key for decrypting received messages.
            uint8_t m_key_rx[KEY_LENGTH];
            /// Session key for encrypting outgoing messages.
            uint8_t m_key_tx[KEY_LENGTH];

            bool m_error;
        };

        /// \endcond

    }

}

#endif
