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
    outstream.WriteByte(STATE_KEY_EXCHANGE);
    outstream.WriteBytes(m_crypto.GetEphemeralPublicKey());
    outstream.WriteBool(m_crypto.GetNeedsChallenge());
    m_state = STATE_KEY_EXCHANGE;
}

ConnectResult EncryptionAuthenticator::Handle(BinaryStream& instream, BinaryStream& outstream) {
    // discard auth packets that are currently unexpected (possibly duplicate, or hostile)
    if (instream.ReadByte() != m_state)
        return ConnectResult::IN_PROGRESS;

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
    // read out the remote public key into a buffer
    const auto keylen = cfg::DefaultEncryption::GetKeyLength();
    BinaryStream remoteKey(keylen);
    remoteKey.WriteZeroes(keylen);
    remoteKey.SeekToBegin();
    instream.ReadBytes(remoteKey.GetWritableBuffer(), keylen);

    // pass the buffer to the crypto layer, so the key exchange can be completed
    if (!m_crypto.HandleKeyExchange(m_origin, remoteKey))
        return ConnectResult::INCORRECT_REMOTE_IDENTITY;

    switch (m_origin) {
    case Handshaker::Origin::SELF:
        // server just sent us their part of the key xchg

        // our next message must be sent with crypto! only kx is done unencrypted
        m_crypto.SetCryptoEstablished();

        if (m_crypto.GetNeedsChallenge()) {
            // write an auth challenge to the handshaker stream
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

        // only actually enable crypto AFTER this kx packet goes out, because client needs to have our unencrypted kx key first
        m_enableCryptoAfterReply = true;

        // client indicates whether they also want to send us an auth challenge
        m_state = instream.ReadBool() ? STATE_AUTHENTICATION : STATE_DONE;

        break;
    default:
        assert(false);
        break;
    }

    return ConnectResult::IN_PROGRESS;
}

ConnectResult EncryptionAuthenticator::HandleAuth(BinaryStream& instream, BinaryStream& outstream) {
    if (m_origin == Handshaker::Origin::REMOTE) {
        // incoming challenge, solve it
        outstream.WriteByte(STATE_AUTHENTICATION);
        if (!m_crypto.HandleChallengeIncoming(instream, outstream))
            return ConnectResult::INCOMPATIBLE_SECURITY;

        m_state = STATE_DONE;
        return ConnectResult::IN_PROGRESS;

    } else {
        // challenge response, verify it
        auto result = m_crypto.HandleChallengeResponse(instream);
        if (!result) {
            outstream.Clear();
            return ConnectResult::INCORRECT_REMOTE_IDENTITY;
        }

        // all good! send final ack
        m_state = STATE_DONE;
        outstream.WriteByte(STATE_DONE);
        return ConnectResult::OK;
    }
}
