#pragma once
#include "Enumerations.h"
#include "RemoteAddress.h"

namespace wirefox {

    /**
     * \brief Private implementation details, not to be used from client application directly.
     */
    namespace detail {

        /**
         * \cond WIREFOX_INTERNAL
         * \brief Represents an abstract network stream.
         * 
         * An interface class that abstracts the underlying byte stream away from the packet management system. This object
         * manages the network connection, and writes / reads raw byte sequences.
         */
        class Socket {
        public:
            /// Indicates the current state of a Socket.
            enum class SocketState {
                /// The socket is unbound and inactive.
                CLOSED,
                /// The socket is bound to a local port.
                OPEN
            };
            /// Represents a callback fired by Socket::BeginRead().
            typedef std::function<void(bool error, RemoteAddress sender, uint8_t* buffer, size_t transferred)> SocketReadCallback_t;

            /// Represents a callback fired by Socket::BeginWrite().
            typedef std::function<void(bool error, size_t transferred)> SocketWriteCallback_t;

            /// Represents a callback fired by Socket::Connect(). Wraps the new client socket to use for this connection.
            typedef std::function<void(bool error, RemoteAddress addr, std::shared_ptr<Socket> socket, std::string errormsg)> SocketConnectCallback_t;

            virtual ~Socket() = default;

            /**
             * \brief Returns the current state of the network socket.
             * \returns An instance of wirefox::SocketState.
             * \sa Socket::IsOpenAndReady
             */
            virtual SocketState GetState() const = 0;

            /**
             * \brief Try connecting to a remote host.
             * 
             * This function attempts to open the socket for the specified remote host. If the attempt was successfully started, the
             * specified callback will be fired later with details on whether or not the socket was opened.
             * 
             * \note Even if this function returns ConnectAttemptResult::OK, and the callback reports no errors, that does not mean the
             * connection is open and stable. Peer must first complete the handshake sequence. See also: Getting Started tutorial page.
             * 
             * \param[in]   host    A string representation of the remote endpoint. May be either a hostname, or an IPv4 or IPv6 address.
             * \param[in]   port    The network port to attempt to connect to. The remote endpoint must be listening on this port.
             * \param[in]   callback    Callback that is executed later, \b if this function returns ConnectAttemptResult::OK.
             */
            virtual ConnectAttemptResult Connect(const std::string& host, unsigned short port, SocketConnectCallback_t callback) = 0;

            /**
             * \brief Disconnect from a remote host.
             * 
             * Any pending reads or writes will fail, and any incoming data that is still in transit will be lost.
             * 
             * \note For UDP, this function does nothing, because UDP is connectionless.
             */
            virtual void Disconnect() = 0;

            /**
             * \brief Resets the socket.
             *
             * Immediately closes the socket and resets the network stream. Any pending reads or writes will fail, and any incoming
             * data that is still in transit will be lost, as if Disconnect() was called.
             * 
             * After calling this, Bind() needs to be called again before normal operation can resume.
             */
            virtual void Unbind() = 0;

            /**
             * \brief Bind the socket to a local port.
             * 
             * The socket will bind to the specified port and interface. For UDP, this means it will receive incoming datagrams.
             * For TCP, this means the socket can listen to, and accept, incoming connection requests.
             */
            virtual bool Bind(SocketProtocol family, unsigned short port) = 0;

            /**
             * \brief Send data to a remote endpoint.
             * 
             * Asynchronously writes data to the network stream. The given callback, if not nullptr, will be fired on completion.
             * 
             * \note        You must ensure that the data buffer you pass in will remain valid up until the callback is fired.
             * 
             * \param[in]   addr        The remote endpoint to send data to.
             * \param[in]   data        A pointer to the data to write.
             * \param[in]   datalen     The length of the buffer represented by \p data.
             * \param[in]   callback    A callback to fire on completion of the write (whether it succeeded or not).
             */
            virtual void BeginWrite(const RemoteAddress& addr, const uint8_t* data, size_t datalen, SocketWriteCallback_t callback) = 0;

            /**
             * \brief Read data from a remote endpoint.
             * 
             * Asynchronously receives data from the network stream. The given callback, if not nullptr, will be fired on completion.
             * Note that this function will automatically keep restarting (i.e. you should not call BeginRead again), using the same
             * callback you passed the first time, until the socket is closed.
             * 
             * \param[in]   callback    A callback to fire on completion of the read (whether it succeeded or not).
             */
            virtual void BeginRead(SocketReadCallback_t callback) = 0;

            /**
             * \brief Indicates whether an async receive operation is currently pending.
             * 
             * If this function returns true, you should not attempt to begin a new receive op (Socket::BeginRead()).
             */
            virtual bool IsReadPending() const = 0;

            /**
             * \brief Indicates whether an async send operation is currently pending.
             *
             * If this function returns true, you should not attempt to begin a new send op (Socket::BeginWrite()).
             */
            virtual bool IsWritePending() const = 0;

            /**
             * \brief Check if the socket is open and ready to write or read data.
             * 
             * Note that this function does not guarantee that the socket is actually maintaining a connection to a remote endpoint,
             * and only indicates whether it, by itself, is ready to work.
             */
            virtual bool IsOpenAndReady() const = 0;
        };

        /// \endcond

    }

}
