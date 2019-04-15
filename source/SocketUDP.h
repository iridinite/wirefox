#pragma once
#include "Socket.h"
#include "RemoteAddress.h"

namespace wirefox {

    namespace detail {

        /**
         * \cond WIREFOX_INTERNAL
         * \brief Represents a Socket implemented using UDP (User Datagram Protocol) as backing protocol.
         */
        class SocketUDP final
            : public Socket
            , public std::enable_shared_from_this<SocketUDP> {
        protected:
            SocketUDP();

        public:
            /// Constructs and initializes a new SocketUDP instance. Use this (as \p cfg::DefaultSocket::Create() ) rather than
            /// calling the constructor (or operator new) manually.
            static std::shared_ptr<Socket> Create();

            ~SocketUDP();

            ConnectAttemptResult    Connect(const std::string& host, unsigned short port, SocketConnectCallback_t callback) override;
            void                    Disconnect() override;
            void                    Unbind() override;
            bool                    Bind(SocketProtocol family, unsigned short port) override;
            void                    BeginWrite(const RemoteAddress& addr, const uint8_t* data, size_t datalen, SocketWriteCallback_t callback) override;
            void                    BeginRead(SocketReadCallback_t callback) override;
            bool                    IsReadPending() const override;
            void                    RunCallbacks() override;

            SocketState             GetState() const override;

            bool                    IsOpenAndReady() const override;

        private:
            asio::ip::udp           GetAsioProtocol() const;

            SocketState             m_state;
            std::string             m_host;
            unsigned short          m_port;

            asio::io_context        m_context;
            asio::ip::udp::socket   m_socket;
            SocketProtocol          m_family;

            uint8_t                 m_readbuf[cfg::PACKETQUEUE_IN_LEN];
            std::atomic_bool        m_reading;
            asio::ip::udp::endpoint m_readsender;

#ifdef WIREFOX_ENABLE_SOCKET_LOCK
            LockableMutex           m_lock;
#endif
        };

        /// \endcond

    }

}
