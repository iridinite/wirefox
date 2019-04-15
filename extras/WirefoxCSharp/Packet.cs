using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using JetBrains.Annotations;

namespace Iridinite.Wirefox {

    [PublicAPI]
    public class Packet : IDisposable {

        private IntPtr m_handle;

        private readonly PacketCommand m_cmd;
        private readonly byte[] m_payload;
        private readonly PeerID m_sender;

        private MemoryStream m_streamMs;

        public Packet(PacketCommand cmd, [NotNull] byte[] payload) {
            // to pass the buffer to unmanaged code, we need to memcpy it to an unmanaged buffer
            var length = payload?.Length ?? 0;
            var buffer = Marshal.AllocHGlobal(length);
            Marshal.Copy(payload, 0, buffer, length);

            m_handle = NativeMethods.wirefox_packet_create((byte) cmd, buffer, new UIntPtr((uint)length));
            m_cmd = cmd;
            m_sender = PeerID.Invalid;
            m_payload = payload;

            // PacketQueue will have copied the packet contents into a blob with a header,
            // so our temporary buffer can be freed
            Marshal.FreeHGlobal(buffer);
        }

        internal Packet(IntPtr handle) {
            m_handle = handle;
            m_cmd = (PacketCommand)NativeMethods.wirefox_packet_get_cmd(handle);
            m_sender = new PeerID(NativeMethods.wirefox_packet_get_sender(handle));
            m_payload = new byte[NativeMethods.wirefox_packet_get_length(handle).ToUInt32()];
            Marshal.Copy(NativeMethods.wirefox_packet_get_data(handle), m_payload, 0, m_payload.Length);
        }

        ~Packet() {
            Dispose(false);
        }

        private void Dispose(bool disposingManaged) {
            NativeMethods.wirefox_packet_destroy(m_handle);
            m_handle = IntPtr.Zero;

            if (disposingManaged) {
                m_streamMs?.Dispose();
                m_streamMs = null;
            }
        }

        public void Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        internal IntPtr GetHandle() {
            return m_handle;
        }

        public PacketCommand GetCommand() {
            return m_cmd;
        }

        public PeerID GetSender() {
            return m_sender;
        }

        public byte[] GetBuffer() {
            return m_payload;
        }

        public BinaryReader GetStream() {
            if (m_streamMs == null)
                m_streamMs = new MemoryStream(m_payload, false);
            m_streamMs.Seek(0, SeekOrigin.Begin);

            return new BinaryReader(m_streamMs, Encoding.UTF8, true);
        }

    }

}
