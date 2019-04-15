#include "PCH.h"
#include "WirefoxTime.h"

Timestamp::Timestamp() noexcept : t(0) {}
Timestamp::Timestamp(uint64_t t) noexcept : t(t) {}

Timestamp::operator uint64_t() const noexcept {
    return t;
}

Timestamp Timestamp::operator+(const Timespan& rhs) noexcept {
    t += rhs;
    return *this;
}

Timestamp Timestamp::operator-(const Timespan& rhs) noexcept{
    // prevent underflow
    if (rhs > t) {
        t = 0;
    } else {
        t -= rhs;
    }

    return *this;
}

Timestamp Time::Now() {
    const auto now = std::chrono::steady_clock::now();
    const auto nano = std::chrono::time_point_cast<precision>(now);
    return nano.time_since_epoch().count();
}

bool Time::Elapsed(const Timestamp& test) {
    return Now() >= test;
}

Timespan Time::Between(const Timestamp& lhs, const Timestamp& rhs) {
    if (lhs > rhs)
        return lhs.t - rhs.t;

    return rhs.t - lhs.t;
}
