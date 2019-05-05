/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#include "PCH.h"
#include "EncryptionLayerSodium.h"

#ifdef WIREFOX_ENABLE_ENCRYPTION
#define WIREFOX_SODIUM_MONITORING 1
#include <sodium.h>

static constexpr size_t WIREFOX_SODIUM_NONCE_LEN = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
static constexpr size_t WIREFOX_SODIUM_MAC_LEN = crypto_secretbox_xchacha20poly1305_MACBYTES;

using namespace detail;

namespace {

    BinaryStream PreallocateBuffer(const size_t len) {
        BinaryStream buffer(len);
        buffer.WriteZeroes(len); // advance length counter
        buffer.SeekToBegin();
        return buffer;
    }

#ifdef WIREFOX_SODIUM_MONITORING
    void PrintKeyToStdout(const char* prefix, uint8_t* key) {
        std::cout << prefix << std::hex;
        for (int i = 0; i < 32; i++)
            std::cout << int(key[i]);

        std::cout << std::dec << std::endl;
    }
#endif

}

std::shared_ptr<EncryptionLayerSodium::Keypair> EncryptionLayerSodium::Keypair::CreateIdentity() {
    // create a keypair that represents a persistent identity (e.g. to be used for signing challenges)
    auto ret = std::make_shared<Keypair>();
    crypto_box_keypair(ret->key_public, ret->key_secret);
    return ret;
}

std::shared_ptr<EncryptionLayerSodium::Keypair> EncryptionLayerSodium::Keypair::CreateKeyExchange() {
    // create an ephemeral keypair that is to be used for ECDH key exchange, and has no meaning beyond that
    auto ret = std::make_shared<Keypair>();
    crypto_kx_keypair(ret->key_public, ret->key_secret);
    return ret;
}

void EncryptionLayerSodium::Keypair::CopyTo(uint8_t* out_secret, uint8_t* out_public) const {
    memcpy(out_secret, key_secret, KEY_LENGTH);
    memcpy(out_public, key_public, KEY_LENGTH);
}

EncryptionLayerSodium::Keypair::Keypair()
    : key_public{}
    , key_secret{} {
    if (sodium_init() < 0)
        throw std::runtime_error("libsodium failed to init");

    // the secure variants are not required here because the uninitialized memory is irrelevant garbage for us,
    // so I don't see the need to call a slower, more complex memzero if we're overwriting it anyway
    memset(key_public, 0, KEY_LENGTH);
    memset(key_secret, 0, KEY_LENGTH);
}

EncryptionLayerSodium::Keypair::Keypair(const uint8_t* key_secret, const uint8_t* key_public)
    : key_public{}
    , key_secret{} {
    memcpy(this->key_secret, key_secret, KEY_LENGTH);
    memcpy(this->key_public, key_public, KEY_LENGTH);
}

EncryptionLayerSodium::Keypair::~Keypair() {
    sodium_memzero(key_public, KEY_LENGTH);
    sodium_memzero(key_secret, KEY_LENGTH);
}

EncryptionLayerSodium::EncryptionLayerSodium()
    : m_remote_identity_pk{}
    , m_issued_challenge{}
    , m_key_rx{}
    , m_key_tx{}
    , m_error(false)
    , m_established(false)
    , m_remoteIdentityKnown(false)
    , m_remoteAuthExpected(false) {
    // asserts done here to make sure the key array sizes match up with what libsodium expects;
    // I don't want to include sodium.h publically so can't use the macros there directly
    static_assert(crypto_kx_PUBLICKEYBYTES == KEY_LENGTH, "kx public key size must match KEY_LENGTH");
    static_assert(crypto_kx_SECRETKEYBYTES == KEY_LENGTH, "kx private key size must match KEY_LENGTH");
    static_assert(crypto_kx_SESSIONKEYBYTES == KEY_LENGTH, "kx session key size must match KEY_LENGTH");
    static_assert(crypto_box_PUBLICKEYBYTES == KEY_LENGTH, "identity public key size must match KEY_LENGTH");
    static_assert(crypto_box_SECRETKEYBYTES == KEY_LENGTH, "identity private key size must match KEY_LENGTH");

    if (sodium_init() < 0)
        throw std::runtime_error("libsodium failed to init");

    m_kx = Keypair::CreateKeyExchange();
}

EncryptionLayerSodium::EncryptionLayerSodium(EncryptionLayerSodium&& other) noexcept
    : EncryptionLayerSodium() {
    *this = std::move(other);
}

EncryptionLayerSodium::~EncryptionLayerSodium() {
    // securely erase all keys from memory
    sodium_memzero(m_remote_identity_pk, KEY_LENGTH);
    sodium_memzero(m_key_tx, KEY_LENGTH);
    sodium_memzero(m_key_tx, KEY_LENGTH);
}

