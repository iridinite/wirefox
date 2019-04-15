#include "PCH.h"
#include "CongestionControl.h"

using namespace wirefox::detail;

std::random_device g_csprng;

bool CongestionControl::SequenceGreaterThan(DatagramID lhs, DatagramID rhs) {
    constexpr decltype(lhs) halfSpan = static_cast<decltype(lhs)>(std::numeric_limits<decltype(lhs)>::max()) / 2;
    return lhs != rhs && rhs - lhs > halfSpan;
}

bool CongestionControl::SequenceLessThan(DatagramID lhs, DatagramID rhs) {
    constexpr decltype(lhs) halfSpan = static_cast<decltype(lhs)>(std::numeric_limits<decltype(lhs)>::max()) / 2;
    return lhs != rhs && rhs - lhs < halfSpan;
}
