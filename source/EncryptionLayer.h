/*
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
             * This should be tested after calling HandleKeyExchange(), Encrypt(), or Decrypt().
             */
            virtual bool GetNeedsToBail() const = 0;

            /**
             * \brief Returns a key belonging to this peer for use in key exchange algorithms.
             * 
             * As part of a key exchange, this piece of data should be sent to the other party.
             */
            virtual BinaryStream GetEphemeralPublicKey() const = 0;

            /**
             * \brief Sets a flag indicating encryption should be used henceforth.
             * 
             * Meant to be used after the key exchange is complete, and the unencrypted key exchange packets
             * have been correctly sent out.
             */
            virtual void SetCryptoEstablished() = 0;

            /**
             * \brief Returns a boolean indicating whether encryption should be used.
             * \sa SetCryptoEnabled()
             */
            virtual bool GetCryptoEstablished() const = 0;

            /**
             * \brief Returns a boolean indicating whether the remote endpoint needs to be authenticated.
             * 
             * This is the case when the remote endpoint's public key is explicitly known, and thus
             * a challenge is required as proof of identity.
             */
            virtual bool GetNeedsChallenge() const = 0;

            /**
             * \brief Prepares an authentication challenge.
             * 
             * An authentication challenge is written to the output stream (its length is undefined),
             * and the correct answer will be stored in this EncryptionLayer's state.
             * 
             * \param[out]  outstream   A stream that will be sent to the remote party.
             */
            virtual void CreateChallenge(BinaryStream& outstream) = 0;

            /**
             * \brief Solves an incoming authentication challenge.
             * 
             * \returns A boolean indicating success. If false, the challenge was not meant for us
             * or is otherwise corrupt. \p answer is only modified if this method returns true.
             * 
             * \param[in]   instream    A stream containing a challenge, as written by CreateChallenge().
             * \param[out]  answer      A stream where our answer will be written.
             */
            virtual bool HandleChallengeIncoming(BinaryStream& instream, BinaryStream& answer) = 0;

            /**
             * \brief Verifies an incoming answer to our challenge.
             * 
             * \returns A boolean indicating success. If true, the answer was correct. If false, something
             * was wrong, and the identity of the remote endpoint cannot be verified.
             * 
             * \param[in]   instream    A stream containing the answer, as written by HandleChallengeIncoming().
             */
            virtual bool HandleChallengeResponse(BinaryStream& instream) = 0;

            /**
             * \brief Completes the key exchange using a remote public key.
             * 
             * As part of a key exchange, this function should be called to compute and store session keys
             * before any data is encrypted or decrypted.
             * 
             * \param[in]   origin      Specifies which endpoint initiated the connection.
             * \param[in]   pubkey      The public key of the remote endpoint.
             * 
             * \returns False if the key is incorrect/suspicious, e.g. if it mismatches the key set by
             * ExpectRemoteIdentity(). Otherwise, returns true.
             */
            virtual bool HandleKeyExchange(Handshaker::Origin origin, BinaryStream& pubkey) = 0;

            /**
             * \brief Sets the keypair that represents the local peer.
             * 
             * \param[in]   keypair     A reference to the keypair shared by all remotes on this local peer.
             */
            virtual void SetLocalIdentity(std::shared_ptr<Keypair> keypair) = 0;

            /**
             * \brief Informs the crypto layer that this specific public key is expected.
             * 
             * This is meant to be used for dedicated server scenarios, where a secure, authorative server can
             * have a keypair whose public key ships with the client software.
             * 
             * \param[in]   pubkey      A stream that contains the remote endpoint's public key.
             */
            virtual void ExpectRemoteIdentity(BinaryStream& pubkey) = 0;

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
