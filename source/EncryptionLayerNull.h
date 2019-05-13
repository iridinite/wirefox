/*
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
            /// Default constructor
            EncryptionLayerNull() = default;
            /// Copy constructor
            EncryptionLayerNull(const EncryptionLayerNull&) = delete;
            /// Move constructor
            EncryptionLayerNull(EncryptionLayerNull&&) noexcept = default;
            /// Default destructor
            virtual ~EncryptionLayerNull() = default;

            /// Copy assignment operator
            EncryptionLayerNull& operator=(const EncryptionLayerNull&) = delete;
            /// Move assignment operator
            EncryptionLayerNull& operator=(EncryptionLayerNull&&) noexcept = default;

            bool GetNeedsToBail() const override { return false; };

            BinaryStream GetEphemeralPublicKey() const override { return BinaryStream(0); }
            bool HandleKeyExchange(Handshaker::Origin, BinaryStream&) override { return true; }
            void ExpectRemoteIdentity(BinaryStream&) override {}

            void SetCryptoEstablished() override {}
            bool GetCryptoEstablished() const override { return false; }
            bool GetNeedsChallenge() const override { return false; }
            void CreateChallenge(BinaryStream&) override {}
            bool HandleChallengeIncoming(BinaryStream&, BinaryStream&) override { return true; }
            bool HandleChallengeResponse(BinaryStream&) override { return true; }

            BinaryStream Encrypt(const BinaryStream& plaintext) override { return plaintext; }
            BinaryStream Decrypt(BinaryStream& ciphertext) override { return ciphertext; }
            void SetLocalIdentity(std::shared_ptr<Keypair>) override {}

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
