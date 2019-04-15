#include <catch2/catch.hpp>
#include <Wirefox.h>

/// Connect target should be easily editable
static constexpr const char* LOCALHOST = "127.0.0.1";

TEST_CASE("Peer can bind to exact port", "[Peer]") {
    auto p = wirefox::IPeer::Factory::Create();
    bool success = p->Bind(wirefox::SocketProtocol::IPv4, 1337);
    REQUIRE(success);
}

TEST_CASE("Peer can bind to zero port", "[Peer]") {
    auto p = wirefox::IPeer::Factory::Create();
    bool success = p->Bind(wirefox::SocketProtocol::IPv4, 0);
    REQUIRE(success);
}

TEST_CASE("Peer cannot double-bind to port", "[Peer]") {
    auto a = wirefox::IPeer::Factory::Create();
    auto b = wirefox::IPeer::Factory::Create();

    SECTION("if the port is the same") {
        REQUIRE(a->Bind(wirefox::SocketProtocol::IPv4, 1337));
        REQUIRE(!b->Bind(wirefox::SocketProtocol::IPv4, 1337));
    }
    SECTION("unless the port is zero") {
        REQUIRE(a->Bind(wirefox::SocketProtocol::IPv4, 0));
        REQUIRE(b->Bind(wirefox::SocketProtocol::IPv4, 0));
    }
}

TEST_CASE("Peer connectivity", "[Peer]") {
    auto a = wirefox::IPeer::Factory::Create(1);
    auto b = wirefox::IPeer::Factory::Create(1);
    REQUIRE(a->Bind(wirefox::SocketProtocol::IPv4, 1337));
    REQUIRE(b->Bind(wirefox::SocketProtocol::IPv4, 0));

    SECTION("Can connect to each other") {
        REQUIRE(b->Connect(LOCALHOST, 1337) == wirefox::ConnectAttemptResult::OK);

        wirefox::PeerID b_to_a;
        auto timeout = wirefox::Time::Now() + wirefox::Time::FromSeconds(5);
        while (true) {
            if (wirefox::Time::Elapsed(timeout)) {
                FAIL("Connection timed out");
                return;
            }

            auto packet = b->Receive();
            if (packet) {
                REQUIRE(packet->GetCommand() == wirefox::PacketCommand::NOTIFY_CONNECT_SUCCESS);
                b_to_a = packet->GetSender();
                break;
            }
        }

        SECTION("Can exchange trivial data") {
            wirefox::BinaryStream payload;
            payload.WriteInt32(12345678);
            wirefox::Packet message(wirefox::PacketCommand::USER_PACKET, std::move(payload));

            b->Send(message, b_to_a, wirefox::PacketOptions::RELIABLE);

            timeout = wirefox::Time::Now() + wirefox::Time::FromSeconds(5);
            while (true) {
                if (wirefox::Time::Elapsed(timeout)) {
                    FAIL("Connection timed out");
                    return;
                }

                auto packet = a->Receive();
                if (packet) {
                    if (packet->GetCommand() == wirefox::PacketCommand::NOTIFY_CONNECTION_INCOMING) {
                        REQUIRE(packet->GetSender() == b->GetMyPeerID());
                    } else if (packet->GetCommand() == wirefox::PacketCommand::USER_PACKET) {
                        wirefox::BinaryStream instream = packet->GetStream();
                        REQUIRE(instream.ReadInt32() == 12345678);
                        break;
                    } else {
                        FAIL(std::to_string(static_cast<int>(packet->GetCommand())));
                        break;
                    }
                }
            }
            auto packet = a->Receive();
            while (packet) {
            }
        }
    }
}
