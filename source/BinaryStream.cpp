/**
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#include "PCH.h"
#include "BinaryStream.h"
#include "Serializable.h"

namespace {

    constexpr size_t ToNextPowerOfTwo(size_t n) {
        n--;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        n++;
        return n;
    }

}

BinaryStream::BinaryStream(const size_t capacity)
    : m_length(0)
    , m_position(0)
    , m_subBytePosition(0)
    , m_readonly(false) {
    m_capacity = ToNextPowerOfTwo(capacity);
    m_buffer = m_capacity > 0
        ? new uint8_t[m_capacity]
        : nullptr;
    m_buffer_ro = m_buffer;
}

BinaryStream::BinaryStream(const uint8_t* buffer, size_t bufferlen, WrapMode mode)
    : m_buffer(nullptr)
    , m_length(bufferlen)
    , m_position(0)
    , m_subBytePosition(0)
    , m_readonly(false) {
    switch (mode) {
    case WrapMode::COPY:
        m_capacity = ToNextPowerOfTwo(bufferlen);
        assert(m_capacity >= bufferlen);

        m_buffer = m_capacity > 0 ? new uint8_t[m_capacity] : nullptr;
        m_buffer_ro = m_buffer;
        if (m_buffer)
            memcpy(m_buffer, buffer, bufferlen);
        break;
    case WrapMode::READONLY:
        m_capacity = bufferlen;
        m_buffer_ro = buffer;
        m_readonly = true;
        break;
    default:
        break;
    }
}

BinaryStream::BinaryStream(const BinaryStream& other)
    : BinaryStream(other.GetBuffer(), other.GetLength()) {}

BinaryStream::BinaryStream(BinaryStream&& other) noexcept
    : m_buffer(nullptr)
    , m_buffer_ro(nullptr)
    , m_capacity(0)
    , m_length(0)
    , m_position(0)
    , m_subBytePosition(0)
    , m_readonly(false) {
    *this = std::move(other);
}

BinaryStream::~BinaryStream() {
    Reset();
}

BinaryStream& BinaryStream::operator=(const BinaryStream& other) {
    // resize our data buffer so it's the same capacity as the operand's, and copy it
    this->Reallocate(other.GetCapacity());
    assert(m_capacity == other.m_capacity);
    assert(m_capacity >= other.m_length);
    memcpy(m_buffer, other.GetBuffer(), other.GetLength());
    m_buffer_ro = m_buffer;

    // copy metadata
    m_length = other.m_length;
    m_position = other.m_position;
    m_subBytePosition = other.m_subBytePosition;
    m_readonly = other.m_readonly;

    return *this;
}

BinaryStream& BinaryStream::operator=(BinaryStream&& other) noexcept {
    if (this != &other) {
        this->m_buffer = other.m_buffer;
        this->m_buffer_ro = other.m_buffer_ro;
        this->m_capacity = other.m_capacity;
        this->m_length = other.m_length;
        this->m_position = other.m_position;
        this->m_subBytePosition = other.m_subBytePosition;
        this->m_readonly = other.m_readonly;

        other.m_buffer = nullptr;
        other.m_buffer_ro = nullptr;
        other.m_length = 0;
        other.m_capacity = 0;
        other.m_position = 0;
        other.m_subBytePosition = 0;
        other.m_readonly = false;
    }

    return *this;
}

void BinaryStream::Ensure(size_t free) {
    if (m_readonly) return;
    assert((m_capacity >= m_position) && "stream pos is outside buffer area");

    const size_t currentFree = m_capacity - m_position;
    if (currentFree >= free) return;

    this->Reallocate(m_position + free);
}

void BinaryStream::Seek(size_t position) {
    // set position, but for safety clamp to the range [0, length]
    m_position = std::min(std::max(position, size_t(0)), m_length);
}

bool BinaryStream::Skip(size_t num) {
    if (IsEOF(num)) return false;
    m_position += num;
    return true;
}

void BinaryStream::Reset() {
    if (m_buffer) {
        assert(m_buffer_ro);
        if (!m_readonly)
            delete[] m_buffer;
        m_buffer = nullptr;
    }

    m_buffer_ro = nullptr;
    m_length = 0;
    m_position = 0;
    m_capacity = 0;
}

void BinaryStream::Clear() {
    if (m_readonly) return;
    // TODO: Could optionally memset to zero. Is there a security issue associated with not doing this?
    m_position = 0;
    m_length = 0;
}

std::unique_ptr<uint8_t[]> BinaryStream::ToArray() const {
    const auto bufferlen = m_length * sizeof(decltype(*m_buffer));
    auto ret = std::unique_ptr<uint8_t[]>(new uint8_t[bufferlen]);
    memcpy(ret.get(), m_buffer, bufferlen);
    return ret;
}

std::unique_ptr<uint8_t[]> BinaryStream::ReleaseBuffer(size_t* length) noexcept {
    if (m_readonly) return nullptr;
    Align();

    // fill in length output if specified
    if (length)
        *length = m_length;

    // keep temporary ref to buffer, and clear all state
    auto* ptr = m_buffer;
    m_buffer = nullptr;
    m_length = 0;
    m_position = 0;
    m_capacity = 0;

    return std::unique_ptr<uint8_t[]>(ptr);
}

void BinaryStream::Write(ISerializable& obj) {
    if (m_readonly) return;
    Align();
    obj.Serialize(*this);
}

void BinaryStream::WriteByte(const uint8_t val) {
    if (m_readonly) return;
    Align();
    Ensure(1);
    m_buffer[m_position++] = val;
    m_length = std::max(m_length, m_position);
}

void BinaryStream::WriteBytes(const BinaryStream& other) {
    WriteBytes(other.m_buffer_ro, other.m_length);
}

void BinaryStream::WriteZeroes(size_t len) {
    if (m_readonly) return;
    Align();
    Ensure(len);
    memset(m_buffer + m_position, 0, len);
    m_position += len;
    m_length = std::max(m_length, m_position);
}

void BinaryStream::WriteBytes(const void* addr, const size_t len) {
    if (m_readonly) return;
    Align();
    Ensure(len);
    memcpy(m_buffer + m_position, addr, len);
    m_position += len;
    m_length = std::max(m_length, m_position);
}

void BinaryStream::WriteInt16(uint16_t val) {
    val = asio::detail::socket_ops::host_to_network_short(val);
    WriteBytes(&val, sizeof val);
}

void BinaryStream::WriteInt32(uint32_t val) {
    val = asio::detail::socket_ops::host_to_network_long(val);
    WriteBytes(&val, sizeof val);
}

void BinaryStream::WriteInt64(uint64_t val) {
    if (IsLittleEndian()) {
        WriteInt32((val >> std::numeric_limits<uint32_t>::digits) & std::numeric_limits<uint32_t>::max());
        WriteInt32(val & std::numeric_limits<uint32_t>::max());
    } else {
        WriteBytes(&val, sizeof val);
    }
}

void BinaryStream::Write7BitEncodedInt(int val) {
    // Custom encoding: write first 7 bits (starting from little end), rightshift source value by 7 bits.
    // If more to follow, set MSB of output byte to 1. Repeat until source value == 0.
    // This lets us encode small-ish ints very compactly (1-2 bytes, instead of always 4).
    // Inspired by similar function in .NET's System.IO.BinaryWriter.

    // todo: check if cast overflows as desired
    // todo: check endian-correctness of this function
    //val = asio::detail::socket_ops::host_to_network_long(val);
    auto raw = static_cast<uint32_t>(val);

    do {
        uint8_t snippet = raw & 0x7F;

        // shift to the next 7 bits, and set MSB if there's still more to follow
        raw >>= 7;
        if (raw > 0) snippet |= 0x80;

        WriteByte(snippet);
    } while (raw > 0);
}

void BinaryStream::WriteBool(bool val) {
    if (m_readonly) return;
    Ensure(1);

    // for first bit written, ensure the byte is initialized to zero
    if (m_subBytePosition == 0)
        m_buffer[m_position] = 0;

    // set specific bit to 1, or leave it at 0
    if (val)
        m_buffer[m_position] |= (0x80 >> m_subBytePosition);
    m_subBytePosition++;

    if (m_subBytePosition > 7) {
        m_position++;
        m_subBytePosition = 0;
        m_length = std::max(m_length, m_position);

    } else {
        m_length = std::max(m_length, m_position + 1);
    }
}

void BinaryStream::WriteString(const std::string& str) {
    // length may be up to 32 bits long
    assert(str.size() < static_cast<size_t>(std::numeric_limits<int>::max()));

    // length-prefixed string, with length saved as compact 7-bit int
    Write7BitEncodedInt(static_cast<int>(str.size()));
    WriteBytes(str.c_str(), str.size());
}

void BinaryStream::Read(ISerializable& obj) {
    Align();
    obj.Deserialize(*this);
}

uint8_t BinaryStream::ReadByte() {
    if (IsEOF(1)) return 0;
    Align();

    return m_buffer_ro[m_position++];
}

void BinaryStream::ReadBytes(uint8_t* buffer, size_t count) {
    if (IsEOF(count) || !buffer) return;
    Align();

    memcpy(buffer, m_buffer_ro + m_position, count);
    m_position += count;
}

uint16_t BinaryStream::ReadUInt16() {
    // read 2 bytes from buffer, and endian swap
    uint16_t ret;
    ReadBytes(reinterpret_cast<uint8_t*>(&ret), sizeof ret); //-V206 : fully intentional behavior
    return asio::detail::socket_ops::network_to_host_short(ret);
}

uint32_t BinaryStream::ReadUInt32() {
    // read 4 bytes from buffer, and endian swap
    uint32_t ret;
    ReadBytes(reinterpret_cast<uint8_t*>(&ret), sizeof ret); //-V206 : fully intentional behavior
    return asio::detail::socket_ops::network_to_host_long(ret);
}

uint64_t BinaryStream::ReadUInt64() {
    uint64_t ret;
    if (IsLittleEndian()) {
        // we can only endian-swap 32 bits at a time, so read two uint32s and OR them together
        ret = static_cast<uint64_t>(ReadUInt32()) << std::numeric_limits<uint32_t>::digits
            | static_cast<uint64_t>(ReadUInt32());
    } else {
        // on a big endian system, we can directly copy the network order (big endian) number
        ReadBytes(reinterpret_cast<uint8_t*>(&ret), sizeof ret); //-V206 : fully intentional behavior
    }
    return ret;
}

bool BinaryStream::ReadBool() {
    //return ReadByte() != 0;
    if (IsEOF(1)) return false;

    bool bit = (m_buffer_ro[m_position] & (0x80 >> m_subBytePosition)) != 0;

    m_subBytePosition++;
    if (m_subBytePosition > 7) {
        m_subBytePosition = 0;
        m_position++;
    }

    return bit;
}

int BinaryStream::Read7BitEncodedInt() {
    // See Write7BitEncodedInt for encoding details

    uint32_t output = 0;
    for (int i = 0; i < 5; i++) {
        const uint8_t snippet = ReadByte();

        // append the snippet to the output number
        const size_t offset = 7 * i;
        output |= (snippet & 0x7F) << offset;

        // if MSB is not set, then the 7-bit int has ended
        if ((snippet & 0x80) == 0)
            break;
    }

    //output = asio::detail::socket_ops::network_to_host_long(output);
    return static_cast<int>(output);
}

std::string BinaryStream::ReadString() {
    const size_t length = static_cast<size_t>(Read7BitEncodedInt());
    if (IsEOF(length)) return std::string();

    std::string ret = std::string(reinterpret_cast<const char*>(m_buffer_ro + m_position), length);
    m_position += length;

    return ret;
}

void BinaryStream::Align() {
    if (m_subBytePosition > 0)
        m_position++;

    m_subBytePosition = 0;
}

void BinaryStream::Reallocate(size_t newCapacity) {
    if (m_readonly) return;

    // allocate a new buffer
    newCapacity = std::max(ToNextPowerOfTwo(newCapacity), size_t(1));
    uint8_t* newBuffer = new uint8_t[newCapacity];

    // copy from the previous buffer as much as we can fit
    if (m_buffer) {
        const auto bytesToCopy = std::min(m_length, newCapacity);
        if (bytesToCopy > 0)
            memcpy(newBuffer, m_buffer, bytesToCopy);
        delete[] m_buffer;
    }

    m_buffer = newBuffer;
    m_buffer_ro = newBuffer;
    m_capacity = newCapacity;
}

bool BinaryStream::IsLittleEndian() {
    // https://stackoverflow.com/a/28592202
    // if htonl changes the given value, then it had to swap to big endian,
    // which means the local machine is little endian
    return asio::detail::socket_ops::host_to_network_long(1) != 1;
}
