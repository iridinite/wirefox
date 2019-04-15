#include "PCH.h"
#include "Channel.h"

Channel::Channel()
    : id(0)
    , mode(ChannelMode::UNORDERED) {}

Channel::Channel(ChannelIndex id, ChannelMode mode)
    : id(id)
    , mode(mode) {}

bool Channel::IsDefault() const {
    return id == 0;
}

bool Channel::RequiresReliability() const {
    return mode == ChannelMode::ORDERED;
}
