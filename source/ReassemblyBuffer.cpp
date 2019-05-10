/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#include "PCH.h"
#include "ReassemblyBuffer.h"
#include "PacketHeader.h"
#include "RemotePeer.h"

using namespace detail;

ReassemblyBuffer::ReassemblyBuffer(RemotePeer* master)
    : m_remote(master) {}

ReassemblyBuffer::ReassemblyBuffer(ReassemblyBuffer&& other) noexcept {
    *this = std::move(other);
}

ReassemblyBuffer& ReassemblyBuffer::operator=(ReassemblyBuffer&& other) noexcept {
    if (&other != this) {
        m_remote = other.m_remote;
        m_backlog = std::move(other.m_backlog);

        other.m_remote = nullptr;
    }

    return *this;
}

void ReassemblyBuffer::Insert(const PacketHeader& header, BinaryStream& instream) {
    assert(header.offset + header.length < cfg::PACKET_MAX_LENGTH);

    // find or create the reassembly buffer associated with this packet ID
    if (!m_backlog.count(header.splitContainer))
        m_backlog.emplace(header.splitContainer, SplitPacket());
    auto itr = m_backlog.find(header.splitContainer);
    assert(itr != m_backlog.end());

    // seek to the header-specified offset, and ensure there is enough space from there to fit the segment
    auto& split = itr->second;
    split.blob.SeekForce(header.offset);
    assert(split.blob.GetPosition() == header.offset);

    // read the input stream directly into the storage buffer (so we don't need to allocate any temporaries)
    split.received.emplace(header.splitIndex);
    split.blob.WriteZeroes(header.length);
    instream.ReadBytes(split.blob.GetWritableBuffer() + header.offset, header.length);

    if (!header.flag_segment) {
        split.last = header.splitIndex;
        assert(split.last > 0); // if there is only one segment, then why is this even a split packet?
    }
}

std::unique_ptr<Packet> ReassemblyBuffer::Reassemble(PacketID container) {
    auto itr = m_backlog.find(container);
    assert(itr != m_backlog.end());
    if (itr == m_backlog.end()) return nullptr;

    // received all segments?
    auto& split = itr->second;
    if (split.last == 0 || split.received.size() != split.last + 1) return nullptr;

    // ensure we start deserializing the packet where it actually starts
    //std::cout << "Successfully reassembled group " << container << ", with " << split.received.size() << " segments" << std::endl;
    split.blob.SeekToBegin();

    // deserialize the packet, erase the partial buffer, and return the final reassembled packet
    auto packet = Packet::Factory::Create(Packet::FromDatagram(m_remote->id, split.blob, split.blob.GetLength()));
    m_backlog.erase(itr);
    return packet;
}
