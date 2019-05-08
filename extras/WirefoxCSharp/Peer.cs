/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

using System;
using System.Runtime.InteropServices;
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
            m_handle = NativeMethods.wirefox_peer_create(new UIntPtr((uint) maxPeers));
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
            NativeMethods.wirefox_peer_stop(m_handle, (uint) linger.TotalMilliseconds);
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
            NativeMethods.wirefox_peer_disconnect(m_handle, who, (uint) linger.TotalMilliseconds);
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
            NativeMethods.wirefox_peer_set_max_incoming_peers(m_handle, new UIntPtr((uint) incoming));
        }

        public int GetPing(PeerID who) {
            return (int) NativeMethods.wirefox_peer_get_ping(m_handle, who);
        }

        public bool GetPingAvailable(PeerID who) {
            return NativeMethods.wirefox_peer_get_ping_available(m_handle, who);
        }

        public void SetNetworkSimulator(float packetLoss, uint additionalPing) {
            NativeMethods.wirefox_peer_set_network_sim(m_handle, packetLoss, additionalPing);
        }

        public void SetOfflineAdvertisement([NotNull] byte[] ad) {
            // to pass the buffer to unmanaged code, we need to memcpy it to an unmanaged buffer
            var length = ad.Length;
            var buffer = IntPtr.Zero;
            try {
                buffer = Marshal.AllocHGlobal(length);
                Marshal.Copy(ad, 0, buffer, length);
                NativeMethods.wirefox_peer_set_offline_ad(m_handle, buffer, new UIntPtr((uint) length));
            } finally {
                // always free the buffer, even if we error out
                if (buffer != IntPtr.Zero)
                    Marshal.FreeHGlobal(buffer);
            }
        }

        public void DisableOfflineAdvertisement() {
            NativeMethods.wirefox_peer_disable_offline_ad(m_handle);
        }

        public void Ping(string host, ushort port) {
            NativeMethods.wirefox_peer_ping(m_handle, host, port);
        }

        public void PingLocalNetwork(ushort port) {
            NativeMethods.wirefox_peer_ping_local_network(m_handle, port);
        }

        public int GetEncryptionKeyLength() {
            return (int) NativeMethods.wirefox_peer_get_crypto_key_length(m_handle);
        }

        public bool GetEncryptionEnabled() {
            return NativeMethods.wirefox_peer_get_crypto_enabled(m_handle);
        }

        public void SetEncryptionEnabled(bool enabled) {
            NativeMethods.wirefox_peer_set_crypto_enabled(m_handle, enabled);
        }

        public void SetEncryptionIdentity([NotNull] byte[] keySecret, [NotNull] byte[] keyPublic) {
            var length = GetEncryptionKeyLength();
            var bufferS = IntPtr.Zero;
            var bufferP = IntPtr.Zero;
            try {
                bufferS = Marshal.AllocHGlobal(length);
                bufferP = Marshal.AllocHGlobal(length);
                Marshal.Copy(keySecret, 0, bufferS, length);
                Marshal.Copy(keyPublic, 0, bufferP, length);

                NativeMethods.wirefox_peer_set_crypto_identity(m_handle, bufferS, bufferP);
            } finally {
                if (bufferS != IntPtr.Zero) Marshal.FreeHGlobal(bufferS);
                if (bufferP != IntPtr.Zero) Marshal.FreeHGlobal(bufferP);
            }
        }

        public void GenerateIdentity(out byte[] keySecret, out byte[] keyPublic) {
            var length = GetEncryptionKeyLength();
            var bufferS = IntPtr.Zero;
            var bufferP = IntPtr.Zero;
            keySecret = new byte[length];
            keyPublic = new byte[length];

            try {
                bufferS = Marshal.AllocHGlobal(length);
                bufferP = Marshal.AllocHGlobal(length);

                NativeMethods.wirefox_peer_generate_crypto_identity(m_handle, bufferS, bufferP);

                Marshal.Copy(bufferS, keySecret, 0, length);
                Marshal.Copy(bufferP, keyPublic, 0, length);
            } finally {
                if (bufferS != IntPtr.Zero) Marshal.FreeHGlobal(bufferS);
                if (bufferP != IntPtr.Zero) Marshal.FreeHGlobal(bufferP);
            }
        }

        public static string ConnectResultToString(ConnectResult cr) {
            switch (cr) {
                case ConnectResult.OK:
                    return "No error occurred.";
                case ConnectResult.CONNECT_FAILED:
                    return "Failed to connect: Could not contact the remote host, or a communication error occurred.";
                case ConnectResult.INCOMPATIBLE_PROTOCOL:
                    return "Failed to connect: The remote host is not running Wirefox, or a communication error occurred.";
                case ConnectResult.INCOMPATIBLE_VERSION:
                    return "Failed to connect: The remote host is running an incompatible version of Wirefox.";
                case ConnectResult.INCOMPATIBLE_SECURITY:
                    return "Failed to establish secure connection: A configuration error was detected.";
                case ConnectResult.INCORRECT_REMOTE_IDENTITY:
                    return "Failed to establish secure connection: The identity of the remote host could not be verified.";
                case ConnectResult.INCORRECT_PASSWORD:
                    return "The password is incorrect.";
                case ConnectResult.NO_FREE_SLOTS:
                    return "The remote host is full. Please try again later.";
                case ConnectResult.ALREADY_CONNECTED:
                    return "You are already connected to this remote host.";
                case ConnectResult.IP_RATE_LIMITED:
                    return "You are sending too many requests in a short time. Please try again later.";
                case ConnectResult.BANNED:
                    return "You are banned from this remote host.";
                default:
                    return "Unknown error.";
            }
        }

    }

}
