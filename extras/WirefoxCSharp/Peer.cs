/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

using System;
using JetBrains.Annotations;

namespace Iridinite.Wirefox {

    /// <summary>
    /// Represents a network peer.
    ///
    /// The IPeer interface is your primary network interface. This object maintains connections at a high level and allows
    /// exchange of data with remote Peers.
    /// </summary>
    [PublicAPI]
    public class Peer : IDisposable {

        private IntPtr m_handle;

        public Peer(int maxPeers) {
            m_handle = NativeMethods.wirefox_peer_create(new UIntPtr((uint)maxPeers));
        }

        ~Peer() {
            Dispose();
        }

        public void Dispose() {
            NativeMethods.wirefox_peer_destroy(m_handle);
            m_handle = IntPtr.Zero;

            GC.SuppressFinalize(this);
        }

        /// <summary>Stops all services of this peer and unbinds it.</summary>
        /// <remarks>
        /// All remote peers are disconnected, and you need to call Bind() again if you want to reuse this instance later.
        /// </remarks>
        /// <param name="linger"></param>
        public void Stop(TimeSpan linger) {
            NativeMethods.wirefox_peer_stop(m_handle, (uint)linger.TotalMilliseconds);
        }

        /// <summary>Bind this peer to a local network interface.</summary>
        /// <remarks>
        /// This function must be called before initiating or accepting new connections. The Peer cannot receive
        /// data from the network unless the peer is bound.After this function is called, you can only use the
        /// protocol family specified in <paramref name="protocol"/> for any incoming or outgoing connections.</remarks>
        /// <param name="protocol">Indicates the socket family to use for this Peer (either IPv4 or IPv6).</param>
        /// <param name="port">Specifies the local port to bind to. Specify 0 to have the system assign a random port.</param>
        /// <returns>True on success; false if the specified port is already in use, or if you have insufficient
        /// system privileges to bind, or if the system is otherwise being difficult.</returns>
        public bool Bind(SocketProtocol protocol, ushort port) {
            return NativeMethods.wirefox_peer_bind(m_handle, protocol, port);
        }

        public ConnectAttemptResult Connect(string host, ushort port) {
            return NativeMethods.wirefox_peer_connect(m_handle, host, port);
        }

        public void Disconnect(PeerID who) {
            Disconnect(who, TimeSpan.FromMilliseconds(200));
        }

        public void Disconnect(PeerID who, TimeSpan linger) {
            NativeMethods.wirefox_peer_disconnect(m_handle, who, (uint)linger.TotalMilliseconds);
        }

        public void DisconnectImmediate(PeerID who) {
            NativeMethods.wirefox_peer_disconnect_immediate(m_handle, who);
        }

        public uint Send(Packet packet, PeerID recipient, PacketOptions options, PacketPriority priority = PacketPriority.MEDIUM) {
            return NativeMethods.wirefox_peer_send(m_handle, packet.GetHandle(), recipient, options, priority, 0);
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

        public Channel MakeChannel(ChannelMode mode) {
            var idx = NativeMethods.wirefox_peer_make_channel(m_handle, mode);
            var channel = new Channel {
                Index = idx,
                Mode = mode
            };
            return channel;
        }

        public ChannelMode GetChannelModeByIndex(byte index) {
            return NativeMethods.wirefox_peer_get_channel_mode(m_handle, index);
        }

        public PeerID GetMyPeerID() {
            return new PeerID(NativeMethods.wirefox_peer_get_my_id(m_handle));
        }

        public int GetMaxPeers() {
            return (int) NativeMethods.wirefox_peer_get_max_peers(m_handle);
        }

        public int GetMaxIncomingPeers() {
            return (int) NativeMethods.wirefox_peer_get_max_incoming_peers(m_handle);
        }

        public void SetMaxIncomingPeers(int incoming) {
            NativeMethods.wirefox_peer_set_max_incoming_peers(m_handle, new UIntPtr((uint)incoming));
        }

        public int GetPing(PeerID who) {
            return (int)NativeMethods.wirefox_peer_get_ping(m_handle, who);
        }

        public bool GetPingAvailable(PeerID who) {
            return NativeMethods.wirefox_peer_get_ping_available(m_handle, who);
        }

        public void SetNetworkSimulator(float packetLoss, uint additionalPing) {
            NativeMethods.wirefox_peer_set_network_sim(m_handle, packetLoss, additionalPing);
        }

    }

}
