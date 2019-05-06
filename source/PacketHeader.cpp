/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#include "PCH.h"
#include "PacketHeader.h"
#include "BinaryStream.h"

using namespace wirefox::detail;

constexpr bool HasFlag(PacketOptions options, PacketOptions test) {
    using num_t = std::underlying_type<decltype(test)>::type;
    return (static_cast<num_t>(options) & static_cast<num_t>(test)) == static_cast<num_t>(test);
}

void PacketHeader::Serialize(BinaryStream& outstream) const {
    // control flags
    outstream.WriteBool(flag_segment);
    outstream.WriteBool(flag_jumbo);

    static_assert(std::is_same<uint8_t, std::underlying_type<PacketOptions>::type>::value, "PacketOptions must be uint8_t");

    outstream.WriteInt32(id);
    outstream.WriteByte(static_cast<uint8_t>(options));

    outstream.WriteByte(channel);
    if (channel)
        outstream.WriteInt32(sequence);

    if (flag_jumbo) {
        outstream.WriteInt32(length);
        outstream.WriteInt32(offset);
    } else {
        outstream.WriteInt16(length & 0xFFFF);
        outstream.WriteInt16(offset & 0xFFFF);
    }
}

bool PacketHeader::Deserialize(BinaryStream& instream) {
    if (instream.IsEOF(3 * sizeof(uint8_t) + sizeof(PacketID)))
        return false;

    flag_segment = instream.ReadBool();
    flag_jumbo = instream.ReadBool();

    // instream.Align();

    id = instream.ReadUInt32();
    options = static_cast<PacketOptions>(instream.ReadByte());
    channel = instream.ReadByte();
    if (channel)
        sequence = instream.ReadUInt32();

    if (flag_jumbo) {
        if (instream.IsEOF(2 * sizeof(uint32_t)))
            return false;

        length = instream.ReadUInt32();
        offset = instream.ReadUInt32();
    } else {
        if (instream.IsEOF(2 * sizeof(uint16_t)))
            return false;

        length = instream.ReadUInt16();
        offset = instream.ReadUInt16();
    }

    // sanity check
    if (length > cfg::PACKET_MAX_LENGTH || offset >= cfg::PACKET_MAX_LENGTH)
        return false;

    return true;
}
