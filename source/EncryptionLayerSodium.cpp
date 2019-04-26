#include "PCH.h"
#include "EncryptionLayerSodium.h"

#ifdef WIREFOX_ENABLE_ENCRYPTION
#include <sodium.h>

using namespace detail;

namespace {

    BinaryStream PreallocateBuffer(const size_t len) {
        BinaryStream buffer(len);
        buffer.WriteZeroes(len); // advance length counter
        buffer.SeekToBegin();
        return buffer;
    }

}

EncryptionLayerSodium::Keypair::Keypair()
    : key_public{}
    , key_secret{} {
    if (sodium_init() < 0)
        throw std::runtime_error("libsodium failed to init");

    crypto_kx_keypair(key_public, key_secret);
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
    : m_key_expect_pub{}
    , m_key_rx{}
    , m_key_tx{}
    , m_error(false)
    , m_knowsRemotePubkey(false) {
    // asserts done here to make sure the key array sizes match up with what libsodium expects;
    // I don't want to include sodium.h publically so can't use the macros there directly
    static_assert(crypto_kx_PUBLICKEYBYTES == KEY_LENGTH, "bad public key array size");
    static_assert(crypto_kx_SECRETKEYBYTES == KEY_LENGTH, "bad private key array size");
    static_assert(crypto_kx_SESSIONKEYBYTES == KEY_LENGTH, "bad session key array size");

    if (sodium_init() < 0)
        throw std::runtime_error("libsodium failed to init");
}

EncryptionLayerSodium::EncryptionLayerSodium(EncryptionLayerSodium&&) noexcept
    : EncryptionLayerSodium() {}

EncryptionLayerSodium::~EncryptionLayerSodium() {
    // securely erase all keys from memory
    sodium_memzero(m_key_expect_pub, KEY_LENGTH);
    sodium_memzero(m_key_tx, KEY_LENGTH);
    sodium_memzero(m_key_tx, KEY_LENGTH);
}

EncryptionLayerSodium& EncryptionLayerSodium::operator=(EncryptionLayerSodium&&) noexcept {
    return *this;
}

bool EncryptionLayerSodium::GetNeedsToBail() const {
    return m_error;
}

size_t EncryptionLayerSodium::GetOverhead() {
    return crypto_secretbox_MACBYTES + crypto_secretbox_NONCEBYTES;
}

size_t EncryptionLayerSodium::GetKeyLength() {
    return KEY_LENGTH;
}

BinaryStream EncryptionLayerSodium::GetPublicKey() const {
    BinaryStream ret(KEY_LENGTH);
    ret.WriteBytes(m_kx->key_public, KEY_LENGTH);
    ret.SeekToBegin();
    return ret;
}

bool EncryptionLayerSodium::SetRemotePublicKey(Handshaker::Origin origin, BinaryStream& pubkey) {
    uint8_t remotekey[KEY_LENGTH];
    pubkey.ReadBytes(remotekey, KEY_LENGTH);

    // verify identity of remote, if we know the key already
    if (m_knowsRemotePubkey && sodium_memcmp(remotekey, m_key_expect_pub, KEY_LENGTH) != 0) {
        // mismatch, remote may be an impostor
        return false;
    }

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
        assert(false && "invalid Handshaker::Origin in EncryptionLayerSodium::SetRemotePublicKey");
        break;
    }

    return !m_error;
}

void EncryptionLayerSodium::SetLocalKeypair(std::shared_ptr<EncryptionLayer::Keypair> keypair) {
    m_kx = std::static_pointer_cast<EncryptionLayerSodium::Keypair>(keypair);
}

void EncryptionLayerSodium::ExpectRemotePublicKey(BinaryStream& pubkey) {
    m_knowsRemotePubkey = true;
    pubkey.ReadBytes(m_key_expect_pub, KEY_LENGTH);
}

BinaryStream EncryptionLayerSodium::Encrypt(const BinaryStream& plaintext) {
    // allocate a buffer long enough to hold the full ciphertext
    const size_t ciphertext_len_full = plaintext.GetLength() + crypto_secretbox_MACBYTES + crypto_secretbox_NONCEBYTES;
    BinaryStream ciphertext = PreallocateBuffer(ciphertext_len_full);
    auto* ciphertext_ptr = ciphertext.GetWritableBuffer() + crypto_secretbox_NONCEBYTES;

    // get a random nonce, and add it to the full ciphertext as prefix
    uint8_t nonce[crypto_secretbox_NONCEBYTES];
    randombytes_buf(nonce, sizeof nonce);
    ciphertext.WriteBytes(nonce, sizeof nonce);

    // encrypt the plaintext and write it to the output BinaryStream
    crypto_secretbox_easy(ciphertext_ptr, plaintext.GetBuffer(), plaintext.GetLength(), nonce, m_key_tx);

    ciphertext.SeekToBegin();
    return ciphertext;
}

BinaryStream EncryptionLayerSodium::Decrypt(BinaryStream& ciphertext) {
    // read out the nonce from the stream
    uint8_t nonce[crypto_secretbox_NONCEBYTES];
    assert(ciphertext.GetPosition() == 0); // because we do pointer arithmetic below, which assumes the full ciphertext starts at GetBuffer()
    ciphertext.SeekToBegin();
    ciphertext.ReadBytes(nonce, sizeof nonce);

    assert(ciphertext.GetLength() >= crypto_secretbox_MACBYTES + crypto_secretbox_NONCEBYTES);
    const size_t plaintext_len = ciphertext.GetLength() - crypto_secretbox_MACBYTES - crypto_secretbox_NONCEBYTES;
    const size_t ciphertext_len = ciphertext.GetLength() - crypto_secretbox_NONCEBYTES;

    // allocate a buffer long enough to hold the decrypted plaintext
    BinaryStream plaintext = PreallocateBuffer(plaintext_len);
    assert(plaintext.GetCapacity() >= plaintext_len);
    // perform decryption
    if (crypto_secretbox_open_easy(plaintext.GetWritableBuffer(), ciphertext.GetBuffer() + crypto_secretbox_NONCEBYTES, ciphertext_len, nonce, m_key_rx) != 0)
        m_error = true;

    return plaintext;
}

#endif
