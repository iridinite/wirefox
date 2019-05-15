/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#include "PCH.h"
#include "RemoteAddressASIO.h"

using namespace detail;

bool RemoteAddressASIO::operator==(const RemoteAddressASIO& rhs) const {
    return endpoint_udp == rhs.endpoint_udp;
}

bool RemoteAddressASIO::operator!=(const RemoteAddressASIO& rhs) const {
    return !(*this == rhs);
}

std::string RemoteAddressASIO::ToString() const {
    std::stringstream txt;
    txt << endpoint_udp.address().to_string();
    return txt.str();
}
