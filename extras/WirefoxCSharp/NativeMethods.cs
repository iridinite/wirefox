using System;
using System.Runtime.InteropServices;

namespace Iridinite.Wirefox {

    internal static class NativeMethods {

        private const string LIBRARY_NAME = "Wirefox";
        private const CallingConvention LIBRARY_CALL = CallingConvention.Cdecl;

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern IntPtr wirefox_peer_create(uint maxPeers);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern void wirefox_peer_destroy(IntPtr handle);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool wirefox_peer_bind(IntPtr handle, SocketProtocol protocol, [MarshalAs(UnmanagedType.U2)] ushort port);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern ConnectAttemptResult wirefox_peer_connect(IntPtr handle, [MarshalAs(UnmanagedType.LPStr)] string host, [MarshalAs(UnmanagedType.U2)] ushort port);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern void wirefox_peer_disconnect(IntPtr handle, ulong id);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool wirefox_peer_send(IntPtr handle, IntPtr packet, ulong recipient, PacketOptions opt);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern void wirefox_peer_send_loopback(IntPtr handle, IntPtr packet);

        [DllImport(LIBRARY_NAME, CallingConvention = LIBRARY_CALL)]
        public static extern IntPtr wirefox_peer_receive(IntPtr handle);

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
        public static extern ulong wirefox_packet_get_sender(IntPtr handle);

    }

}
