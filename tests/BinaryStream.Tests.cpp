#include <catch2/catch.hpp>
#include <Wirefox.h>

TEST_CASE("BinaryStream move ctor", "[BinaryStream]") {
    wirefox::BinaryStream a;
    a.WriteInt32(1234);
    a.SeekToBegin();

    wirefox::BinaryStream b(std::move(a));
    REQUIRE(a.GetBuffer() == nullptr);
    REQUIRE(b.ReadInt32() == 1234);
}

TEST_CASE("BinaryStream read-only wrapper", "[BinaryStream]") {
    const uint8_t foo[] = {'E', 'x', 'a', 'm', 'p', 'l', 'e'};
    wirefox::BinaryStream s(foo, sizeof foo, wirefox::BinaryStream::WrapMode::READONLY);
    REQUIRE(s.GetBuffer() == foo);
    REQUIRE(s.IsReadOnly());
    REQUIRE(s.ReadByte() == 'E');
}

TEST_CASE("BinaryStream concatenation", "[BinaryStream]") {
    wirefox::BinaryStream a, b, c;
    a.WriteInt32(1);
    b.WriteInt32(2);
    c.WriteInt32(3);

    a.WriteBytes(b);
    a.WriteBytes(c);

    a.SeekToBegin();
    REQUIRE(a.ReadInt32() == 1);
    REQUIRE(a.ReadInt32() == 2);
    REQUIRE(a.ReadInt32() == 3);
}

TEST_CASE("BinaryStream resizes dynamically", "[BinaryStream]") {
    wirefox::BinaryStream s(4);
    REQUIRE(s.GetCapacity() >= 4);
    REQUIRE(s.GetLength() == 0);

    char foo[16]; // arbitrary data, contents not important
    s.WriteBytes(foo, 16);

    REQUIRE(s.GetCapacity() >= 16);
    REQUIRE(s.GetLength() == 16);

    s.Ensure(20);
    REQUIRE(s.GetCapacity() >= 36);
}

TEST_CASE("BinaryStream self-consistency", "[BinaryStream]") {
    wirefox::BinaryStream s;
    s.WriteBool(true);
    s.WriteByte(1);
    s.WriteInt16(2);
    s.WriteInt32(3);
    s.WriteInt64(4);
    s.WriteString("asdf");

    s.SeekToBegin();

    REQUIRE(s.ReadBool() == true);
    REQUIRE(s.ReadByte() == 1);
    REQUIRE(s.ReadInt16() == 2);
    REQUIRE(s.ReadInt32() == 3);
    REQUIRE(s.ReadInt64() == 4);
    REQUIRE(s.ReadString() == "asdf");
}

TEST_CASE("BinaryStream endianness conversion", "[BinaryStream]") {
    // we write some fixed value to the stream, and examine the buffer directly to verify
    // that it was written as a big-endian (network order) value

    constexpr unsigned someValue = 12345; // 0x3039 on little endian
    wirefox::BinaryStream s;

    SECTION("16-bit") {
        s.WriteInt16(someValue);

        auto* buffer = reinterpret_cast<const uint16_t*>(s.GetBuffer());
        REQUIRE(*buffer == 0x3930);
    }
    SECTION("32-bit") {
        s.WriteInt32(someValue);

        auto* buffer = reinterpret_cast<const uint32_t*>(s.GetBuffer());
        REQUIRE(*buffer == 0x39300000);
    }
    SECTION("64-bit") {
        s.WriteInt64(someValue);

        auto* buffer = reinterpret_cast<const uint64_t*>(s.GetBuffer());
        REQUIRE(*buffer == 0x3930000000000000);
    }
}
