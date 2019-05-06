/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

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
