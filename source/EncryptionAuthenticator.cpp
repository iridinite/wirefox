/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#include "PCH.h"
#include "EncryptionAuthenticator.h"
#include "WirefoxConfigRefs.h"

using namespace detail;

EncryptionAuthenticator::EncryptionAuthenticator(Handshaker::Origin origin, EncryptionLayer& crypto)
    : m_crypto(crypto)
    , m_origin(origin)
    , m_state(STATE_KEY_EXCHANGE)
    , m_enableCryptoAfterReply(false) {}

void EncryptionAuthenticator::Begin(BinaryStream& outstream) {
    std::cout << "Begin STATE_KEY_EXCHANGE" << std::endl;
    outstream.WriteByte(STATE_KEY_EXCHANGE);
    outstream.WriteBytes(m_crypto.GetEphemeralPublicKey());
    outstream.WriteBool(m_crypto.GetNeedsChallenge());
    m_state = STATE_KEY_EXCHANGE;
}

ConnectResult EncryptionAuthenticator::Handle(BinaryStream& instream, BinaryStream& outstream) {
    // discard auth packets that are currently unexpected (possibly duplicate, or hostile)
    if (instream.ReadByte() != m_state) {
        std::cout << "Authenticator state mismatch, origin " << int(m_origin) << std::endl;
        return ConnectResult::IN_PROGRESS;
    }

    switch (m_state) {
    case STATE_KEY_EXCHANGE:
        return HandleKeyExchange(instream, outstream);
    case STATE_AUTHENTICATION:
        return HandleAuth(instream, outstream);
    case STATE_DONE:
        assert(m_origin == Handshaker::Origin::REMOTE);
        return ConnectResult::OK;
    default:
        assert(false && "corrupt AUTH_MSG passed to EncryptionAuthenticator");
        return ConnectResult::INCOMPATIBLE_PROTOCOL;
    }
}

void EncryptionAuthenticator::PostHandle() {
    if (!m_enableCryptoAfterReply) return;

    m_enableCryptoAfterReply = false;
    m_crypto.SetCryptoEstablished();
}

ConnectResult EncryptionAuthenticator::HandleKeyExchange(BinaryStream& instream, BinaryStream& outstream) {
    std::cout << "Handle STATE_KEY_EXCHANGE, origin " << int(m_origin) << std::endl;

    // read out the remote public key into a buffer
    const auto keylen = cfg::DefaultEncryption::GetKeyLength();
    BinaryStream remoteKey(keylen);
    remoteKey.WriteZeroes(keylen);
    remoteKey.SeekToBegin();
    instream.ReadBytes(remoteKey.GetWritableBuffer(), keylen);

    // pass the buffer to the crypto layer, so the key exchange can be completed
    if (!m_crypto.HandleKeyExchange(m_origin, remoteKey))
        return ConnectResult::INCORRECT_REMOTE_IDENTITY;

    m_enableCryptoAfterReply = true;

    switch (m_origin) {
    case Handshaker::Origin::SELF:
        // server just sent us their part of the key xchg, so now move on to auth if that's required
        if (m_crypto.GetNeedsChallenge()) {
            // write an auth challenge to the handshaker stream
            std::cout << "Sending auth challenge" << std::endl;
            outstream.WriteByte(STATE_AUTHENTICATION);
            m_crypto.CreateChallenge(outstream);
            m_state = STATE_AUTHENTICATION;
        } else {
            // no auth required, send final ack
            m_state = STATE_DONE;
            outstream.WriteByte(STATE_DONE);
            return ConnectResult::OK;
        }

        break;
    case Handshaker::Origin::REMOTE:
        // give the client our ephemeral public key, for the key xchg
        outstream.WriteByte(STATE_KEY_EXCHANGE);
        outstream.WriteBytes(m_crypto.GetEphemeralPublicKey());

        // client indicates whether they also want to send us an auth challenge
        if (instream.ReadBool()) {
            std::cout << "Client desires auth challenge" << std::endl;
            m_state = STATE_AUTHENTICATION;
        } else {
            std::cout << "Client needs no auth challenge" << std::endl;
            m_state = STATE_DONE;
            //return ConnectResult::OK;
        }

        break;
    default:
        assert(false);
        break;
    }

    return ConnectResult::IN_PROGRESS;
}

ConnectResult EncryptionAuthenticator::HandleAuth(BinaryStream& instream, BinaryStream& outstream) {
    std::cout << "Handle STATE_AUTHENTICATION, origin " << int(m_origin) << std::endl;

    if (m_origin == Handshaker::Origin::REMOTE) {
        // incoming challenge, solve it
        std::cout << "Incoming auth challenge, solving..." << std::endl;
        outstream.WriteByte(STATE_AUTHENTICATION);
        if (!m_crypto.HandleChallengeIncoming(instream, outstream))
            return ConnectResult::INCOMPATIBLE_SECURITY;

        return ConnectResult::IN_PROGRESS;

    } else {
        // challenge response, verify it
        std::cout << "Incoming auth response, verifying..." << std::endl;
        auto result = m_crypto.HandleChallengeResponse(instream);
        if (!result)
            return ConnectResult::INCORRECT_REMOTE_IDENTITY;

        // all good! send final ack
        m_state = STATE_DONE;
        outstream.WriteByte(STATE_DONE);
        return ConnectResult::OK;
    }
}
