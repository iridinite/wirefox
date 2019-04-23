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

    // split off a separate thread so we can use normal blocking I/O calls for input
    std::thread inputThread(InputThreadWorker);
    std::mt19937 rng(static_cast<std::mt19937::result_type>(time(nullptr)));

    // create a LAN advertisement: this is the data you want other unconnected Peers to see.
    // of course, the format of this message is entirely up to you; this is some random example data.
    wirefox::BinaryStream advert;
    advert.WriteString("Awesome Lobby No. " + std::to_string(rng()));
    advert.WriteString("de_dust2"); // map name
    advert.WriteInt32(rng() % 50); // player count
    advert.WriteInt32(50); // max players

    // set up a peer
    auto peer = wirefox::IPeer::Factory::Create();
    peer->Bind(wirefox::SocketProtocol::IPv6, port);
    peer->SetOfflineAdvertisement(advert); // this call enables ping responses (server advertising)

    // this BinaryStream can be *optionally* released; Wirefox makes a copy of the advert internally
    advert.Reset();

    std::cout << "---- Wirefox LAN Discovery Demo ----" << std::endl;
    std::cout << "(C) Mika Molenkamp, 2019." << std::endl << std::endl;

    std::cout << "Available commands:" << std::endl;
    std::cout << "b   - broadcast to LAN to find peers" << std::endl;
    std::cout << "e   - exit demo" << std::endl;

    while (true) {
        // process Wirefox notifications
        {
            auto recv = peer->Receive();
            while (recv) {
                switch (recv->GetCommand()) {
                case wirefox::PacketCommand::PONG:
                    break;
                default:
                    // there are no other PacketCommands this demo should receive
                    break;
                }

                recv = peer->Receive();
            }
        }

        // check if a new user input string is available
        std::string userInput;
        if (!InputCheckAndPop(userInput)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        if (userInput.empty()) continue;

        // process user commands
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
