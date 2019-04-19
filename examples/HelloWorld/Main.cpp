// This single header wraps the entire public API along with all standard dependencies.
// You'll likely want to put this inside a precompiled header or similar tech.
#include <Wirefox.h>

using namespace wirefox;

// In the client-server model, you'll want to have the server run on a known port, so clients can find it.
// Pure clients, however, can simply bind to port 0, which means the OS will assign a random port for you.
constexpr int SERVER_PORT = 1337;
constexpr int CLIENT_PORT = 0;

int main(int, const char**) {
    // -------- SETUP --------
    // Start up a peer that will act as a listen server
    auto server = IPeer::Factory::Create(10);
    // Bind the peer to a port. This is required for all peers (even clients), otherwise they can't receive data.
    // If Bind() returns false, the port is in use, or the OS is denying you access for some reason.
    if (!server->Bind(SocketProtocol::IPv4, SERVER_PORT)) {
        std::cout << "Server failed to bind to port." << std::endl;
        return EXIT_FAILURE;
    }
    // Allow one incoming connection. By default this value is zero, unless explicitly set like this.
    server->SetMaximumIncomingPeers(1);
    std::cout << "Server setup OK. Server PeerID: " << server->GetMyPeerID() << std::endl;

    // start up a peer that will act as a client
    auto client = IPeer::Factory::Create();
    if (!client->Bind(SocketProtocol::IPv4, CLIENT_PORT)) {
        std::cout << "Client failed to bind to port." << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Client setup OK. Client PeerID: " << client->GetMyPeerID() << std::endl;


    // -------- CONNECTING --------
    // begin connecting the client to the listen server
    auto connectRet = client->Connect("localhost", SERVER_PORT);
    if (connectRet != ConnectAttemptResult::OK) {
        // something went wrong; we could not send a connection request to the server
        std::cout << "Client failed to begin connect: " << int(connectRet) << std::endl;
        return EXIT_FAILURE;
    }


    while (true) {
        std::unique_ptr<Packet> recv;
        bool exit = false;

        // Refer to the documentation pages for more details on how Send() and Receive() work.

        // --------- SERVER RECEIVE ---------
        recv = server->Receive();
        if (recv) {
            // client successfully connected?
            if (recv->GetCommand() == PacketCommand::NOTIFY_CONNECTION_INCOMING) {
                PeerID clientID = recv->GetSender();
                std::cout << "---> Hey, a connection is incoming! Client ID: " << clientID << std::endl;

                // write some example data
                BinaryStream outstream;
                outstream.WriteInt64(1337);
                outstream.WriteString("Hello world!");

                // Create a new network packet using that example payload.
                // Note that using std::move here is optional, but an optimization, as it lets Packet reuse BinaryStream's internal buffer.
                // Further, note that in most practical applications, you would define your own enum that starts at wirefox::USER_PACKET_START.
                Packet packet(PacketCommand::USER_PACKET, std::move(outstream));
                server->Send(packet, clientID, PacketOptions::RELIABLE);
            }
        }

        // --------- CLIENT RECEIVE ---------
        recv = client->Receive();
        if (recv) {
            // GetStream() is a convenience function that gives you a lightweight readonly wrapper stream around the packet payload.
            BinaryStream instream = recv->GetStream();

            switch (recv->GetCommand()) {
            case PacketCommand::NOTIFY_CONNECT_SUCCESS: {
                // Wirefox informs us that our Connect() succeeded
                std::cout << "---> Hey, connection was successful!" << std::endl;
                break;
            }
            case PacketCommand::NOTIFY_CONNECT_FAILED: {
                // Our connection attempt failed. :(
                // Detailed failure reason is encoded as the first byte of the payload. You could of course do more specific
                // handling based on the exact ConnectResult, but for the purposes of this demo, we'll just shut down.
                auto problem = static_cast<ConnectResult>(instream.ReadByte());
                std::cout << "---> Connection failed :( Reason: " << static_cast<int>(problem) << std::endl;

                exit = true;
                break;
            }
            case PacketCommand::USER_PACKET:
                // our custom hello-world packet arrived!
                std::cout << "---> Client received message from server!" << std::endl;
                std::cout << "Number: " << instream.ReadInt64() << std::endl;
                std::cout << "String: " << instream.ReadString() << std::endl;
                exit = true;
                break;
            default:
                break;
            }
        }

        if (exit) break;
    }

    // --------- SHUTDOWN ---------
    client->Stop();
    server->Stop();

    return EXIT_SUCCESS;
}
