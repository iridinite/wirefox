#pragma once
#include "Shared.h"

class ChatServer : public IChat {
public:
    ChatServer(unsigned short port);
    ~ChatServer();

    void    Tick() override;
    void    HandleInput(const std::string& input) override;

private:
    struct ChatUser {
        wirefox::PeerID id;
        std::string nick;
    };

    void        HandleChatMessage(ChatUser* user, const std::string& message);

    void        Broadcast(const std::string& message) const;
    void        SendToSpecific(wirefox::PeerID id, const std::string& message) const;

    void        ExampleRPC(wirefox::IPeer& peer, wirefox::PeerID sender, wirefox::BinaryStream& instream);
    void        ExampleBlockingRPC(wirefox::IPeer& peer, wirefox::PeerID sender, wirefox::BinaryStream& instream, wirefox::BinaryStream& response);

    ChatUser*   GetUserByPeerID(wirefox::PeerID id);

    std::unique_ptr<wirefox::IPeer> m_peer;
    wirefox::Channel        m_channelChat;
    std::mt19937_64         m_rng;
    std::vector<ChatUser>   m_users;
};
