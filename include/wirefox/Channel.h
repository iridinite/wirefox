/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#pragma once
#include "WirefoxConfig.h"
#include "Enumerations.h"

namespace wirefox {
    
    /**
     * \brief Represents a channel on which packets may be sorted.
     */
    struct WIREFOX_API Channel {
        /// The index number that uniquely identifies this channel.
        ChannelIndex    id;
        /// The operation mode of this channel.
        ChannelMode     mode;

        /// Construct the default channel.
        Channel();

        /// Construct a channel with specific settings.
        Channel(ChannelIndex id, ChannelMode mode);

        /// Returns a value indicating whether this channel represents the default (unordered) channel.
        bool            IsDefault() const;

        /// Returns a value indicating whether packets sent through this channel will have their reliability overridden.
        bool            RequiresReliability() const;
    };

}
