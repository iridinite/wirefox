/**
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

using JetBrains.Annotations;

namespace Iridinite.Wirefox {

    [PublicAPI]
    public struct Channel {

        public byte Index { get; set; }
        public ChannelMode Mode { get; set; }

    }

}
