# Wirefox - Networking API

[![Build Status](https://travis-ci.org/iridinite/wirefox.svg?branch=master)](https://travis-ci.org/iridinite/wirefox) [![Build status](https://ci.appveyor.com/api/projects/status/lc448a3ax9ghqbec/branch/master?svg=true)](https://ci.appveyor.com/project/iridinite/wirefox/branch/master)

Wirefox is a flexible, easy-to-use and no-nonsense networking library.

- Connection-oriented API
- Message-oriented delivery
- Works on Windows, Unix-like systems, and Nintendo Switch
- Peer-to-peer-style architecture, but also supports client-server model
- Supports both IPv4 & IPv6 out of the box
- Supports reliable delivery of messages
- Supports encryption, with automatic key exchange and forward secrecy
- Supports custom channels, to sort and sequence packets
- Supports LAN discovery, to detect game lobbies etc. on the local network

## Here's a quick example!

```cpp
// Set up a server that accepts 1 client
auto server = wirefox::IPeer::Factory::Create();
server->SetMaximumIncomingPeers(1);
server->Bind(wirefox::SocketProtocol::IPv6, port);

// Set up a client
auto client = wirefox::IPeer::Factory::Create();
client->Bind(wirefox::SocketProtocol::IPv6, 0);

// Let's go!
client->Connect("amazing.server.com", port);
```

### Planned features:

- Custom packet prioritization
- Remote procedure calls
- Path MTU discovery
- PlayStation 4 support
- And more?

## Building from source

### Requirements

- Operating system support for BSD sockets interface
  (Windows, Linux, most newer game consoles)
- A C++14-compliant compiler, or newer
- CMake 3.9 or higher

Additionally, if encryption features are desired:

- [libsodium](https://github.com/jedisct1/libsodium/releases) 1.0.12 or higher

### Windows

Use CMake or CMake-GUI, then build with your favorite IDE (such as Visual Studio).

### Linux

Run CMake, then build using your favorite tool.
```
$ mkdir build
$ cd build
$ cmake ../
$ make
```

### Console support

In CMake, set the `WIREFOX_PLATFORM` setting to the desired platform. These values are recognized:

- `DESKTOP` refers to Windows or Linux,
- `NX` refers to Nintendo Switch.

Due to NDA restrictions, some files required to build for Nintendo Switch are not included in this public repo. To obtain them, please get in touch with me at mikamolenkamp@hotmail.com.

## Documentation

[Documentation is hosted here.](https://www.syllendel.nl/wirefox) Included is an API reference and a set of getting-started guides to walk you through it.

Alternatively, offline docs can be generated using [Doxygen](http://www.doxygen.nl/). Simply run `doxygen` on the command-line (in the project root folder) to generate documentation in the docs/html folder.

Note that you can enable or disable the internal documentation (e.g. everything that is not part of the public interface) by removing or adding "WIREFOX_INTERNAL" to the `ENABLED_SECTIONS` variable in Doxyfile. It's enabled by default.
