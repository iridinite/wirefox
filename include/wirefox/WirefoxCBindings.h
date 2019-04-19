/**
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#ifndef _WIREFOX_C_BINDINGS_H
#define _WIREFOX_C_BINDINGS_H

#ifdef WIREFOX_BUILD_C_BINDINGS

extern "C" {

    // Essentially void pointers. Unfortunately we cannot import any of the C++ classes,
    // so we lose all type safety for this wrapper, and have to duplicate all enums :(
typedef struct HWirefoxPeer HWirefoxPeer;
typedef struct HPacket HPacket;
typedef uint64_t TPeerID;

typedef enum {
    PROTO_IPv4 = 0,
    PROTO_IPv6 = 1
} ESocketProtocol;

typedef enum {
    POPT_UNRELIABLE = 0,
    POPT_RELIABLE = 1,
} EPacketOptions;

typedef enum {
    CONNECTATTEMPT_OK,
    CONNECTATTEMPT_INVALID_PARAMETER,
    CONNECTATTEMPT_INVALID_HOSTNAME,
    CONNECTATTEMPT_INVALID_STATE,
    CONNECTATTEMPT_ALREADY_CONNECTING,
    CONNECTATTEMPT_NO_FREE_SLOTS
} EConnectAttemptResult;

typedef enum {
    CONNECTRESULT_IN_PROGRESS,
    CONNECTRESULT_OK,
    CONNECTRESULT_CONNECT_FAILED,
    CONNECTRESULT_INCOMPATIBLE_PROTOCOL,
    CONNECTRESULT_INCOMPATIBLE_VERSION,
    CONNECTRESULT_INCOMPATIBLE_SECURITY,
    CONNECTRESULT_INVALID_PASSWORD,
    CONNECTRESULT_NO_FREE_SLOTS,
    CONNECTRESULT_ALREADY_CONNECTED,
    CONNECTRESULT_IP_RATE_LIMITED,
    CONNECTRESULT_BANNED
} EConnectResult;

WIREFOX_API HWirefoxPeer* wirefox_peer_create(size_t maxPeers);
WIREFOX_API void wirefox_peer_destroy(HWirefoxPeer* handle);
WIREFOX_API int wirefox_peer_bind(HWirefoxPeer* handle, ESocketProtocol protocol, uint16_t port);
WIREFOX_API EConnectAttemptResult wirefox_peer_connect(HWirefoxPeer* handle, const char* host, uint16_t port);

WIREFOX_API void wirefox_peer_send_loopback(HWirefoxPeer* handle, HPacket* packet);
WIREFOX_API int wirefox_peer_send(HWirefoxPeer* handle, HPacket* packet, TPeerID recipient, EPacketOptions options);
WIREFOX_API HPacket* wirefox_peer_receive(HWirefoxPeer* handle);

WIREFOX_API HPacket* wirefox_packet_create(uint8_t cmd, const uint8_t* data, size_t len);
WIREFOX_API void wirefox_packet_destroy(HPacket* handle);
WIREFOX_API const uint8_t* wirefox_packet_get_data(HPacket* packet);
WIREFOX_API size_t wirefox_packet_get_length(HPacket* packet);
WIREFOX_API uint8_t wirefox_packet_get_cmd(HPacket* packet);
WIREFOX_API TPeerID wirefox_packet_get_sender(HPacket* packet);

}

#endif
#endif
