/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#include "PCH.h"
#include "Packet.h"
#include "BinaryStream.h"

namespace {

    std::unique_ptr<uint8_t[]> CopyFrom(const uint8_t* raw, size_t count) {
        // packet can have no payload; nullptr would be more elegant than a zero size array
        if (!raw || count == 0) return nullptr;

        // allocate a new buffer the size of the source buffer
        const auto bufferlen = count * sizeof(decltype(*raw));
        std::unique_ptr<uint8_t[]> dataCopy(new uint8_t[count]);

        // copy the data using a memcpy
        memcpy(dataCopy.get(), raw, bufferlen);
        return dataCopy;
    }

}

Packet::Packet(const Packet& other)
    : m_sender(0)
    , m_command(PacketCommand(0))
    , m_length(0)
    , m_data(nullptr) {
    *this = other;
}

Packet::Packet(Packet&& rhs) noexcept
    : m_sender(0)
    , m_command(PacketCommand(0))
    , m_length(0)
    , m_data(nullptr) {
    *this = std::move(rhs);
}

Packet::Packet(PacketCommand cmd, const uint8_t* data, size_t datalen)
    : m_sender(0) {
    m_command = cmd;
    m_data = CopyFrom(data, datalen);
    m_length = datalen;
}

Packet::Packet(PacketCommand cmd, const BinaryStream& data)
    : m_sender(0)
    , m_command(cmd)
    , m_length(data.GetLength())
    , m_data(data.ToArray()) {}

Packet::Packet(PacketCommand cmd, BinaryStream&& data)
    : m_sender(0)
    , m_command(cmd)
    , m_length(0)
    , m_data(data.ReleaseBuffer(&m_length)) {}

Packet& Packet::operator=(const Packet& rhs) {
    m_data = CopyFrom(rhs.m_data.get(), rhs.m_length);
    m_command = rhs.m_command;
    m_length = rhs.m_length;
    m_sender = rhs.m_sender;
    return *this;
}

Packet& Packet::operator=(Packet&& rhs) noexcept {
    if (this != &rhs) {
        m_data = std::move(rhs.m_data);
        m_command = rhs.m_command;
        m_length = rhs.m_length;
        m_sender = rhs.m_sender;

        rhs.m_command = PacketCommand(0);
        rhs.m_length = 0;
        rhs.m_sender = 0;
    }
    return *this;
}

BinaryStream Packet::GetStream() const {
    return BinaryStream(m_data.get(), m_length, BinaryStream::WrapMode::READONLY);
}

void Packet::ToDatagram(BinaryStream& outstream) const {
    // This really only just prepends the cmd id and then copies the whole data buffer. The primary
    // reason this is here is so that there is a nice analogue to FromDatagram, in the same source file.
    // Therefore, the Packet's internal format is abstracted away.

    assert(!outstream.IsReadOnly());
    outstream.WriteByte(static_cast<uint8_t>(m_command));

    // Only call WriteBytes if we actually have a payload; m_data can be nullptr
    if (m_length > 0 && m_data != nullptr)
        outstream.WriteBytes(m_data.get(), m_length);
}

Packet Packet::FromDatagram(PeerID sender, BinaryStream& instream, size_t len) {
    return Packet(sender, instream, len);
}

Packet::Packet(PeerID sender, BinaryStream& instream, size_t len)
    : m_sender(sender)
    , m_length(len - 1) {
    assert(len >= 1);
    m_command = static_cast<PacketCommand>(instream.ReadByte());
    m_data = CopyFrom(instream.GetBuffer() + instream.GetPosition(), len - 1);

    assert(GetDatagramLength() == len);
    instream.Skip(len - 1);
}
