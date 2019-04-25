/**
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#pragma once
#include "BinaryStream.h"
#include "Handshaker.h"

namespace wirefox {

    namespace detail {

        /**
         * \cond WIREFOX_INTERNAL
         * \brief Represents an abstract encryption mechanism.
         * 
         * The purpose of this interface is to provide a wrapper using Wirefox objects around any
         * implementation, and to avoid leaking any and all implementation details.
         * 
         * This interface makes no assumptions about the underlying cryptography, other than that
         * a key exchange takes place before any data is sent.
         */
        class EncryptionLayer {
        public:
            /// Represents an abstract public/private keypair.
            class Keypair {
            protected:
                Keypair() = default;
            };

            /// Default constructor
            EncryptionLayer() = default;
            /// Copy constructor
            EncryptionLayer(const EncryptionLayer&) = delete;
            /// Move constructor
            EncryptionLayer(EncryptionLayer&&) noexcept = default;
            /// Destructor
            virtual ~EncryptionLayer() = default;

            /// Copy assignment operator
            EncryptionLayer& operator=(const EncryptionLayer&) = delete;
            /// Move assignment operator
            EncryptionLayer& operator=(EncryptionLayer&&) noexcept = default;

            /**
             * \brief Returns a value indicating whether an error has occurred.
             * 
             * If a cryptographic operation fails, or if a verification operation fails, or if the
             * implementation believes the connected party is compromised, this function returns true.
             * 
             * This should be tested after calling SetRemotePublicKey(), Encrypt(), or Decrypt().
             */
            virtual bool GetNeedsToBail() const = 0;

            /**
             * \brief Returns a key belonging to this peer for use in key exchange algorithms.
             * 
             * As part of a key exchange, this piece of data should be sent to the other party.
             */
            virtual BinaryStream GetPublicKey() const = 0;

            /**
             * \brief Completes the key exchange using a remote public key.
             * 
             * As part of a key exchange, this function should be called to compute and store session keys
             * before any data is encrypted or decrypted.
             * 
             * \param[in]   origin      Specifies which endpoint initiated the connection.
             * \param[in]   pubkey      The public key of the remote endpoint.
             */
            virtual void SetRemotePublicKey(Handshaker::Origin origin, BinaryStream& pubkey) = 0;

            virtual void SetLocalKeypair(std::shared_ptr<Keypair> keypair) = 0;

            /**
             * \brief Encrypts a piece of data into a ciphertext.
             * 
             * \param[in]   plaintext   The message that should be encrypted.
             * \returns An opaque blob representing the ciphertext. May be longer than \p plaintext, if needed.
             */
            virtual BinaryStream Encrypt(const BinaryStream& plaintext) = 0;

            /**
             * \brief Decrypts a ciphertext into a plaintext.
             * 
             * \param[in]   ciphertext  The encrypted data, as received from the remote party.
             * \returns The decrypted plaintext, on success. On failure, return value is undefined.
             */
            virtual BinaryStream Decrypt(BinaryStream& ciphertext) = 0;
        };

        /// \endcond

    }

}
