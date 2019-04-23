#include <cstring>
#include <Wirefox.h>

constexpr static uint16_t LAN_DEFAULT_PORT = 51234;

// -- async console input, taken from DemoChat --

std::atomic<bool> g_inputStop(false);
std::queue<std::string> g_inputQueue;
std::mutex g_inputMutex;

void InputThreadWorker() {
    while (!g_inputStop) {
        std::string line;
        std::getline(std::cin, line);

        std::lock_guard<std::mutex> guard(g_inputMutex);
        g_inputQueue.push(std::string(line));

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

bool InputCheckAndPop(std::string& out) {
    std::lock_guard<std::mutex> guard(g_inputMutex);
    if (g_inputQueue.empty()) return false;

    out = g_inputQueue.front();
    g_inputQueue.pop();
    return true;
}

// -- end async input --

int main(int argc, const char** argv) {
    unsigned short port = LAN_DEFAULT_PORT;
    std::mt19937 rng(static_cast<std::mt19937::result_type>(time(nullptr)));

    // parse command-line arguments
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-port") == 0) {
            // explicit port specification
            i++;
            if (i >= argc) {
                std::cerr << "Error: -port option is missing argument" << std::endl;
                return EXIT_FAILURE;
            }

            port = static_cast<unsigned short>(atoi(argv[i]));
        }
    }

    // Split off a separate thread so we can use normal blocking I/O calls for input
    std::thread inputThread(InputThreadWorker);

    // Generate a random name; this is just for aesthetics, and making it easier to sift through long lists
    static const std::string adjectives[] = {"Awesome", "Glorious", "Wonderful", "Brilliant", "Shiny"};
    static constexpr auto adjectivesCount = sizeof adjectives / sizeof adjectives[0];
    std::string myLobbyName = adjectives[rng() % adjectivesCount] + " Lobby No. " + std::to_string(rng());

    // Create a LAN advertisement: this is the data you want other unconnected Peers to see.
    // Of course, the format of this message is entirely up to you; this is some random example data.
    wirefox::BinaryStream advert;
    advert.WriteString(myLobbyName);
    advert.WriteString("de_dust2"); // map name
    advert.WriteInt32(rng() % 50); // player count
    advert.WriteInt32(50); // max players

    // Set up a Wirefox peer
    auto peer = wirefox::IPeer::Factory::Create();
    peer->Bind(wirefox::SocketProtocol::IPv4, port);
    peer->SetOfflineAdvertisement(advert); // this call enables ping responses (server advertising)

    // This BinaryStream can be *optionally* released; Wirefox makes a copy of the advert internally
    advert.Reset();

    std::cout << "---- Wirefox LAN Discovery Demo ----" << std::endl;
    std::cout << "(C) Mika Molenkamp, 2019." << std::endl << std::endl;

    std::cout << "We are hosting lobby: " << myLobbyName << std::endl << std::endl;
    std::cout << "Available commands:" << std::endl;
    std::cout << "b   - broadcast to LAN to find peers" << std::endl;
    std::cout << "e   - exit demo" << std::endl;

    while (true) {
        // Process Wirefox notifications
        {
            auto recv = peer->Receive();
            while (recv) {
                switch (recv->GetCommand()) {
                // Received a pong from another system! let's print the packet contents
                case wirefox::PacketCommand::NOTIFY_ADVERTISEMENT: {
                    auto instream = recv->GetStream();

                    // The first item is a string (hostname or IP) that you can use to Connect() to this sender if you want
                    auto hostname = instream.ReadString();

                    // Everything else is the data that you passed to SetOfflineAdvertisement() on the remote end
                    auto nameLobby = instream.ReadString();
                    auto nameMap = instream.ReadString();
                    auto playerCount = instream.ReadInt32();
                    auto playerMax = instream.ReadInt32();

                    std::cout << hostname << " | " << nameLobby << " | " << nameMap << " | " <<
                        std::to_string(playerCount) << "/" << std::to_string(playerMax) << " players" << std::endl;

                    break;
                }
                default:
                    // there are no other PacketCommands this demo should receive
                    break;
                }

                recv = peer->Receive();
            }
        }

        // Check if a new user input string is available
        std::string userInput;
        if (!InputCheckAndPop(userInput)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        if (userInput.empty()) continue;

        // Process user commands
        if (strcmp(userInput.c_str(), "e") == 0) {
            g_inputStop = true;
            break;
        }
        if (strcmp(userInput.c_str(), "b") == 0) {
            peer->PingLocalNetwork(port);
            std::cout << "Broadcast sent." << std::endl;
        }
    }

    if (inputThread.joinable())
        inputThread.join();

    return EXIT_SUCCESS;
}
