/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#pragma once
#include "Socket.h"
#include "RemoteAddress.h"
#include "WirefoxConfig.h"

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
            bool                    Resolve(const std::string& hostname, uint16_t port, RemoteAddress& output) override;
            void                    BeginWrite(const RemoteAddress& addr, const uint8_t* data, size_t datalen, SocketWriteCallback_t callback) override;
            void                    BeginRead(SocketReadCallback_t callback) override;
            bool                    IsReadPending() const override;
            bool                    IsWritePending() const override;

            SocketState             GetState() const override;
            SocketProtocol          GetProtocol() const override;

            bool                    IsOpenAndReady() const override;

        private:
            void                    ThreadWorker();
            asio::ip::udp           GetAsioProtocol() const;

            SocketState             m_state;
            SocketProtocol          m_family;

            asio::io_context        m_context;
            asio::ip::udp::socket   m_socket;
            std::thread             m_socketThread;
            std::atomic_bool        m_socketThreadAbort;
            std::atomic_bool        m_reading;
            std::atomic_bool        m_sending;

            uint8_t                 m_readbuf[cfg::PACKETQUEUE_IN_LEN];
            asio::ip::udp::endpoint m_readsender;
        };

        /// \endcond

    }

}
