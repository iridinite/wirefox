using System;
using JetBrains.Annotations;

namespace Iridinite.Wirefox {

    [PublicAPI]
    public class Peer : IDisposable {

        private IntPtr m_handle;

        public Peer(uint maxPeers) {
            m_handle = NativeMethods.wirefox_peer_create(maxPeers);
        }

        ~Peer() {
            Dispose();
        }

        public void Dispose() {
            NativeMethods.wirefox_peer_destroy(m_handle);
            m_handle = IntPtr.Zero;

            GC.SuppressFinalize(this);
        }

        public void Stop() {
            // TODO
        }

        public bool Bind(SocketProtocol protocol, ushort port) {
            return NativeMethods.wirefox_peer_bind(m_handle, protocol, port);
        }

        public ConnectAttemptResult Connect(string host, ushort port) {
            return NativeMethods.wirefox_peer_connect(m_handle, host, port);
        }

        public bool Send(Packet packet, PeerID recipient, PacketOptions options) {
            return NativeMethods.wirefox_peer_send(m_handle, packet.GetHandle(), recipient, options);
        }

        public void SendLoopback(Packet packet) {
            NativeMethods.wirefox_peer_send_loopback(m_handle, packet.GetHandle());
        }

        public Packet Receive() {
            var packetHandle = NativeMethods.wirefox_peer_receive(m_handle);
            return packetHandle == IntPtr.Zero
                ? null
                : new Packet(packetHandle);
        }

    }

}
