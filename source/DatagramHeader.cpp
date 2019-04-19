/**
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#include "PCH.h"
#include "DatagramHeader.h"
#include "BinaryStream.h"

using namespace wirefox::detail;

void DatagramHeader::Serialize(BinaryStream& outstream) const {
    // control flags
    outstream.WriteBool(flag_data);
    outstream.WriteBool(flag_link);
    outstream.WriteBool(!acks.empty());
    outstream.WriteBool(!nacks.empty());

    //outstream.Align();
    outstream.WriteInt32(datagramID);

    // acknowledgements
    if (!acks.empty()) {
        const size_t numAcks = std::min(acks.size(), static_cast<size_t>(256));
        assert(numAcks > 0);

        outstream.WriteByte(static_cast<uint8_t>(numAcks - 1));
        for (auto id : acks)
            outstream.WriteInt32(id);
    }

    // non-acknowledgements
    if (!nacks.empty()) {
        const size_t numNacks = std::min(nacks.size(), static_cast<size_t>(256));
        assert(numNacks > 0);

        outstream.WriteByte(static_cast<uint8_t>(numNacks - 1));
        for (auto id : nacks)
            outstream.WriteInt32(id);
    }

    // payload
    if (flag_data) {
        assert(dataLength < std::numeric_limits<uint16_t>::max());
        outstream.WriteInt16(static_cast<uint16_t>(dataLength));
    }
}

bool DatagramHeader::Deserialize(BinaryStream& instream) {
    if (instream.IsEOF(sizeof(uint8_t) + sizeof(DatagramID))) //-V119 : ignored because alignment is irrelevant for stream data
        return false;

    // control flags
    flag_data = instream.ReadBool();
    flag_link = instream.ReadBool();
    const bool hasAcks = instream.ReadBool();
    const bool hasNacks = instream.ReadBool();

    // instream.Align();
    datagramID = instream.ReadUInt32();

    if (hasAcks) {
        const int numAcks = instream.ReadByte() + 1; // zero-indexed, so 0x00 means 1 ack
        acks.clear();
        acks.reserve(numAcks);
        for (int i = 0; i < numAcks; i++)
            acks.push_back(instream.ReadUInt32());
    }

    if (hasNacks) {
        const int numNacks = instream.ReadByte() + 1;
        nacks.clear();
        nacks.reserve(numNacks);
        for (int i = 0; i < numNacks; i++)
            nacks.push_back(instream.ReadUInt32());
    }

    if (flag_data) {
        if (instream.IsEOF(sizeof(uint32_t)))
            return false;

        dataLength = instream.ReadUInt16();

        // though the datagram header itself may be complete now, if the payload is missing, we should wait
        // for the rest of the data to arrive, before we report that this datagram is complete
        if (instream.IsEOF(dataLength))
            return false;
    }

    return true;
}
