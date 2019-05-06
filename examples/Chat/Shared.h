#pragma once
#include <Wirefox.h>
#include <algorithm>
#include <locale>
#include <cstring>

/// Default port for the client and server to talk on, if -port option was not specified
constexpr static unsigned short CHAT_DEFAULT_PORT = 51234;

/// Here's where you put custom message IDs
enum class ChatPacketCommand {
    MESSAGE = wirefox::USER_PACKET_START
};

/// Quick-and-dirty interface to help simplify Main.cpp somewhat
class IChat {
public:
    virtual ~IChat() = default;

    virtual void    Tick() = 0;
    virtual void    HandleInput(const std::string& input) = 0;
};


// These are example encryption keys, hardcoded for convenience.
// You can generate your own keys by calling peer->GenerateKeypair(), and passing in two memory blocks
// that are large enough to hold them (peer->GetEncryptionKeyLength()), then save them to a file.

// Pass your keys peer->SetEncryptionLocalKeypair() on the server, then pass the public key to
// peer->Connect() on the client. The client will refuse to complete the connection if the server
// then reports a mismatching public key, thus providing resistance against MITM attacks.
// Obviously, do not ship the server's secret key with the client like in this demo.

// Note that you don't have to do all this in a pure P2P scenario (you can't, because the server's
// private key must not ship with the client). Just enabling encryption is enough for general purposes;
// Wirefox will generate keys and deal with everything under the hood, but keep in mind that an adversary
// could falsify the key exchange, and the only way to prevent that is by having a secure dedi server.

constexpr static unsigned char SERVER_KEY_SECRET[32] = {
    0xB9, 0x8D, 0xB6, 0x97, 0x0B, 0x33, 0x78, 0xB3, 0xD4, 0xBE, 0x70, 0x2B, 0xC0, 0x3B, 0x63, 0xB5,
    0x3C, 0x29, 0xA0, 0xFA, 0xA7, 0x21, 0x8F, 0x59, 0x4D, 0xA4, 0x02, 0xBB, 0xF2, 0xEA, 0x52, 0xCA
};
constexpr static unsigned char SERVER_KEY_PUBLIC[32] = {
    0x1B, 0xB1, 0x24, 0x3B, 0x00, 0xE8, 0x60, 0x94, 0xB9, 0x7E, 0x3B, 0xFF, 0x2D, 0x84, 0x7B, 0x49,
    0x50, 0xE1, 0x33, 0x74, 0xAC, 0x1B, 0x47, 0x84, 0x1D, 0x1D, 0xFE, 0xB7, 0xDE, 0x6F, 0xAB, 0x2C
};


// https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
// trim from start (in place)
inline void ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !isspace(ch);
    }));
}

// trim from end (in place)
inline void rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
inline void trim(std::string& s) {
    ltrim(s);
    rtrim(s);
}
