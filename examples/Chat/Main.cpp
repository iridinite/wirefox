#include "Shared.h"
#include "ChatServer.h"
#include "ChatClient.h"

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

int main(int argc, const char** argv) {

    unsigned short port = CHAT_DEFAULT_PORT;
    IChat* chat = nullptr;

    // parse command-line arguments
    bool isClient = true;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-server") == 0) {
            // explicitly set to server (client by default)
            isClient = false;

        } else if (strcmp(argv[i], "-port") == 0) {
            // explicit port specification
            i++;
            if (i >= argc) {
                std::cerr << "Error: -port option is missing argument" << std::endl;
                return EXIT_FAILURE;
            }

            port = static_cast<unsigned short>(atoi(argv[i]));
        }
    }

    // some welcome text
    std::cout << "---- Wirefox Chat Demo ---" << std::endl;
    std::cout << "(C) Mika Molenkamp, 2019." << std::endl << std::endl;

    if (isClient) {
        std::cout << "Available commands:" << std::endl;
        std::cout << "/connect <host>    -  connect to a chat server" << std::endl;
        std::cout << "/exit              -  quit the chat client" << std::endl;
        std::cout << "/list              -  if connected, list other connected users" << std::endl;
        std::cout << "/nick <name>       -  if connected, change your nickname" << std::endl;
        std::cout << std::endl;

    } else {
        std::cout << "Server mode." << std::endl;
    }

    // split off a separate thread so we can use normal blocking I/O calls for input
    std::thread recv(InputThreadWorker);
    recv.detach();

    if (isClient)
        chat = new ChatClient(port);
    else
        chat = new ChatServer(port);

#ifdef _DEBUG
    // Start a server also, but as a separate thread. This is super hacky, but lets me debug both
    // server & client in the same Visual Studio session. Obviously this isn't needed for your usage.
    if (isClient) {
        const char* serverarg[] = { "-server" };
        std::thread serverThread(main, 1, serverarg);
        serverThread.detach();
    }
#endif

    while (true) {
        // periodic updates (receiving packets)
        chat->Tick();

#if _DEBUG
        if (!isClient) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
#endif

        // check if a new user input string is available
        std::string userInput;
        if (!InputCheckAndPop(userInput)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        if (userInput.empty()) continue;

        // some special command(s) that don't pertain to the chat
        if (strcmp(userInput.c_str(), "/exit") == 0) {
            g_inputStop = true;
            break;
        }

        // pass off the string to the client or server
        chat->HandleInput(userInput);
    }

    delete chat;

    if (recv.joinable())
        recv.join();

    return EXIT_SUCCESS;
}
