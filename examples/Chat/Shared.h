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
