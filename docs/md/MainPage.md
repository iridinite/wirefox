@mainpage

# Wirefox - Networking API

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

## Documentation

[Documentation is hosted here.](https://www.syllendel.nl/wirefox) Included is an API reference and a set of getting-started guides to walk you through it.

Alternatively, offline docs can be generated using [Doxygen](http://www.doxygen.nl/). Simply run `doxygen` on the command-line (in the project root folder) to generate documentation in the docs/html folder.

Note that you can enable or disable the internal documentation (e.g. everything that is not part of the public interface) by removing or adding "WIREFOX_INTERNAL" to the ENABLED_SECTIONS variable in Doxyfile. It's enabled by default.
