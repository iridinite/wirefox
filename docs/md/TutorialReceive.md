Tutorial: Receiving Messages
============================

# Receiving from the network

[IPeer::Receive()](@ref wirefox::IPeer::Receive()) will return the next [Packet](@ref wirefox::Packet) in the queue, if any. This Packet may be a user packet (something your game sent), or a system notification (to inform you of new connections, etc).

As implied by the return type of std::unique_ptr<Packet>, you gain full ownership of the Packet and are responsible for its deletion. Of course, simply letting the unique_ptr go out of scope works perfectly fine.

You should call Receive() regularly - I recommend at least once per frame in a game, or on a timer of around 5-10 ms. Note that in order to fully empty the queue, you should keep repeatedly calling Receive() until it returns a null pointer. Only calling it once per frame may cause a very long queue to form.

Example:

@code{.cpp}
auto recv = myPeer->Receive();
while (recv) {
	// We received a packet, do something with it.
	// ...
	
	// Get the next available Packet
	recv = myPeer->Receive();
}
@endcode

# Handling packets

Okay, so you got yourself a nifty [Packet](@ref wirefox::Packet), now what?

There are three parts to a Packet that are of interest:

* The PacketCommand, which is an ID number that tells you what kind of packet it is.  You can get this value with [GetCommand()](@ref wirefox::Packet::GetCommand()).
* The payload, or the contents of the message. You can obtain either a raw pointer using [GetBuffer()](@ref wirefox::Packet::GetBuffer()) and [GetLength()](@ref wirefox::Packet::GetLength()), or you can obtain a read-only BinaryStream using [GetStream()](@ref wirefox::Packet::GetStream()).
* The sender, which is the PeerID of whoever sent you this message or is related to it. For example, a `NOTIFY_CONNECTION_INCOMING` has the newly connected remote PeerID as the sender. Use [GetSender()](@ref wirefox::Packet::GetSender()).

You could use a switch statement to enumerate all possible PacketCommands, and handle them from there. You could also use a more elegant approach like a dispatch table (mapping PacketCommands to handler functions) - it's up to you. Check out the demo projects in the examples folder for an idea of how you could work with the API.

In general, if you used a [BinaryStream](@ref wirefox::BinaryStream) to construct a Packet, it is advised that you call `GetStream()` to obtain a stream you can read from. It is more efficient than manually constructing a new stream around the raw buffer, because GetStream() will not make a copy of the data. Of course, be sure to read the data in the same order that it was written.

@note All system notifications have their payloads written using a BinaryStream, so it is advised to use a BinaryStream to read them.

Refer to the PacketCommand enum in Enumerations.h for a description of all system notifications you may receive, and their payloads (if any).