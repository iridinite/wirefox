Tutorial: Sending Messages
==========================

# Building a Packet

A [Packet](@ref wirefox::Packet) represents a single message; these are what you send back and forth between Peers. You can create one with a [PacketCommand](@ref wirefox::PacketCommand) and some data to send.

A PacketCommand is an ID number or 'tag' that tells the receiver what this Packet is meant to say or do. There are a number of built-in PacketCommands (referred to as system notifications) to inform you of (dis)connections and such, but obviously you'll want to define your own. More on that later.

You have two options:

## Using a raw buffer

You can pass a buffer along with the length to the Packet constructor. The data will be copied, so afterwards it is safe to change or deallocate the original buffer as you like.

Note that because Wirefox internally deals with bytes as uint8_t, you may need to `reinterpret_cast` your buffer.

Further, beware that because this method treats the payload as an opaque blob, no endianness conversions or the like are performed. This may or may not be a problem for your use case.

Example:

@code{.cpp}
char buffer[256]; // assuming this was filled with some data
Packet packet(PacketCommand::USER_PACKET, reinterpret_cast<uint8_t*>(buffer), sizeof(buffer));
@endcode

## Using a BinaryStream

The [BinaryStream](@ref wirefox::BinaryStream) class is meant to be a fast, lightweight tool for serializing user data. It's essentially an auto-sized array, but also automatically performs endianness conversion, and provides tools for packing some types of data more tightly.

Refer to the docs of BinaryStream for a proper list of read/write functions, but the general gist is that you call a WriteX(), which on the other end is met with an equal ReadX().

@code{.cpp}
// Create a stream with some data you want to send over the network.
BinaryStream myStream;
myStream.WriteInt32(1337);
myStream.WriteString("Hello, Wirefox!");

// Make a Packet containing a copy of this user data.
Packet packetCopy(PacketCommand::USER_PACKET, myStream);

// Make a Packet that takes ownership of the BinaryStream's buffer. More efficient!
Packet packetMove(PacketCommand::USER_PACKET, std::move(myStream));

// Let's pretend we just got this from the network... read the data!
BinaryStream instream = packetMove->GetStream();
auto num = instream.ReadInt32(); // 1337
auto str = instream.ReadString(); // Hello, Wirefox!
@endcode

# And off we go

Pass your deliciously newfangled Packet to [IPeer::Send()](@ref wirefox::IPeer::Send()). Other than the Packet itself, you'll need to:

* specify who you want to send it to (don't forget to save the PeerID when you get a `NOTIFY_CONNECT_SUCCESS` or `NOTIFY_CONNECTION_INCOMING`!)
* specify whether you want the packet to be sent reliably. Unreliable packets have less overhead, but may be lost in transit. Reliable packets will be automatically re-sent in that event. You can bitwise-OR together any PacketOptions you like.

Like so:

@code{.cpp}
// Use Packet::GetSender() on incoming packets to find the PeerIDs of others.
PeerID recipient = ...;

myPeer->Send(packet, recipient, PacketOptions::RELIABLE);
@endcode

## Custom PacketCommands

For your game / application, you likely want your own PacketCommands to easily distinguish different kinds of messages. For example, in a first person shooter, you might have different packets for player movement, events like shots, or chat messages.

As an example:

@code{.cpp}
enum class GameCommand : uint8_t {
	PLAYER_MOVE = wirefox::USER_PACKET_START,
	PLAYER_SHOOT,
	UPDATE_HEALTH,
	UPDATE_AMMO,
	CHAT
}
@endcode

Note the usage of [USER_PACKET_START](@ref wirefox::USER_PACKET_START), which is the first ID number that you can use for your own purposes. All IDs smaller than this constant are reserved for internal use.

This example enum uses uint8_t as underlying type, because the PacketCommand currently uses one byte, and can therefore only support IDs up to 255. This may change later. For now, if you wish to go higher than 255, you'll need to implement your own extension mechanism (i.e. a command that is followed in the payload by another, longer command).

Note also that PacketCommand is a strongly-typed enum, meaning that you'll need to cast back and forth between your custom enums and the built-in ones. This may also change later if I can find a nice way to implicitly perform the casts; for now you'll need to `static_cast` between the enums.