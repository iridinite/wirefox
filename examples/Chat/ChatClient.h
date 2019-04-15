#pragma once
#include "Shared.h"

class ChatClient : public IChat {
public:
    ChatClient(unsigned short port);
    ~ChatClient();

    void    Tick() override;
    void    HandleInput(const std::string& input) override;

private:
    void    Connect(const std::string& host, unsigned short port);

    std::unique_ptr<wirefox::IPeer> m_peer;
    wirefox::PeerID         m_server;
    wirefox::Channel        m_channelChat;
    unsigned short          m_port;
    bool                    m_connected;
};
