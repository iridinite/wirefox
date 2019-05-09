#include "ChatClient.h"

namespace {

    std::string ConnectResultToString(wirefox::ConnectResult r) {
        switch (r) {
        case wirefox::ConnectResult::CONNECT_FAILED:
            return "Connection timed out or contact failed.";
        case wirefox::ConnectResult::INCOMPATIBLE_PROTOCOL:
            return "Communication error: incompatible protocol.";
        case wirefox::ConnectResult::INCOMPATIBLE_VERSION:
            return "Communication error: incompatible Wirefox version.";
        case wirefox::ConnectResult::INCOMPATIBLE_SECURITY:
            return "Communication error: incompatible security settings.";
        case wirefox::ConnectResult::INCORRECT_REMOTE_IDENTITY:
            return "Communication error: unable to verify server identity.";
        case wirefox::ConnectResult::INCORRECT_PASSWORD:
            return "The password is incorrect.";
        case wirefox::ConnectResult::NO_FREE_SLOTS:
            return "The server is full.";
        case wirefox::ConnectResult::ALREADY_CONNECTED:
            return "You are already connected to this server.";
        case wirefox::ConnectResult::IP_RATE_LIMITED:
            return "This IP is being rate limited. Try again later.";
        case wirefox::ConnectResult::BANNED:
            return "You are banned from this server.";
        default:
            return "<?>";
        }
    }

}

ChatClient::ChatClient(unsigned short port)
    : m_server(0)
    , m_port(port)
    , m_connected(false) {
    m_peer = wirefox::IPeer::Factory::Create();
    m_peer->SetNetworkSimulation(0.1f, 5);
    m_peer->SetEncryptionEnabled(true);
    m_peer->Bind(wirefox::SocketProtocol::IPv4, 0); // bind to any port, doesn't matter for client

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

    auto attempt = m_peer->Connect(host, port, m_peer->GetEncryptionEnabled() ? SERVER_KEY_PUBLIC : nullptr);
    switch (attempt) {
    case wirefox::ConnectAttemptResult::OK:
        std::cout << "Hold on..." << std::endl;
        break;
    case wirefox::ConnectAttemptResult::INVALID_PARAMETER:
    case wirefox::ConnectAttemptResult::INVALID_STATE:
    case wirefox::ConnectAttemptResult::NO_FREE_SLOTS:
        std::cout << "[ERROR] Internal error in ChatClient::Connect: ConnectAttemptResult " << std::to_string(int(attempt)) << std::endl;
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

#ifdef _DEBUG
    } else if (strcmp(input.c_str(), "/c") == 0) {
        // connect localhost
        Connect("localhost", m_port);
#endif

    } else if (strcmp(input.c_str(), "/dc") == 0 || strcmp(input.c_str(), "/disconnect") == 0) {
        if (!m_connected) {
            std::cout << "You're not connected to a server." << std::endl;
            return;
        }
        std::cout << "Disconnecting..." << std::endl;
        m_peer->Disconnect(m_server);

    } else {
        assert(input.size() > 0);

        // treat as text message, pack it into a packet and send to server
        wirefox::BinaryStream outstream;
        outstream.WriteString(input);

        wirefox::Packet packet(static_cast<wirefox::PacketCommand>(ChatPacketCommand::MESSAGE), std::move(outstream));
        m_peer->Send(packet, m_server, wirefox::PacketOptions::RELIABLE, wirefox::PacketPriority::MEDIUM, m_channelChat);
    }
}
