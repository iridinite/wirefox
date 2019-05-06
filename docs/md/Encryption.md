Cryptography & Security
=======================

[TOC]

# Overview

Wirefox can provide encryption on your connections. Features include:

* **Strong cipher:** Wirefox uses the fast, well-established ChaCha20 cipher with 256-bit keys. This cipher, plus the Poly1305 authenticator, is battle-tested and has no known practical vulnerabilities.
* **Perfect forward secrecy:** Unique session keys are negotiated for every connection. Even if keys for one session are later compromised, no other sessions can be decrypted.
* **Resists tampering:** All messages are stamped with MAC signatures, preventing an adversary from changing the ciphertext in transit.
* **Resists replay attacks:** An eavesdropper who records encrypted traffic, cannot later play that conversation back to either party, because new session keys will be in use.
* **Resists man-in-the-middle attacks:** If (and only if) the server's public key is known ahead of time, the client challenges the server to prove its identity.

@warning **IMPORTANT:** If a connecting client does not know the server's public key ahead of time, this cryptography suite is vulnerable to active MITM-attacks (man-in-the-middle) during the key exchange. An adversary could intercept and forge session keys, and the connecting party has no way of detecting this. This is an inevitable consequence of the fact that in a pure P2P scenario, without any information about the remote party, it is simply impossible to guarantee their identity. Keep this in mind while developing your application.

## Crypto for peer-to-peer

To enable cryptography features, all you need to do is call the relevant function on your Peer (*before* you call `Bind()`):

```cpp
myPeer->SetEncryptionEnabled(true);
```

This will cause Wirefox to generate a random identity and key exchange settings. This works fine on a local network, but as explained above, do keep in mind that this is potentially vulnerable to active attacks.

The better, more robust way to do this, is using a dedicated server.

## Crypto for dedicated servers

By dedicated server, I'm referring to a server that you, as the developer/publisher, operate, and have full control over. Particularly, this means that any game clients out in the wild already know who they're supposed to be connecting to - the server is an established, well-known authority.

### Preparing identity

Clients will identify your server using a public/private keypair that identifies you. You can generate this keypair using the `GenerateIdentity()` method, as shown below. You can then write your keys to files on disk, so you can read them back in later, or use whatever other persistent storage is relevant for you.

Of course, you only need to do this once (or whenever you need to rotate secrets).

```cpp
// Allocate two buffers long enough to hold our identity keys
const auto key_length = myPeer->GetEncryptionKeyLength();
auto key_secret = std::make_unique<uint8_t[]>(key_length);
auto key_public = std::make_unique<uint8_t[]>(key_length);

// Generate a keypair
myPeer->GenerateIdentity(key_secret.get(), key_public.get());

// Save your keys somewhere safe!
// ...
```

When your server program starts up, read your keypair from persistent storage and inform Wirefox about it, like so:

```cpp
myPeer->SetEncryptionEnabled(true);
myPeer->SetEncryptionIdentity(server_key_secret, server_key_public);
```

### Enforcing server identity

Now that your server has an established identity, the client needs to verify it when connecting. To do so, all you need to do is pass in the server's public key to the optional parameter on Connect():

```cpp
auto attempt = myPeer->Connect("game.example.com", 1337, server_key_public);
// ... process ConnectAttemptResult as normal, etc.
```

@warning Do not ship your server's private key with the client application. The public key is needed, but leaking the private key will allow anyone to successfully impersonate your server.

That's all you need to do. After establishing the key exchange, the client will now send the server a challenge that it can only solve if it actually knows the proper private key. If something goes wrong, you'll get one of these errors:

* `ConnectAttemptResult::INVALID_PARAMETER` - You passed in an explicit public key, but encryption is not enabled on this peer. For safety reasons, this is considered an error. (It would be dangerous to silently discard the public key, and make you think you were having an encrypted conversation.)
* `ConnectResult::INCOMPATIBLE_SECURITY` - 1) The two peers do not have matching encryption settings. Make sure none or both have crypto enabled. 2) An internal cryptography-related error occurred, likely a bug.
* `ConnectResult::INCORRECT_REMOTE_IDENTITY` - The identity of the remote peer could not be verified. There may be a configuration error, or (worst case) an active MITM-attack is ongoing.

## Overhead

* **CPU time:** ChaCha20-Poly1305 is a [fast and battery-friendly](https://www.imperialviolet.org/2014/02/27/tlssymmetriccrypto.html) cipher, [vetted for by Google](https://blog.cloudflare.com/do-the-chacha-better-mobile-performance-with-cryptography/) as a strong mobile replacement for AES.
* **Memory:** An additional ~250 bytes per peer slot are required; for most purposes the difference should be negligible.
* **Bandwidth:** Two extra round trips are required during handshake (one for key exchange, one for authentication). After handshake, a constant overhead of 40 bytes per datagram is added.
