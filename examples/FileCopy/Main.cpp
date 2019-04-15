#include <Wirefox.h>
#include <fstream>
#include <set>
#include <cstring>

using namespace wirefox;

constexpr static size_t CHUNKS_IN_FLIGHT = 64;
constexpr static size_t CHUNK_SIZE = 1024;
static_assert(CHUNK_SIZE <= (wirefox::cfg::MTU - 100), "CHUNK_SIZE must leave room for packet headers");

enum class SendMode {
    UNSET,
    SEND,
    RECEIVE
};

enum class CustomCommand : uint8_t {
    FILE_BEGIN = USER_PACKET_START,
    FILE_CHUNK,
    FILE_END
};

void StartSender(const std::string& filename, const std::string& receiver, uint16_t port) {
    std::ifstream infile(filename, std::ios::binary | std::ios::ate);
    if (!infile.is_open()) {
        std::cerr << "Sender: Failed to open file: " << filename << std::endl;
        return;
    }

    // determine file length and chunk count
    size_t filelen = static_cast<size_t>(infile.tellg());
    size_t chunksTotal = (filelen / CHUNK_SIZE) + 1;
    size_t chunksSent = 0;
    infile.seekg(0, std::ios::beg);

    // set up Wirefox peer
    auto peer = IPeer::Factory::Create();
    peer->Bind(SocketProtocol::IPv4, 0);
    peer->SetNetworkSimulation(0.05f, 1);

    // connect to a receiver
    PeerID receiverID = 0;
    auto channel = peer->MakeChannel(ChannelMode::ORDERED);
    auto connect = peer->Connect(receiver, port);
    if (connect != ConnectAttemptResult::OK) {
        std::cerr << "Sender: Failed to connect: " << static_cast<int>(connect) << std::endl;
        return;
    }

    std::set<PacketID> awaiting;

    auto timeStart = std::chrono::steady_clock::now();
    bool eofPacketExpected = false; // because there is no 'unset' PacketID value
    PacketID eofPacketID = 0;

    // program loop
    while (true) {
        // to help avoid burning cpu cycles / spinlocking as we wait for stuff to arrive
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        // receiver loop
        auto recv = peer->Receive();
        while (recv) {
            switch (recv->GetCommand()) {
            case PacketCommand::NOTIFY_CONNECT_SUCCESS:
                std::cout << "Sender: Connect successful." << std::endl;
                receiverID = recv->GetSender();
                {
                    // Send the receiver a packet to initiate file transfer
                    BinaryStream outstream;
                    outstream.WriteString(filename);
                    outstream.WriteInt64(filelen);
                    Packet packet(static_cast<PacketCommand>(CustomCommand::FILE_BEGIN), std::move(outstream));
                    peer->Send(packet, receiverID, PacketOptions::RELIABLE, PacketPriority::MEDIUM, channel);
                }
                break;
            case PacketCommand::NOTIFY_CONNECT_FAILED:
                std::cout << "Sender: Connect failed." << std::endl;
                return;
            case PacketCommand::NOTIFY_CONNECTION_LOST:
            case PacketCommand::NOTIFY_DISCONNECTED:
                std::cout << "Sender: Connection closed." << std::endl;
                return;
            case PacketCommand::NOTIFY_RECEIPT_ACKED: {
                // Find the packetID mentioned in this receipt, and remove it from the awaiting list.
                auto instream = recv->GetStream();
                PacketID id = instream.ReadUInt32();

                // we finished all delivery
                if (eofPacketExpected && id == eofPacketID) {
                    auto timeElapsed = std::chrono::steady_clock::now() - timeStart;
                    std::cout << "Sender: All packets delivered; disconnecting." << std::endl;
                    std::cout << "Sender: Time elapsed: " << std::chrono::duration_cast<std::chrono::milliseconds>(timeElapsed).count() << " ms" << std::endl;

                    // TODO: Disconnect currently immediately wipes the outbox and can cause some packets to not arrive
                   // peer->Disconnect(receiverID);
                    break;
                }

                // otherwise, this ack is about one of the file chunks
                chunksSent++;
                if (chunksSent % CHUNKS_IN_FLIGHT == 0) {
                    std::cout << "Sender: Progress: " << (chunksSent * 100 / chunksTotal) << "% (" << chunksSent << " / " << chunksTotal << ")" << std::endl;
                }
                //std::cout << "Sender: Ack chunk " << id << std::endl;
                auto it = awaiting.find(id);
                if (it != awaiting.end())
                    awaiting.erase(it);

                break;
            }
            case PacketCommand::NOTIFY_RECEIPT_LOST:
                // If this assert trips, a packet was lost. In this demo, that means the entire connection is
                // lost, because losing a reliable packet == dead connection (unlike unreliable loss).
                assert(false);
                break;
            default:
                break;
            }

            recv = peer->Receive();
        }

        // if receiverID is set, then we must've connected, so send file chunks
        while (receiverID && awaiting.size() < CHUNKS_IN_FLIGHT && !infile.eof()) {
            // produce a new chunk
            char chunk[CHUNK_SIZE];
            memset(chunk, 0, CHUNK_SIZE);
            infile.read(chunk, CHUNK_SIZE);

            // fire ze missiles
            BinaryStream outstream(CHUNK_SIZE);
            outstream.WriteInt16(static_cast<uint16_t>(infile.gcount()));
            outstream.WriteBytes(chunk, CHUNK_SIZE);
            Packet packet(static_cast<PacketCommand>(CustomCommand::FILE_CHUNK), std::move(outstream));
            auto packetID = peer->Send(packet, receiverID, PacketOptions::RELIABLE | PacketOptions::WITH_RECEIPT, PacketPriority::MEDIUM, channel);

            // track this packet ID, so Wirefox can tell us when it's been delivered
            awaiting.emplace(packetID);

            // Send out the EOF packet if we ran out of bytes to send
            if (infile.eof()) {
                std::cout << "Sender: Progress: 100%" << std::endl;
                Packet packetEof(static_cast<PacketCommand>(CustomCommand::FILE_END), nullptr, 0);
                eofPacketID = peer->Send(packetEof, receiverID, PacketOptions::RELIABLE | PacketOptions::WITH_RECEIPT, PacketPriority::MEDIUM, channel);
                eofPacketExpected = true;
            }
        }
    }
}

