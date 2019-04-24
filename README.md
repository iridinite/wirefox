# Wirefox - Networking API

[![Build Status](https://travis-ci.org/iridinite/wirefox.svg?branch=master)](https://travis-ci.org/iridinite/wirefox) [![Build status](https://ci.appveyor.com/api/projects/status/lc448a3ax9ghqbec/branch/master?svg=true)](https://ci.appveyor.com/project/iridinite/wirefox/branch/master)

Wirefox is a flexible, easy-to-use and no-nonsense networking library.

- Connection-oriented API
- Message-oriented delivery
- Peer-to-peer-style architecture, but also supports client-server model
- Supports IPv4 and IPv6
- Supports reliable delivery
- Supports custom channels, to sort and sequence packets

This library is under active development, and probably not ready to be used in production. Regardless, I'd be happy to hear from you if you have any thoughts or concerns.

Planned features:

- Custom packet prioritization
- LAN discovery and pinging
- Remote procedure calls
- Encryption

## Building from source

### Requirements

- Operating system support for BSD sockets interface
  (Windows, Linux, most newer game consoles)
- A C++14-compliant compiler, or newer
- CMake 3.9 or higher

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

## Documentation

[Documentation is hosted here.](https://www.syllendel.nl/wirefox) Included is an API reference and a set of getting-started guides to walk you through it.

Alternatively, offline docs can be generated using [Doxygen](http://www.doxygen.nl/). Simply run `doxygen` on the command-line (in the project root folder) to generate documentation in the docs/html folder.

Note that you can enable or disable the internal documentation (e.g. everything that is not part of the public interface) by removing or adding "WIREFOX_INTERNAL" to the ENABLED_SECTIONS variable in Doxyfile. It's enabled by default.
