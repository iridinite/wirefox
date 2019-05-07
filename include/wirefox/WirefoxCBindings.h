/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#ifndef _WIREFOX_C_BINDINGS_H
#define _WIREFOX_C_BINDINGS_H

#ifdef WIREFOX_BUILD_C_BINDINGS

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Essentially void pointers. Unfortunately we cannot import any of the C++ classes,
// so we lose all type safety for this wrapper.
typedef struct HWirefoxPeer HWirefoxPeer;
typedef struct HPacket HPacket;
typedef uint64_t TPeerID;
typedef uint32_t TPacketID;
typedef uint8_t TChannelIndex;

// Empty enums just to have types rather than ints in function signatures. I really don't want to duplicate
// all enums here *again*. If you're actually writing C and need those enums, copy them from Enumerations.h.
typedef enum { _DUMMY_1 } ESocketProtocol, EConnectAttemptResult, EChannelMode;
typedef enum : uint8_t { _DUMMY_2 } EPacketOptions, EPacketPriority;

WIREFOX_API HWirefoxPeer*   wirefox_peer_create(size_t maxPeers);
WIREFOX_API void            wirefox_peer_destroy(HWirefoxPeer* handle);
WIREFOX_API int             wirefox_peer_bind(HWirefoxPeer* handle, ESocketProtocol protocol, uint16_t port);
WIREFOX_API void            wirefox_peer_stop(HWirefoxPeer* handle, unsigned linger);

WIREFOX_API EConnectAttemptResult wirefox_peer_connect(HWirefoxPeer* handle, const char* host, uint16_t port);
WIREFOX_API void            wirefox_peer_disconnect(HWirefoxPeer* handle, TPeerID who, unsigned linger);
WIREFOX_API void            wirefox_peer_disconnect_immediate(HWirefoxPeer* handle, TPeerID who);

WIREFOX_API void            wirefox_peer_send_loopback(HWirefoxPeer* handle, HPacket* packet);
WIREFOX_API TPacketID       wirefox_peer_send(HWirefoxPeer* handle, HPacket* packet, TPeerID recipient, EPacketOptions options, EPacketPriority priority, TChannelIndex channelIndex);
WIREFOX_API HPacket*        wirefox_peer_receive(HWirefoxPeer* handle);

WIREFOX_API TChannelIndex   wirefox_peer_make_channel(HWirefoxPeer* handle, EChannelMode mode);
WIREFOX_API EChannelMode    wirefox_peer_get_channel_mode(HWirefoxPeer* handle, TChannelIndex index);
WIREFOX_API TPeerID         wirefox_peer_get_my_id(HWirefoxPeer* handle);
WIREFOX_API size_t          wirefox_peer_get_max_peers(HWirefoxPeer* handle);
WIREFOX_API size_t          wirefox_peer_get_max_incoming_peers(HWirefoxPeer* handle);
WIREFOX_API void            wirefox_peer_set_max_incoming_peers(HWirefoxPeer* handle, size_t incoming);
WIREFOX_API unsigned        wirefox_peer_get_ping(HWirefoxPeer* handle, TPeerID who);
WIREFOX_API int             wirefox_peer_get_ping_available(HWirefoxPeer* handle, TPeerID who);
WIREFOX_API void            wirefox_peer_set_network_sim(HWirefoxPeer* handle, float packetLoss, unsigned additionalPing);

WIREFOX_API void            wirefox_peer_set_offline_ad(HWirefoxPeer* handle, const uint8_t* data, size_t len);
WIREFOX_API void            wirefox_peer_disable_offline_ad(HWirefoxPeer* handle);
WIREFOX_API void            wirefox_peer_ping(HWirefoxPeer* handle, const char* host, uint16_t port);
WIREFOX_API void            wirefox_peer_ping_local_network(HWirefoxPeer* handle, uint16_t port);

WIREFOX_API size_t          wirefox_peer_get_crypto_key_length(HWirefoxPeer* handle);
WIREFOX_API int             wirefox_peer_get_crypto_enabled(HWirefoxPeer* handle);
WIREFOX_API void            wirefox_peer_set_crypto_enabled(HWirefoxPeer* handle, int enabled);
WIREFOX_API void            wirefox_peer_set_crypto_identity(HWirefoxPeer* handle, const uint8_t* key_secret, const uint8_t* key_public);
WIREFOX_API void            wirefox_peer_generate_crypto_identity(HWirefoxPeer* handle, uint8_t* key_secret, uint8_t* key_public);

WIREFOX_API HPacket*        wirefox_packet_create(uint8_t cmd, const uint8_t* data, size_t len);
WIREFOX_API void            wirefox_packet_destroy(HPacket* handle);
WIREFOX_API const uint8_t*  wirefox_packet_get_data(HPacket* packet);
WIREFOX_API size_t          wirefox_packet_get_length(HPacket* packet);
WIREFOX_API uint8_t         wirefox_packet_get_cmd(HPacket* packet);
WIREFOX_API TPeerID         wirefox_packet_get_sender(HPacket* packet);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // WIREFOX_BUILD_C_BINDINGS
#endif // _WIREFOX_C_BINDINGS_H