EncryptionLayerSodium& EncryptionLayerSodium::operator=(EncryptionLayerSodium&& other) noexcept {
    if (this != &other) {
        m_kx = std::move(other.m_kx);
        m_identity = std::move(other.m_identity);

        memcpy(m_remote_identity_pk, other.m_remote_identity_pk, sizeof m_remote_identity_pk);
        memcpy(m_key_rx, other.m_key_rx, sizeof m_key_rx);
        memcpy(m_key_tx, other.m_key_tx, sizeof m_key_tx);

        sodium_memzero(other.m_remote_identity_pk, sizeof m_remote_identity_pk);
        sodium_memzero(other.m_key_rx, sizeof m_key_rx);
        sodium_memzero(other.m_key_tx, sizeof m_key_tx);

        m_error = other.m_error;
        m_established = other.m_established;
        m_remoteIdentityKnown = other.m_remoteIdentityKnown;
        m_remoteAuthExpected = other.m_remoteAuthExpected;
        other.m_error = false;
        other.m_established = false;
        other.m_remoteIdentityKnown = false;
        other.m_remoteAuthExpected = false;
    }
    return *this;
}

bool EncryptionLayerSodium::GetNeedsToBail() const {
    return m_error;
}

size_t EncryptionLayerSodium::GetOverhead() {
    return WIREFOX_SODIUM_NONCE_LEN + WIREFOX_SODIUM_MAC_LEN;
}

size_t EncryptionLayerSodium::GetKeyLength() {
    return KEY_LENGTH;
}

BinaryStream EncryptionLayerSodium::GetEphemeralPublicKey() const {
#ifdef WIREFOX_SODIUM_MONITORING
    PrintKeyToStdout("kxpk = ", m_kx->key_public);
#endif
    BinaryStream ret(KEY_LENGTH);
    ret.WriteBytes(m_kx->key_public, KEY_LENGTH);
    ret.SeekToBegin();
    return ret;
}

void EncryptionLayerSodium::SetCryptoEstablished() {
    m_established = true;

#ifdef WIREFOX_SODIUM_MONITORING
    std::cout << "Key exchange complete, encryption enabled" << std::endl;
#endif
}

bool EncryptionLayerSodium::GetCryptoEstablished() const {
    return m_established;
}

bool EncryptionLayerSodium::GetNeedsChallenge() const {
    return m_remoteIdentityKnown;
}

void EncryptionLayerSodium::CreateChallenge(BinaryStream& outstream) {
    // state flag to remember we want an answer to this challenge
    m_remoteAuthExpected = true;

    // random nonce
    //uint8_t nonce[WIREFOX_SODIUM_NONCE_LEN];
    //randombytes_buf(nonce, sizeof nonce);

    // just generate some random bytes to be used as challenge
    uint8_t encrypted[CHALLENGE_LENGTH + crypto_box_SEALBYTES];
    randombytes_buf(m_issued_challenge, CHALLENGE_LENGTH);
    //crypto_aead_xchacha20poly1305_ietf_encrypt(encrypted, m_issued_challenge, CHALLENGE_LENGTH, nullptr, 0, nullptr, nonce, )
    //crypto_sign_detached(encrypted, nullptr, m_issued_challenge, CHALLENGE_LENGTH, m_identity->key_secret);
    crypto_box_seal(encrypted, m_issued_challenge, CHALLENGE_LENGTH, m_remote_identity_pk);

#ifdef WIREFOX_SODIUM_MONITORING
    PrintKeyToStdout("original challenge = ", m_issued_challenge);
    PrintKeyToStdout("encrypt  challenge = ", encrypted);
#endif

    outstream.WriteBytes(encrypted, sizeof encrypted);
}

bool EncryptionLayerSodium::HandleKeyExchange(Handshaker::Origin origin, BinaryStream& pubkey) {
    uint8_t remotekey[KEY_LENGTH];
    pubkey.ReadBytes(remotekey, KEY_LENGTH);

#ifdef WIREFOX_SODIUM_MONITORING
    PrintKeyToStdout("pk = ", remotekey);
#endif

    // verify identity of remote, if we know the key already
    // EDIT: cannot do this here anymore because KX public key has nothing to do with remote identity
    //if (m_remoteIdentityKnown && sodium_memcmp(remotekey, m_remote_identity_pk, KEY_LENGTH) != 0) {
    //    // mismatch, remote may be an impostor
    //    return false;
    //}

    switch (origin) {
    case Handshaker::Origin::SELF:
        if (crypto_kx_client_session_keys(m_key_rx, m_key_tx, m_kx->key_public, m_kx->key_secret, remotekey) != 0)
            m_error = true;
        break;
    case Handshaker::Origin::REMOTE:
        if (crypto_kx_server_session_keys(m_key_rx, m_key_tx, m_kx->key_public, m_kx->key_secret, remotekey) != 0)
            m_error = true;
        break;
    default:
        assert(false && "invalid Handshaker::Origin in EncryptionLayerSodium::HandleKeyExchange");
        break;
    }

#ifdef WIREFOX_SODIUM_MONITORING
    PrintKeyToStdout("rx = ", m_key_rx);
    PrintKeyToStdout("tx = ", m_key_tx);
#endif

    return !m_error;
}

