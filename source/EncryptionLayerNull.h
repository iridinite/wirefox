/**
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#pragma once
#include "EncryptionLayer.h"

namespace wirefox {

    namespace detail {

        /**
         * \cond WIREFOX_INTERNAL
         * \brief Represents a cryptography layer that returns messages unchanged.
         */
        class EncryptionLayerNull : public EncryptionLayer {
        public:
            EncryptionLayerNull() = default;
            EncryptionLayerNull(const EncryptionLayerNull&) = delete;
            EncryptionLayerNull(EncryptionLayerNull&&) noexcept = default;
            virtual ~EncryptionLayerNull() = default;

            EncryptionLayerNull& operator=(const EncryptionLayerNull&) = delete;
            EncryptionLayerNull& operator=(EncryptionLayerNull&&) noexcept = default;

            bool GetNeedsToBail() const override { return false; };

            BinaryStream GetPublicKey() const override { return BinaryStream(0); }
            void SetRemotePublicKey(Handshaker::Origin, BinaryStream&) override {}

            BinaryStream Encrypt(const BinaryStream& plaintext) override { return plaintext; }
            BinaryStream Decrypt(BinaryStream& ciphertext) override { return ciphertext; }
            void SetLocalKeypair(std::shared_ptr<Keypair>) override {}

            /**
             * \brief Returns the maximum amount of overhead added to a plaintext, in bytes.
             */
            static size_t GetOverhead() { return 0; }

            /**
             * \brief Returns the length of the private or public key, in bytes.
             */
            static size_t GetKeyLength() { return 0; }
        };

        /// \endcond

    }

}