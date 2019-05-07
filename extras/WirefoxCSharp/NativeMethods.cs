/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

using System;
using System.Runtime.InteropServices;
using TPeerID = System.UInt64;
using TPacketID = System.UInt32;
using TChannelIndex = System.Byte;

namespace Iridinite.Wirefox {

    internal static class NativeMethods {

        private const string LIBRARY_NAME = "Wirefox";
        private const CallingConvention LIBRARY_CALL = CallingConvention.Cdecl;

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern IntPtr wirefox_peer_create([MarshalAs(UnmanagedType.SysUInt)] UIntPtr maxPeers);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern void wirefox_peer_destroy(IntPtr handle);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool wirefox_peer_bind(IntPtr handle, SocketProtocol protocol, [MarshalAs(UnmanagedType.U2)] ushort port);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern void wirefox_peer_stop(IntPtr handle, uint linger);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern ConnectAttemptResult wirefox_peer_connect(IntPtr handle, [MarshalAs(UnmanagedType.LPStr)] string host, [MarshalAs(UnmanagedType.U2)] ushort port);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern void wirefox_peer_disconnect(IntPtr handle, TPeerID id, uint linger);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern void wirefox_peer_disconnect_immediate(IntPtr handle, TPeerID id);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern TPacketID wirefox_peer_send(IntPtr handle, IntPtr packet, TPeerID recipient, PacketOptions options, PacketPriority priority, TChannelIndex channelIndex);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern void wirefox_peer_send_loopback(IntPtr handle, IntPtr packet);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern IntPtr wirefox_peer_receive(IntPtr handle);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern TChannelIndex wirefox_peer_make_channel(IntPtr handle, ChannelMode mode);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern ChannelMode wirefox_peer_get_channel_mode(IntPtr handle, TChannelIndex index);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern TPeerID wirefox_peer_get_my_id(IntPtr handle);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        [return: MarshalAs(UnmanagedType.SysUInt)]
        public static extern UIntPtr wirefox_peer_get_max_peers(IntPtr handle);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        [return: MarshalAs(UnmanagedType.SysUInt)]
        public static extern UIntPtr wirefox_peer_get_max_incoming_peers(IntPtr handle);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern void wirefox_peer_set_max_incoming_peers(IntPtr handle, [MarshalAs(UnmanagedType.SysUInt)] UIntPtr incoming);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern uint wirefox_peer_get_ping(IntPtr handle, TPeerID who);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool wirefox_peer_get_ping_available(IntPtr handle, TPeerID who);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern void wirefox_peer_set_network_sim(IntPtr handle, float packetLoss, uint additionalPing);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern void wirefox_peer_set_offline_ad(IntPtr handle, IntPtr data, [MarshalAs(UnmanagedType.SysUInt)] UIntPtr len);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern void wirefox_peer_disable_offline_ad(IntPtr handle);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern void wirefox_peer_ping(IntPtr handle, [MarshalAs(UnmanagedType.LPStr)] string host, ushort port);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern void wirefox_peer_ping_local_network(IntPtr handle, ushort port);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        [return: MarshalAs(UnmanagedType.SysUInt)]
        public static extern UIntPtr wirefox_peer_get_crypto_key_length(IntPtr handle);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool wirefox_peer_get_crypto_enabled(IntPtr handle);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern void wirefox_peer_set_crypto_enabled(IntPtr handle, [MarshalAs(UnmanagedType.I1)] bool enabled);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern void wirefox_peer_set_crypto_identity(IntPtr handle, IntPtr key_secret, IntPtr key_public);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern void wirefox_peer_generate_crypto_identity(IntPtr handle, IntPtr key_secret, IntPtr key_public);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern IntPtr wirefox_packet_create(byte command, IntPtr data, [MarshalAs(UnmanagedType.SysUInt)] UIntPtr len);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern void wirefox_packet_destroy(IntPtr handle);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern IntPtr wirefox_packet_get_data(IntPtr handle);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        [return: MarshalAs(UnmanagedType.SysUInt)]
        public static extern UIntPtr wirefox_packet_get_length(IntPtr handle);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern byte wirefox_packet_get_cmd(IntPtr handle);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern TPeerID wirefox_packet_get_sender(IntPtr handle);

    }

}
