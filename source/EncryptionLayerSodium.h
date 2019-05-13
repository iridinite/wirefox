/*
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
            static constexpr size_t KEY_LENGTH = 32;
            static constexpr size_t CHALLENGE_LENGTH = 64;

        public:
            /// Represents a public/private keypair suitable for use with libsodium's key exchange.
            class Keypair : public EncryptionLayer::Keypair {
                friend class EncryptionLayerSodium;
            public:
                /// Default constructor, initializes a zero keypair.
                Keypair();
                /// Constructor that copies preset keys.
                Keypair(const uint8_t* key_secret, const uint8_t* key_public);
                /// Destructor, securely erases the keypair from memory.
                ~Keypair();

                /// Create new keypair meant for persistent identities (e.g. signing authentication challenges).
                static std::shared_ptr<Keypair> CreateIdentity();
                /// Create new ephemeral keypair meant for the key exchange. These keys should not be stored.
                static std::shared_ptr<Keypair> CreateKeyExchange();

                /**
                 * \brief Copies the keys from this Keypair into user-specified buffers.
                 * 
                 * \param[out]  out_secret  A buffer where the secret key will be written to.
                 * \param[out]  out_public  A buffer where the public key will be written to.
                 */
                void CopyTo(uint8_t* out_secret, uint8_t* out_public) const;

            private:
                uint8_t key_public[KEY_LENGTH];
                uint8_t key_secret[KEY_LENGTH];
            };

            /// Default constructor
            EncryptionLayerSodium();
            /// Copy constructor, disabled for security
            EncryptionLayerSodium(const EncryptionLayerSodium&) = delete;
            /// Move constructor
            EncryptionLayerSodium(EncryptionLayerSodium&&) noexcept;
            /// Destructor
            virtual ~EncryptionLayerSodium();

            /// Copy assignment operator, disabled for security
            EncryptionLayerSodium& operator=(const EncryptionLayerSodium&) = delete;
            /// Move assignment operator
            EncryptionLayerSodium& operator=(EncryptionLayerSodium&&) noexcept;

            bool GetNeedsToBail() const override;

            BinaryStream GetEphemeralPublicKey() const override;

            void SetCryptoEstablished() override;
            bool GetCryptoEstablished() const override;
            bool GetNeedsChallenge() const override;
            void CreateChallenge(BinaryStream& outstream) override;
            bool HandleKeyExchange(Handshaker::Origin origin, BinaryStream& pubkey) override;
            bool HandleChallengeResponse(BinaryStream& instream) override;
            bool HandleChallengeIncoming(BinaryStream& instream, BinaryStream& answer) override;
            /// \copydoc wirefox::detail::EncryptionLayer::SetLocalIdentity
            void SetLocalIdentity(std::shared_ptr<EncryptionLayer::Keypair> keypair) override;
            void ExpectRemoteIdentity(BinaryStream& pubkey) override;

            BinaryStream Encrypt(const BinaryStream& plaintext) override;
            BinaryStream Decrypt(BinaryStream& ciphertext) override;

            /**
             * \brief Returns the maximum amount of overhead added to a plaintext, in bytes.
             */
            static size_t GetOverhead();

            /**
             * \brief Returns the length of the private or public key, in bytes.
             */
            static size_t GetKeyLength();

        private:
            /// A handle to the local peer's keypair.
            std::shared_ptr<Keypair> m_identity;
            /// A handle to the ephemeral keypair used for the key exchange.
            std::shared_ptr<Keypair> m_kx;

            /// The remote public key, if known beforehand.
            uint8_t m_remote_identity_pk[KEY_LENGTH];

            /// The challenge that was issued to the remote host, if any.
            uint8_t m_issued_challenge[CHALLENGE_LENGTH];

            /// Session key for decrypting received messages.
            uint8_t m_key_rx[KEY_LENGTH];
            /// Session key for encrypting outgoing messages.
            uint8_t m_key_tx[KEY_LENGTH];

            bool m_error;
            bool m_established;
            bool m_remoteIdentityKnown;
            bool m_remoteAuthExpected;
        };

        /// \endcond

    }

}

#endif
