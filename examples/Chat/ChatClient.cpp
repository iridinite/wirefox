#include "ChatClient.h"

namespace {

    std::string ConnectResultToString(wirefox::ConnectResult r) {
        switch (r) {
        case wirefox::ConnectResult::IN_PROGRESS:
            return "IN_PROGRESS";
        case wirefox::ConnectResult::OK:
            return "OK";
        case wirefox::ConnectResult::CONNECT_FAILED:
            return "CONNECT_FAILED";
        case wirefox::ConnectResult::INCOMPATIBLE_PROTOCOL:
            return "INCOMPATIBLE_PROTOCOL";
        case wirefox::ConnectResult::INCOMPATIBLE_VERSION:
            return "INCOMPATIBLE_VERSION";
        case wirefox::ConnectResult::INCOMPATIBLE_SECURITY:
            return "INCOMPATIBLE_SECURITY";
        case wirefox::ConnectResult::INVALID_PASSWORD:
            return "INVALID_PASSWORD";
        case wirefox::ConnectResult::NO_FREE_SLOTS:
            return "NO_FREE_SLOTS";
        case wirefox::ConnectResult::ALREADY_CONNECTED:
            return "ALREADY_CONNECTED";
        case wirefox::ConnectResult::IP_RATE_LIMITED:
            return "IP_RATE_LIMITED";
        case wirefox::ConnectResult::BANNED:
            return "BANNED";
        default:
            return "<?>";
        }
    }

}

ChatClient::ChatClient(unsigned short port)
    : m_port(port)
    , m_connected(false) {
    m_peer = wirefox::IPeer::Factory::Create();
    m_peer->Bind(wirefox::SocketProtocol::IPv4, 0); // bind to any port, doesn't matter for client
    m_peer->SetNetworkSimulation(0.1f, 5);

    m_channelChat = m_peer->MakeChannel(wirefox::ChannelMode::ORDERED);
}

ChatClient::~ChatClient() {
    m_peer->Stop();
}

void ChatClient::Tick() {
    auto recv = m_peer->Receive();
    while (recv) {
        switch (recv->GetCommand()) {
        case wirefox::PacketCommand::NOTIFY_CONNECT_SUCCESS: {
            m_server = recv->GetSender();
            m_connected = true;
            break;
        }
        case wirefox::PacketCommand::NOTIFY_CONNECT_FAILED: {
            // check what happened with our connection attempt
            auto instream = recv->GetStream();
            auto result = static_cast<wirefox::ConnectResult>(instream.ReadByte());
            std::cout << "Failed to connect! Reason: " << ConnectResultToString(result) << std::endl;

            break;
        }
        case wirefox::PacketCommand::NOTIFY_CONNECTION_LOST:
        case wirefox::PacketCommand::NOTIFY_DISCONNECTED: {
            // user left :(
            m_connected = false;
            std::cout << "Disconnected from server." << std::endl;
            break;
        }
        default: {
            auto customCmd = static_cast<ChatPacketCommand>(recv->GetCommand());
            if (customCmd == ChatPacketCommand::MESSAGE) {
                // this is a chat message. get a BinaryStream from the Packet, use it to read out the message contents,
                // then prefix the user's nickname to it.
                wirefox::BinaryStream instream = recv->GetStream();
                std::cout << instream.ReadString() << std::endl;
            }
        }
        }

        // keep going while we more incoming data to process
        recv = m_peer->Receive();
    }
}

void ChatClient::Connect(const std::string& host, unsigned short port) {
    if (m_connected) {
        std::cout << "You're already connected." << std::endl;
        return;
    }

    auto attempt = m_peer->Connect(host, port);
    switch (attempt) {
    case wirefox::ConnectAttemptResult::OK:
        std::cout << "Hold on..." << std::endl;
        break;
    case wirefox::ConnectAttemptResult::INVALID_PARAMETER:
    case wirefox::ConnectAttemptResult::INVALID_STATE:
    case wirefox::ConnectAttemptResult::NO_FREE_SLOTS:
        std::cout << "[ERROR] Internal error in ChatClient::Connect: ConnectAttemptResult invalid" << std::endl;
        break;
    case wirefox::ConnectAttemptResult::INVALID_HOSTNAME:
        std::cout << "That host name couldn't be resolved. Check the spelling?" << std::endl;
        break;
    case wirefox::ConnectAttemptResult::ALREADY_CONNECTING:
        std::cout << "Be more patient, jeez." << std::endl;
        break;
    case wirefox::ConnectAttemptResult::ALREADY_CONNECTED:
        std::cout << "You're already connected." << std::endl;
        break;
    }
}

void ChatClient::HandleInput(const std::string& input) {
    if (strncmp(input.c_str(), "/connect ", 9) == 0) {
        if (input.size() < 10) {
            std::cout << "Usage: /connect <host>" << std::endl;
            return;
        }

        std::string hostname = input.substr(9);
        Connect(hostname, m_port);

    } else if (strcmp(input.c_str(), "/dc") == 0 || strcmp(input.c_str(), "/disconnect") == 0) {
        std::cout << "Disconnecting..." << std::endl;
        m_peer->Disconnect(m_server);

    } else {
        // treat as message
        wirefox::BinaryStream outstream;
        outstream.WriteString(input);
        wirefox::Packet packet(static_cast<wirefox::PacketCommand>(ChatPacketCommand::MESSAGE), std::move(outstream));
        m_peer->Send(packet, m_server, wirefox::PacketOptions::RELIABLE, wirefox::PacketPriority::MEDIUM, m_channelChat);
    }
}
