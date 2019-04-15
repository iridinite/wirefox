Wire Format
===========

[TOC]

# Datagram Header

A datagram is one frame of data being sent across the network. One datagram may contain zero or more packets. In UDP, datagrams are delivered either complete and intact, or not at all, so no partial datagrams will arrive.

To help conserve bandwidth, both datagram and packet headers in Wirefox are variable-length, and generally omit irrelevant data.

@code{.cpp}

struct DatagramHeader {

	uint8	flags;
	uint32	datagramID;
	
	if (flags_ACK) {
		uint8	numAcks;
		uint32	acks[numAcks];
	}
	if (flags_NAK) {
		uint8	numNacks;
		uint32	nacks[numNacks];
	}
	
	if (flags_DAT) {
		uint16	payloadLength;
		uint8	payload[payloadLength];
	}

}

@endcode


# Packet Header

A packet represents one message or notification; either a user message (such as gameplay info) or a system message (such as a connection request).

@code{.cpp}

struct PacketHeader {

	uint8	flags;
	uint32	datagramID;
	uint8	channel;
	
	if (flags_BIG) {
		uint32	offset;
		uint32	length;
	} else {
		uint16	offset;
		uint16	length;
	}

}

@endcode

# Datagram Header Fields

| Field | Description |
|---|---|
| Control Flags (Bit 0) | DAT. Set if datagram includes a message. |
| Control Flags (Bit 1) | LNK. Set if sender thinks it's connected to us, unset if this is an offline datagram. |
| Control Flags (Bit 2) | ACK. Set if datagram includes one or more ACKs. |
| Control Flags (Bit 3) | NAK. Set if datagram includes one or more NACKs. |
| Control Flags (Bits 4-7) | Reserved. |
| # ACKs | uint8. Present if ACK set. Number of ACK IDs in header. Range [1, 255]. |
| Acknowledgement ID | uint32. Datagram number that was successfully received. |
| # NACKs | uint8. Present if NAK set. Number of NACK IDs in header. Range [1, 255]. |
| Non-Acknowledgement ID | uint32. Datagram number that was lost and must be re-sent. |

Note: It is considered a protocol violation to send a datagram that
- if LNK is set, has neither DAT or ACK or NAK set.
- if LNK is unset, has DAT also unset.

# Packet Header Fields

| Field | Description |
|---|---|
| Control Flags (Bit 0) | SEG. Set if this is a fragmented packet, and more will follow. Unset if this is the last packet in the fragment sequence. |
| Control Flags (Bit 1) | BIG. Set if this is (part of) a jumbogram (huge datagram). If set, Length and Offset become 32 bits wide. |
| Control Flags (Bit 2-7) | Reserved. |
| Packet ID | uint32. The packet number associated with the following payload. |
| Channel | uint8. Indicates ordering channel of attached message. 0 means unordered/no channel. |
| Length | uint16. Indicates the length, in bytes, of the payload. If this is not a fragmented packet, then this is the full length of the message. |
| Offset | uint16. Indicates the relative position, in bytes, in the final message where the payload belongs. Zero for unfragmented packets. |
