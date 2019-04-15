using JetBrains.Annotations;

namespace Iridinite.Wirefox {

    [PublicAPI]
    public struct PeerID {

        public static PeerID Invalid { get; } = new PeerID(0);

        private readonly ulong m_val;

        internal PeerID(ulong val) {
            m_val = val;
        }

        public static implicit operator ulong(PeerID self) {
            return self.m_val;
        }

    }

}
