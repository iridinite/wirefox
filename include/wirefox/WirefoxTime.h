#pragma once

namespace wirefox {

    /// Represents a duration of time.
    typedef uint64_t    Timespan;

    /// Represents a single, exact point in time.
    struct Timestamp {
        friend class Time;

        /// Default constructor.
        Timestamp() noexcept;
        /// Constructor with explicit value.
        Timestamp(uint64_t t) noexcept;

        /// Implicitly converts this Timestamp to its underlying value.
        operator uint64_t() const noexcept;
        /// Adds a Timespan to the Timestamp, and returns the new Timestamp.
        Timestamp operator+(const Timespan& rhs) noexcept;
        /// Subtracts a Timespan from the Timestamp, and returns the new Timestamp.
        Timestamp operator-(const Timespan& rhs) noexcept;

        /// Equality operator.
        bool operator==(const Timestamp& rhs) const noexcept { return t == rhs.t; }
        /// Inequality operator.
        bool operator!=(const Timestamp& rhs) const noexcept { return t != rhs.t; }
        /// Greater-than operator.
        bool operator>(const Timestamp& rhs) const noexcept { return t > rhs.t; }
        /// Greater-than-or-equal operator.
        bool operator>=(const Timestamp& rhs) const noexcept { return t >= rhs.t; }
        /// Less-than operator.
        bool operator<(const Timestamp& rhs) const noexcept { return t < rhs.t; }
        /// Less-than-or-equal operator.
        bool operator<=(const Timestamp& rhs) const noexcept { return t <= rhs.t; }

    private:
        uint64_t t;
    };

    /// Provides utilities for dealing with, and generating, timestamps.
    class WIREFOX_API Time final {
        using precision = std::chrono::nanoseconds;

    public:
        /// Returns a Timestamp that represents the system time at the moment the function was called.
        static Timestamp        Now();

        /// Returns a boolean indicating whether the specified Timestamp has already elapsed.
        static bool             Elapsed(const Timestamp& test);

        /// Returns a Timespan representing the time between two Timestamps.
        static Timespan         Between(const Timestamp& lhs, const Timestamp& rhs);

        /// Generates a Timespan from a number of milliseconds.
        constexpr static Timespan   FromMilliseconds(int milliseconds) {
            return std::chrono::duration_cast<precision>(std::chrono::milliseconds(milliseconds)).count();
        }

        /// Generates a Timespan from a number of seconds.
        constexpr static Timespan   FromSeconds(int seconds) {
            return std::chrono::duration_cast<precision>(std::chrono::seconds(seconds)).count();
        }
    };

}