bool EncryptionLayerSodium::HandleChallengeResponse(BinaryStream& instream) {
    uint8_t response[CHALLENGE_LENGTH];
    instream.ReadBytes(response, sizeof response);

    return sodium_memcmp(response, m_issued_challenge, CHALLENGE_LENGTH) == 0;
}

bool EncryptionLayerSodium::HandleChallengeIncoming(BinaryStream& instream, BinaryStream& answer) {
    uint8_t encrypted[CHALLENGE_LENGTH + crypto_box_SEALBYTES];
    uint8_t decrypted[CHALLENGE_LENGTH];
    instream.ReadBytes(encrypted, sizeof encrypted);

    // decrypt the challenge using our identity's private key; the remote should've encrypted using our public key
    if (crypto_box_seal_open(decrypted, encrypted, sizeof encrypted, m_identity->key_public, m_identity->key_secret) != 0)
        return false;

#ifdef WIREFOX_SODIUM_MONITORING
    PrintKeyToStdout("received challenge = ", encrypted);
    PrintKeyToStdout("decrypt  challenge = ", decrypted);
#endif

    // now that we have the decrypted challenge, write it out
    answer.WriteBytes(decrypted, sizeof decrypted);
    return true;
}

void EncryptionLayerSodium::SetLocalIdentity(std::shared_ptr<EncryptionLayer::Keypair> keypair) {
    m_identity = std::static_pointer_cast<EncryptionLayerSodium::Keypair>(keypair);
}

void EncryptionLayerSodium::ExpectRemoteIdentity(BinaryStream& pubkey) {
    m_remoteIdentityKnown = true;
    pubkey.ReadBytes(m_remote_identity_pk, KEY_LENGTH);
}

BinaryStream EncryptionLayerSodium::Encrypt(const BinaryStream& plaintext) {
    // allocate a buffer long enough to hold the full ciphertext
    const size_t ciphertext_len_full = plaintext.GetLength() + WIREFOX_SODIUM_MAC_LEN + WIREFOX_SODIUM_NONCE_LEN;
    BinaryStream ciphertext = PreallocateBuffer(ciphertext_len_full);
    auto* ciphertext_ptr = ciphertext.GetWritableBuffer() + WIREFOX_SODIUM_NONCE_LEN;

    // get a random nonce, and add it to the full ciphertext as prefix
    uint8_t nonce[WIREFOX_SODIUM_NONCE_LEN];
    randombytes_buf(nonce, sizeof nonce);
    ciphertext.WriteBytes(nonce, sizeof nonce);

    // encrypt the plaintext and write it to the output BinaryStream
    //crypto_aead_xchacha20poly1305_ietf_encrypt()
    crypto_secretbox_easy(ciphertext_ptr, plaintext.GetBuffer(), plaintext.GetLength(), nonce, m_key_tx);

    ciphertext.SeekToBegin();
    return ciphertext;
}

BinaryStream EncryptionLayerSodium::Decrypt(BinaryStream& ciphertext) {
    // read out the nonce from the stream
    uint8_t nonce[WIREFOX_SODIUM_NONCE_LEN];
    assert(ciphertext.GetPosition() == 0);  // because we do pointer arithmetic below, which assumes the full ciphertext starts at GetBuffer().
    ciphertext.SeekToBegin();               // but set it anyway, just to be safe
    ciphertext.ReadBytes(nonce, sizeof nonce);

    // prevent buffer overread if ciphertext is too small to actually contain a proper ciphertext
    if (ciphertext.GetLength() < WIREFOX_SODIUM_MAC_LEN + WIREFOX_SODIUM_NONCE_LEN) {
        m_error = true;
        return ciphertext;
    }

    const auto* ciphertext_ptr = ciphertext.GetBuffer() + WIREFOX_SODIUM_NONCE_LEN;
    const size_t ciphertext_len = ciphertext.GetLength() - WIREFOX_SODIUM_NONCE_LEN;
    const size_t plaintext_len = ciphertext_len - WIREFOX_SODIUM_MAC_LEN;

    // allocate a buffer long enough to hold the decrypted plaintext
    BinaryStream plaintext = PreallocateBuffer(plaintext_len);
    assert(plaintext.GetCapacity() >= plaintext_len);

    // perform decryption
    if (crypto_secretbox_open_easy(plaintext.GetWritableBuffer(), ciphertext_ptr, ciphertext_len, nonce, m_key_rx) != 0)
        m_error = true;

    return plaintext;
}

#endif
