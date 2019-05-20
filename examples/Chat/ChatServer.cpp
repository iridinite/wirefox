#include "ChatServer.h"

ChatServer::ChatServer(unsigned short port)
    : m_rng(time(nullptr)) {
    constexpr size_t maxClients = 32;

    m_peer = wirefox::IPeer::Factory::Create(maxClients);
    m_peer->SetMaximumIncomingPeers(maxClients);
    m_peer->SetNetworkSimulation(0.1f, 5);
    m_peer->SetEncryptionEnabled(true);
    m_peer->SetEncryptionIdentity(SERVER_KEY_SECRET, SERVER_KEY_PUBLIC);
    if (!m_peer->Bind(wirefox::SocketProtocol::IPv4, port)) {
        std::cerr << "Server failed to bind to port!" << std::endl;
        exit(1);
    }

    m_channelChat = m_peer->MakeChannel(wirefox::ChannelMode::ORDERED);
}

ChatServer::~ChatServer() {
    m_peer->Stop();
}

void ChatServer::Tick() {
    auto recv = m_peer->Receive();
    while (recv) {
        switch (recv->GetCommand()) {
        case wirefox::PacketCommand::NOTIFY_CONNECTION_INCOMING: {
            // new incoming user! add them to our list of folks, and give them a nice name
            ChatUser newbie;
            newbie.id = recv->GetSender();
            newbie.nick = std::string("RandomStranger") + std::to_string((m_rng() % 100000) + 10000);
            Broadcast(newbie.nick + " has joined the chat!");
            SendToSpecific(newbie.id, "Welcome!");
            m_users.push_back(std::move(newbie));
            break;
        }
        case wirefox::PacketCommand::NOTIFY_CONNECTION_LOST:
        case wirefox::PacketCommand::NOTIFY_DISCONNECTED: {
            // user left :(
            auto sender = recv->GetSender();
            auto it = std::find_if(m_users.begin(), m_users.end(), [sender](const auto& u) {
                return u.id == sender;
            });
            if (it == m_users.end()) {
                std::cerr << "[ERROR] Unknown user " << recv->GetSender() << " disconnected?? Ignoring..." << std::endl;
                break;
            }
            Broadcast(it->nick + " has left the chat! :(");
            m_users.erase(it);
            break;
        }
        default: {
            auto customCmd = static_cast<ChatPacketCommand>(recv->GetCommand());
            auto* user = GetUserByPeerID(recv->GetSender());
            if (user == nullptr) {
                std::cerr << "[ERROR] Got packet from unknown user " << recv->GetSender() << ", discarding..." << std::endl;
                break;
            }

            if (customCmd == ChatPacketCommand::MESSAGE) {
                // this is a chat message. get a BinaryStream from the Packet, use it to read out the message contents
                wirefox::BinaryStream instream = recv->GetStream();
                HandleChatMessage(user, instream.ReadString());
            }
        }
        }
        
        // keep going while we more incoming data to process
        recv = m_peer->Receive();
    }
}

void ChatServer::HandleInput(const std::string&) {}

void ChatServer::HandleChatMessage(ChatUser* user, const std::string& message) {
    if (strncmp(message.c_str(), "/nick ", 6) == 0) {
        // --------------- NICKNAME COMMAND ---------------
        if (message.size() < 7) {
            SendToSpecific(user->id, "Usage: /nick <name>");
            return;
        }

        // parse the new desired nickname from the message
        std::string desiredNick = message.substr(6);
        trim(desiredNick);
        if (desiredNick.empty()) {
            SendToSpecific(user->id, "Please provide a non-empty nick.");
            return;
        }

        // change the user's nickname as requested
        Broadcast(user->nick + " has changed their nick to " + desiredNick);
        user->nick = desiredNick;
        return;
    }
    if (strcmp(message.c_str(), "/list") == 0) {
        // --------------- LIST COMMAND ---------------
        std::string output = "Connected users: ";
        for (const auto& listEntry : m_users) {
            output += listEntry.nick + ", ";
        }

        SendToSpecific(user->id, output);
        return;
    }

    // otherwise, treat this as a normal chat message, so prefix the user's nick and forward it to everyone
    std::string chatMessage = "[" + user->nick + "] " + message;
    Broadcast(chatMessage);
}

void ChatServer::Broadcast(const std::string& message) const {
    // TODO: could maybe put an out-of-the-box Broadcast function in wirefox::Peer?

    // console output in server
    std::cout << message << std::endl;

    for (const auto& user : m_users)
        SendToSpecific(user.id, message);
}

void ChatServer::SendToSpecific(wirefox::PeerID id, const std::string& message) const {
    wirefox::BinaryStream outstream;
    outstream.WriteString(message);

    wirefox::Packet packet(static_cast<wirefox::PacketCommand>(ChatPacketCommand::MESSAGE), std::move(outstream));
    m_peer->Send(packet, id, wirefox::PacketOptions::RELIABLE, wirefox::PacketPriority::MEDIUM, m_channelChat);
}

ChatServer::ChatUser* ChatServer::GetUserByPeerID(wirefox::PeerID id) {
    auto it = std::find_if(m_users.begin(), m_users.end(), [id](const auto& u) {
        return u.id == id;
    });

    if (it == m_users.end()) return nullptr;
    return &(*it);
}
