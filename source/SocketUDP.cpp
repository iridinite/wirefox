#include "PCH.h"
#include "SocketUDP.h"

using namespace asio::ip;
using namespace wirefox::detail;

SocketUDP::SocketUDP()
    : m_state(SocketState::CLOSED)
    , m_family()
    , m_socket(m_context)
    , m_readbuf{}
    , m_reading(false) {}

std::shared_ptr<Socket> SocketUDP::Create() {
    // Use a factory method like this to allow safe usage of std::shared_from_this, which I need because
    // SocketConnectCallback_t should return a shared_ptr<Socket>. It should return this same instance
    // because UDP doesn't generate more sockets like a TCP acceptor does.
    // https://en.cppreference.com/w/cpp/memory/enable_shared_from_this
    return std::shared_ptr<SocketUDP>(new SocketUDP);
}

SocketUDP::~SocketUDP() {
    Disconnect();
    Unbind();

    m_context.stop();
}

ConnectAttemptResult SocketUDP::Connect(const std::string& host, const unsigned short port, SocketConnectCallback_t callback) {
    if (GetState() != SocketState::OPEN || !IsOpenAndReady())
        return ConnectAttemptResult::INVALID_STATE;

    // make sure input parameters are not nonsensical
    if (host.empty() || port == 0)
        return ConnectAttemptResult::INVALID_PARAMETER;

    // pick the desired IP version
    const auto protocol = GetAsioProtocol();

    // attempt to resolve the given hostname into an endpoint
    udp::endpoint endpoint;
    udp::resolver resolver(m_context);
    try {
        udp::resolver::iterator it = resolver.resolve(protocol, host, std::to_string(port));
        if (it == udp::resolver::iterator() /* end */)
            return ConnectAttemptResult::INVALID_HOSTNAME;

        endpoint = *it;

    } catch (const asio::system_error&) {
        return ConnectAttemptResult::INVALID_HOSTNAME;
    }

    // for simplicity, assume the first endpoint is going to work
    RemoteAddress addr;
    addr.endpoint_udp = endpoint;

    assert(callback);
    m_context.post(std::bind(callback, false, addr, shared_from_this(), std::string()));

    return ConnectAttemptResult::OK;
}

void SocketUDP::Disconnect() {
    // Not implemented for UDP. Connections are managed on a RemotePeer level instead.
}

void SocketUDP::Unbind() {
    if (!m_socket.is_open()) return;

    m_socket.shutdown(udp::socket::shutdown_both);
    m_socket.cancel();
    m_socket.close();
    m_state = SocketState::CLOSED;
}

bool SocketUDP::Bind(const SocketProtocol family, const unsigned short port) {
    // socket should be inactive and unbound
    if (GetState() != SocketState::CLOSED) return false;

    // pick the desired IP version
    m_family = family;
    const auto protocol = GetAsioProtocol();

    // close and reopen the socket to clear state and change to the new protocol (in case it changed)
    try {
        m_socket.close();
        m_socket.open(protocol);
        m_socket.bind(udp::endpoint(protocol, port));

    } catch (const asio::system_error&) {
        return false;
    }

    m_state = SocketState::OPEN;

    return true;
}

void SocketUDP::BeginWrite(const RemoteAddress& addr, const uint8_t* data, size_t datalen, SocketWriteCallback_t callback) {
    m_socket.async_send_to(asio::buffer(data, datalen),
        addr.endpoint_udp,
        [callback](const asio::error_code& error, size_t bytes_transferred) -> void {
#if _DEBUG
            if (error)
                std::cerr << "ERROR IN ASIO: " << error << " --> " << error.message() << std::endl;
#endif

            assert(callback);
            callback(static_cast<bool>(error), bytes_transferred);
        }
    );
}

void SocketUDP::BeginRead(SocketReadCallback_t callback) {
    m_reading.store(true);
    m_socket.async_receive_from(asio::buffer(m_readbuf, cfg::PACKETQUEUE_IN_LEN),
        m_readsender, // will be filled in with the sender address of the incoming datagram
        [&, callback](const asio::error_code& error, size_t bytes_transferred) -> void {
#if _DEBUG
            if (error)
                std::cerr << "ERROR IN ASIO: " << error << " --> " << error.message() << std::endl;
#endif

            // wrap the UDP endpoint in a RemoteAddress, to help somewhat conceal the ASIO implementation detail
            RemoteAddress addr;
            addr.endpoint_udp = m_readsender;

            // pass the read data to the subscriber (probably PacketQueue)
            assert(callback);
            callback(static_cast<bool>(error), addr, m_readbuf, bytes_transferred);

            // automatically and immediately restart the read cycle using the same callback
            if (!error && IsOpenAndReady())
                BeginRead(callback);
            else
                m_reading.store(false);
        }
    );
}

bool SocketUDP::IsReadPending() const {
    return m_reading.load();
}

void SocketUDP::RunCallbacks() {
    if (m_context.stopped())
        m_context.restart();

    // run pending callbacks, but in a non-blocking manner (return if none exist)
    m_context.poll();
}

Socket::SocketState SocketUDP::GetState() const {
    return m_state;
}

bool SocketUDP::IsOpenAndReady() const {
    return m_socket.is_open();
}

udp SocketUDP::GetAsioProtocol() const {
    if (m_family == SocketProtocol::IPv4)
        return udp::v4();

    return udp::v6();
}
