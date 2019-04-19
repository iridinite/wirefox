/**
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#include "PCH.h"
#include "RemoteAddress.h"

using namespace wirefox::detail;

bool RemoteAddress::operator==(const RemoteAddress& rhs) const {
    return endpoint_udp == rhs.endpoint_udp;
}

bool RemoteAddress::operator!=(const RemoteAddress& rhs) const {
    return !(*this == rhs);
}

std::string RemoteAddress::ToString() const {
    std::stringstream txt;
    txt << endpoint_udp;
    return txt.str();
}
