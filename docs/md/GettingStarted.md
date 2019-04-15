Getting Started
===============

[TOC]

# Compiling from source

**Requirements:**

* C++14 compliant compiler
* CMake 3.9 or higher

**Windows**

Run CMake or CMake-GUI, optionally tweaking settings to your liking, to generate project files.

**Linux**

Run CMake, then build using your favorite tool.

```
$ cmake ./build
$ make
```

# Setting up

After you've compiled and set up the Wirefox library, you can include the all-in-one header to ensure everything is available and all dependencies are also included.

You *can* include loose headers if you really want to, but you have to ensure all required STL dependencies and wirefox/WirefoxConfig.h are included before them.

@code{.cpp}
#include <Wirefox.h>
@endcode

## Creating a Peer

A [Peer](@ref wirefox::IPeer) represents a single node in the network. Usually, you'd need one Peer for one instance of the game. Wirefox is built as a peer-to-peer networking library, but you can also use it for the more classic client/server model if you like.

Create a Peer using the IPeer::Factory utility:

@code{.cpp}
auto myPeer = IPeer::Factory::Create(maxPeers);
@endcode

The @p maxPeers parameter lets you specify how many connections this Peer should be able to handle at the same time. For pure clients, this could be 1 (in which case you can also omit the argument, it'll default to 1); for servers it'll be the number of clients that can be simultaneously connected. Be sure to pick a sane number; Wirefox reserves memory for the slots.

## Binding

Before your [Peer](@ref wirefox::IPeer) can do anything interesting, you need to bind it to a port.

@code{.cpp}
myPeer->Bind(SocketProtocol::IPv4, port);
@endcode

You can specify whether you'd like the socket to work on IPv4 or IPv6. Note that you can only bind to one of those, and you can only communicate with Peers who are using the same socket protocol.

The port will be the network port your socket will use for communications. If you expect incoming connections, this should be set to some known value (dependent on your use case / game), so that other Peers can easily connect to it. For pure clients, it may be useful to set the port to zero; this will cause the OS to assign a random port for you.

Bind() returns a boolean indicating success. If it returns false (indicating failure), make sure that the specified port is not already in use, and that you have sufficient OS privileges to bind a socket.

# Communication

## Connecting

To have a Peer connect to a remote endpoint, call [Connect()](@ref wirefox::IPeer::Connect()). This will return a [ConnectAttemptResult](@ref wirefox::ConnectAttemptResult), which indicates whether the connection request was successfully sent out or not.

If all is well, you will receive a notification later telling you whether the connection was established or not. You'll get an incoming Packet, with a command of either @p NOTIFY_CONNECT_SUCCESS or @p NOTIFY_CONNECT_FAILED. If the attempt failed, you can optionally read one byte from the payload as a [ConnectResult](@ref wirefox::ConnectResult) to get a more specific failure reason.

Example:

@code{.cpp}
ConnectAttemptResult attempt;
attempt = myPeer->Connect("1.2.3.4", 8080);

if (attempt != ConnectAttemptResult::OK) {
	// something went wrong, most likely the hostname wasn't understood
}
@endcode

## Accepting connections

To allow incoming connections, a Peer must explicitly specify how many slots can be used up by incoming connections. If more remote endpoints try to connect than this setting, they will be rejected with a [NO_FREE_SLOTS](@ref wirefox::ConnectResult::NO_FREE_SLOTS) error. The default value is zero.

You probably want to set this to the same number as the @p maxPeers you passed when creating your Peer, but that is not required. Particularly, if you expect to have both incoming and outgoing connections, that may be useful.

@code{.cpp}
// A maximum of 3 other people can connect with me.
myPeer->SetMaximumIncomingPeers(3);
@endcode

If a remote endpoint successfully connects, you will receive a @p NOTIFY_CONNECTION_INCOMING notification.

## Cool, now what?

Further reading:

* [Receiving messages](docs/md/TutorialReceive.md)
* [Sending messages](docs/md/TutorialSend.md)
* [IPeer API reference](@ref wirefox::IPeer)
* [Packet API reference](@ref wirefox::Packet)
* [BinaryStream API reference](@ref wirefox::BinaryStream)