void StartReceiver(uint16_t port) {
    // set up Wirefox peer
    auto peer = IPeer::Factory::Create();
    peer->Bind(SocketProtocol::IPv4, port);
    peer->SetMaximumIncomingPeers(1);
    peer->SetNetworkSimulation(0.05f, 1);
    peer->MakeChannel(ChannelMode::ORDERED); // result is not used, but channel needs to be registered at least

    std::ofstream outfile;
    size_t filelen = 0;
    size_t filerecv = 0;

    while (true) {
        // to help avoid burning cpu cycles / spinlocking as we wait for stuff to arrive
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        auto recv = peer->Receive();
        while (recv) {
            switch (recv->GetCommand()) {
            case PacketCommand::NOTIFY_CONNECTION_INCOMING:
                // In this demo, we don't actually care about who we're connected to.
                std::cout << "Receiver: Incoming connection." << std::endl;
                break;
            case PacketCommand::NOTIFY_CONNECTION_LOST:
            case PacketCommand::NOTIFY_DISCONNECTED:
                std::cout << "Receiver: Connection closed." << std::endl;
                // close the file
                if (outfile.is_open()) {
                    outfile.flush();
                    outfile.close();
                }
                return;
            default: {
                // I wish there were a more expressive way to do this in C++. Ideally you'd put both
                // the PacketCommand and FooUserCommand enums in one switch, but that is not allowed.
                auto usercmd = static_cast<CustomCommand>(recv->GetCommand());
                switch (usercmd) {
                case CustomCommand::FILE_BEGIN: {
                    auto instream = recv->GetStream();
                    auto filename = "recv_" + instream.ReadString();
                    filelen = instream.ReadUInt64();
                    std::cout << "Receiver: Begin new file: " << filename << " (" << (filelen / 1024) << " kB)" << std::endl;

                    if (outfile.is_open()) {
                        outfile.flush();
                        outfile.close();
                    }

                    outfile.open(filename, std::ios::binary);
                    break;
                }
                case CustomCommand::FILE_CHUNK: {
                    char buffer[CHUNK_SIZE];

                    // read the chunk length
                    auto instream = recv->GetStream();
                    auto length = instream.ReadUInt16();
                    assert(length <= CHUNK_SIZE);
                    filerecv += length;

                    // read the chunk from the packet and write it to the output file
                    instream.ReadBytes(reinterpret_cast<uint8_t*>(buffer), length);
                    outfile.write(buffer, length);
                    //std::cout << "Receiver: Received chunk of length " << length << std::endl;
                    break;
                }
                case CustomCommand::FILE_END:
                    std::cout << "Receiver: All packets received, closing file." << std::endl;
                    std::cout << "Receiver: Received " << filerecv << " of " << filelen << " bytes." << std::endl;
                    if (outfile.is_open()) {
                        outfile.flush();
                        outfile.close();
                    }

                    peer->Disconnect(recv->GetSender());
                    return;
                }
            }
            }

            recv = peer->Receive();
        }
    }
}

int main(int argc, const char** argv) {
    uint16_t recvPort = 51234;
    SendMode mode = SendMode::UNSET;
    std::string filename;
    std::string sendTo("localhost");

    // parse command-line arguments
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-recv") == 0) {
            // explicitly set to server (client by default)
            mode = SendMode::RECEIVE;
        } else if (strcmp(argv[i], "-send") == 0) {
            i++;
            if (i >= argc) {
                std::cerr << "Error: -send option is missing argument" << std::endl;
                return EXIT_FAILURE;
            }

            mode = SendMode::SEND;
            filename = argv[i];
        } else if (strcmp(argv[i], "-to") == 0) {
            i++;
            if (i >= argc) {
                std::cerr << "Error: -to option is missing argument" << std::endl;
                return EXIT_FAILURE;
            }

            sendTo = argv[i];
        } else if (strcmp(argv[i], "-port") == 0) {
            // explicit port specification
            i++;
            if (i >= argc) {
                std::cerr << "Error: -port option is missing argument" << std::endl;
                return EXIT_FAILURE;
            }

            recvPort = static_cast<unsigned short>(atoi(argv[i]));
        }
    }

#ifdef _DEBUG
    // Start a server also, but as a separate thread. This is super hacky, but lets me debug both
    // server & client in the same Visual Studio session. Obviously this isn't needed for your usage.
    if (mode == SendMode::RECEIVE) {
        const char* serverarg[] = {"-send", "dummy2.bin", "-to", "localhost"};
        std::thread serverThread(main, 4, serverarg);
        serverThread.detach();
    }
#endif

    switch (mode) {
    case SendMode::RECEIVE:
        StartReceiver(recvPort);
        break;
    case SendMode::SEND:
        StartSender(filename, sendTo, recvPort);
        break;
    default:
        std::cout << "Usage:" << std::endl;
        std::cout << "DemoFileCopy -recv [-port <x>]" << std::endl;
        std::cout << "DemoFileCopy -send <filename> -to <host>" << std::endl;
        break;
    }

    return EXIT_SUCCESS;
}
