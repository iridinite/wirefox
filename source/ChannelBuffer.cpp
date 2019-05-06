/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#include "PCH.h"
#include "ChannelBuffer.h"
#include "PeerAbstract.h"

using namespace detail;

namespace {

    constexpr bool SequenceLessThan(SequenceID lhs, SequenceID rhs) {
        constexpr auto halfSpan = static_cast<decltype(lhs)>(std::numeric_limits<decltype(lhs)>::max()) / 2;
        return lhs != rhs && rhs - lhs < halfSpan;
    }

    constexpr bool SequenceGreaterThanOrEqual(SequenceID lhs, SequenceID rhs) {
        constexpr auto halfSpan = static_cast<decltype(lhs)>(std::numeric_limits<decltype(lhs)>::max()) / 2;
        return lhs == rhs || rhs - lhs > halfSpan;
    }

}

ChannelBuffer::ChannelBuffer(const IPeer* peer, ChannelIndex index)
    : m_peer(peer)
    , m_nextEnqueue(0)
    , m_nextDequeue(0)
    , m_outgoing(0)
    , m_index(index) {
    // unordered channels should not have a ChannelBuffer constructed for them
    assert(GetMode() != ChannelMode::UNORDERED);
}

void ChannelBuffer::Enqueue(SequenceID sequence, Payload packet) {
    if (GetMode() == ChannelMode::SEQUENCED) {
        // discard sequenced packets that are out of date
        if (SequenceLessThan(sequence, m_nextEnqueue))
            return;

        m_nextEnqueue = sequence + 1;
    }

    m_backlog.emplace(sequence, std::move(packet));
}

ChannelBuffer::Payload ChannelBuffer::Dequeue() {
    // Because map<> stores the sequence in order, we only ever need to check the first entry.
    auto it = m_backlog.begin();
    if (it == m_backlog.end() || !IsEligible(it->first))
        return Payload(nullptr);

    // copy out the Payload object, erase the entry, and return it
    auto payload = std::move(it->second);
    m_nextDequeue = it->first + 1;
    m_backlog.erase(it);
    return payload;
}

bool ChannelBuffer::IsEligible(SequenceID sequence) const {
    if (GetMode() == ChannelMode::ORDERED) {
        // for ordered, cannot exceed the next expected entry in the sequence
        return SequenceGreaterThanOrEqual(m_nextDequeue, sequence);
    }

    // sequence is always eligible, because out-of-order packets were already discarded in Enqueue
    return true;
}

SequenceID ChannelBuffer::GetNextOutgoing() {
    return m_outgoing++;
}

ChannelMode ChannelBuffer::GetMode() const {
    return m_peer->GetChannelModeByIndex(m_index);
}
