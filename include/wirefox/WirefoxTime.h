/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#pragma once

namespace wirefox {

    /// Represents a duration of time.
    typedef uint64_t    Timespan;

    /// Represents a single, exact point in time.
    struct WIREFOX_API Timestamp {
        friend class Time;

        /// Default constructor.
        Timestamp() noexcept;
        /// Constructor with explicit value.
        Timestamp(uint64_t t) noexcept;

        /// Implicitly converts this Timestamp to its underlying value.
        explicit operator uint64_t() const noexcept;
        /// Checks whether this Timestamp is set to a value.
        bool IsValid() const noexcept;
        /// Adds a Timespan to the Timestamp, and returns the new Timestamp.
        Timestamp operator+(const Timespan& rhs) const noexcept;
        /// Subtracts a Timespan from the Timestamp, and returns the new Timestamp.
        Timestamp operator-(const Timespan& rhs) const noexcept;

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

        /// Returns the number of milliseconds represented by a Timespan.
        constexpr static int        ToMilliseconds(Timespan timespan) {
            return static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(precision(timespan)).count());
        }
    };

}
