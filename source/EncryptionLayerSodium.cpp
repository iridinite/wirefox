#include "PCH.h"
#include "EncryptionLayerSodium.h"

#ifdef WIREFOX_ENABLE_ENCRYPTION
#include <sodium.h>

using namespace detail;

namespace {
    
    BinaryStream PreallocateBuffer(const size_t len) {
        BinaryStream buffer(len);
        buffer.WriteZeroes(len); // advance length counter
        return buffer;
    }

}

EncryptionLayerSodium::EncryptionLayerSodium()
    : m_kx_secret{}
    , m_kx_public{}
    , m_key_rx{}
    , m_key_tx{}
    , m_error(false) {
    // asserts done here to make sure the key array sizes match up with what libsodium expects;
    // I don't want to include sodium.h publically so can't use the macros there directly
    static_assert(crypto_kx_PUBLICKEYBYTES == KEY_LENGTH, "bad public key array size");
    static_assert(crypto_kx_SECRETKEYBYTES == KEY_LENGTH, "bad private key array size");
    static_assert(crypto_kx_SESSIONKEYBYTES == KEY_LENGTH, "bad session key array size");

    if (sodium_init() < 0)
        throw std::runtime_error("libsodium failed to init");

    crypto_kx_keypair(m_kx_public, m_kx_secret);
}

EncryptionLayerSodium::EncryptionLayerSodium(EncryptionLayerSodium&&) noexcept
    : EncryptionLayerSodium() {}

EncryptionLayerSodium::~EncryptionLayerSodium() {
    // securely erase all keys from memory
    sodium_memzero(m_kx_secret, KEY_LENGTH);
    sodium_memzero(m_kx_public, KEY_LENGTH);
    sodium_memzero(m_key_rx, KEY_LENGTH);
    sodium_memzero(m_key_tx, KEY_LENGTH);
}

EncryptionLayerSodium& EncryptionLayerSodium::operator=(EncryptionLayerSodium&&) noexcept {
    return *this;
}

bool EncryptionLayerSodium::GetNeedsToBail() const {
    return m_error;
}

size_t EncryptionLayerSodium::GetOverhead() const {
    return crypto_secretbox_MACBYTES + crypto_secretbox_NONCEBYTES;
}

BinaryStream EncryptionLayerSodium::GetPublicKey() const {
    BinaryStream ret(KEY_LENGTH);
    ret.WriteBytes(m_kx_public, KEY_LENGTH);
    ret.SeekToBegin();
    return ret;
}

void EncryptionLayerSodium::SetRemotePublicKey(Handshaker::Origin origin, BinaryStream& pubkey) {
    uint8_t remotekey[KEY_LENGTH];
    pubkey.ReadBytes(remotekey, KEY_LENGTH);

    switch (origin) {
    case Handshaker::Origin::SELF:
        if (crypto_kx_client_session_keys(m_key_rx, m_key_tx, m_kx_public, m_kx_secret, remotekey) != 0)
            m_error = true;
        break;
    case Handshaker::Origin::REMOTE:
        if (crypto_kx_server_session_keys(m_key_rx, m_key_tx, m_kx_public, m_kx_secret, remotekey) != 0)
            m_error = true;
        break;
    default:
        assert(false && "invalid Handshaker::Origin in EncryptionLayerSodium::SetRemotePublicKey");
        break;
    }
}

BinaryStream EncryptionLayerSodium::Encrypt(const BinaryStream& plaintext) {
    // allocate a buffer long enough to hold the full ciphertext
    const size_t ciphertext_len_full = plaintext.GetLength() + crypto_secretbox_MACBYTES + crypto_secretbox_NONCEBYTES;
    BinaryStream ciphertext = PreallocateBuffer(ciphertext_len_full);
    auto* ciphertext_ptr = ciphertext.GetWritableBuffer() + crypto_secretbox_NONCEBYTES;

    // get a random nonce, and add it to the full ciphertext as prefix
    uint8_t nonce[crypto_secretbox_NONCEBYTES];
    randombytes_buf(nonce, sizeof nonce);
    ciphertext.SeekToBegin();
    ciphertext.WriteBytes(nonce, sizeof nonce);

    // encrypt the plaintext and write it to the output BinaryStream
    crypto_secretbox_easy(ciphertext_ptr, plaintext.GetBuffer(), plaintext.GetLength(), nonce, m_key_tx);

    ciphertext.SeekToBegin();
    return ciphertext;
}

BinaryStream EncryptionLayerSodium::Decrypt(BinaryStream& ciphertext) {
    uint8_t nonce[crypto_secretbox_NONCEBYTES];
    ciphertext.SeekToBegin();
    ciphertext.ReadBytes(nonce, sizeof nonce);

    assert(static_cast<int>(ciphertext.GetLength()) - crypto_secretbox_MACBYTES - crypto_secretbox_NONCEBYTES >= 0);
    const size_t plaintext_len = ciphertext.GetLength() - crypto_secretbox_MACBYTES - crypto_secretbox_NONCEBYTES;
    const size_t ciphertext_len = ciphertext.GetLength() - crypto_secretbox_NONCEBYTES;

    BinaryStream plaintext = PreallocateBuffer(plaintext_len);
    if (crypto_secretbox_open_easy(plaintext.GetWritableBuffer(), ciphertext.GetBuffer() + crypto_secretbox_NONCEBYTES, ciphertext_len, nonce, m_key_rx) != 0)
        m_error = true;

    plaintext.SeekToBegin();
    return plaintext;
}

#endif
