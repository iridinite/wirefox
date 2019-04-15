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
