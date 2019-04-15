#pragma once

namespace wirefox {

    class ISerializable;

    /**
     * \brief Represents an in-memory byte stream.
     * 
     * This class manages a resizable byte array, and provides functions for reading from and writing
     * to that internal buffer. It's intended to be an easy method for (de)serializing your data to
     * and from network packets.
     * 
     * When using the read and write primitives (WriteInt32() etc.) this class will transparently
     * perform endianness-conversion for you as needed.
     * 
     * \par Example
     * \code
     * BinaryStream buffer;
     * buffer.WriteInt32(42);
     * buffer.WriteString("Hello, world!");
     * 
     * Packet packet(buffer);
     * \endcode
     * 
     * \sa wirefox::ISerializable, wirefox::Packet
     */
    class WIREFOX_API BinaryStream {
    public:
        /// Specifies how a BinaryStream should wrap a user data buffer.
        enum class WrapMode {
            COPY,           ///< The user buffer should be copied, so it becomes writable.
            READONLY        ///< The user buffer should be wrapped directly, as a read-only stream.
        };

        /// Constructs an empty, writable BinaryStream with a specified (or default) storage capacity.
        BinaryStream(size_t capacity = cfg::BINARYSTREAM_DEFAULT_CAPACITY);

        /// Constructs a BinaryStream that wraps around a buffer.
        BinaryStream(const uint8_t* buffer, size_t bufferlen, WrapMode mode = WrapMode::COPY);

        /// Constructs a BinaryStream that is a copy of another BinaryStream.
        BinaryStream(const BinaryStream& other);

        /// Constructs a BinaryStream that moves the buffer from another BinaryStream. The other BinaryStream is reset.
        BinaryStream(BinaryStream&& other) noexcept;

        /// Destroys this BinaryStream and releases the internal buffer, if any.
        ~BinaryStream();

        /// Copies the buffer and internal state from another BinaryStream into this instance.
        BinaryStream&   operator=(const BinaryStream&);

        /// Moves the buffer and internal state from another BinaryStream into this instance. The other BinaryStream is reset.
        BinaryStream&   operator=(BinaryStream&&) noexcept;

        /**
         * \brief Ensures that at least \p free bytes are free.
         * 
         * Call this function when you expect to make many writes to the stream, and know upfront how much space you need in
         * total. This function will reallocate the internal buffer to accommodate \p free extra bytes, if needed.
         * \param[in]   free    The number of free bytes required
         */
        void            Ensure(size_t free);

        /**
         * \brief Move the read/write head position to the specified position in bytes.
         * 
         * Positions that are outside the stream bounds (lower than 0, or higher than GetLength()) will be clamped.
         * \param[in]   position    The new absolute stream position, in bytes.
         */
        void            Seek(size_t position);

        /// Seek to the beginning of the stream. Equivalent to \p Seek(0) .
        void            SeekToBegin() { Seek(0); }

        /// Seek to the end of the stream. Equivalent to \p Seek(GetLength()) .
        void            SeekToEnd() { Seek(GetLength()); }

        /// Attempts to advance position so that \p num bytes are skipped. Returns false if EOF will be reached before \p num bytes.
        bool            Skip(size_t num);

        /// Deallocates the internal buffer and resets the stream.
        void            Reset();

        /// Resets the stream length and position, but keeps the memory allocated so far.
        void            Clear();

        /**
         * \brief Make a copy of the internal buffer.
         * 
         * This will return a \b copy of the stream buffer. Any unused capacity in the BinaryStream is not included, the
         * returned array is only large enough to fit the actual written buffer contents.
         */
        std::unique_ptr<uint8_t[]> ToArray() const;

        /// Gets the total capacity, in bytes, of the stream. This is how big the stream can grow before it must be reallocated.
        size_t          GetCapacity() const noexcept { return m_capacity; }

        /// Gets the total length, in bytes, of the stream. This is the number of meaningful bytes that were written to the buffer.
        size_t          GetLength() const noexcept { return m_length; }

        /// Gets the current stream position in bytes. This is the offset at which the next read or write will take place.
        size_t          GetPosition() const noexcept { return m_position; }

        /// Gets the internal buffer.
        const uint8_t*  GetBuffer() const noexcept { return m_buffer_ro; }

        /**
         * \brief Yield ownership of the internal buffer.
         * 
         * Returns a pointer to the internal buffer, optionally writes the buffer length to \p length, and clears the
         * stream's internal state. After this function returns, you gain full ownership of the pointer.
         * 
         * \param[out]  length      If not \p nullptr, the buffer's length in bytes will be written here
         */
        std::unique_ptr<uint8_t[]> ReleaseBuffer(size_t* length = nullptr) noexcept;

        /**
         * \brief Checks if the end of the stream has been reached.
         * 
         * Returns true if the distance between the stream position and the stream length is less than \p read.
         * 
         * \attention   Never attempt to read past the end of the stream. Some safeguards may be in place, but
         *              in general this will result in reading garbage data, memory corruption or outright crashes.
         * 
         * \param[in]   read    The number of bytes you intend to read from the current position onward
         */
        bool            IsEOF(const size_t read = 1) const noexcept { return m_position > (m_length - read); }

        /**
         * \brief Checks whether this stream is read-only.
         * 
         * Returns true if the stream was constructed using a buffer pointer and readonly_t. Refer to the constructor
         * BinaryStream(uint8_t*, size_t, readonly_t) for more information.
         * 
         * If this function returns true, it is not allowed to call any function that may attempt to mutate the buffer
         * (e.g. Write(), Clear(), Reset(), Ensure()).
         */
        bool            IsReadOnly() const noexcept { return m_readonly; }

        /**
         * \brief Checks whether this stream is currently empty.
         * 
         * This function returns true if either the stream has no backing buffer, or if the stream length is zero.
         */
        bool            IsEmpty() const noexcept { return !m_buffer_ro || m_length == 0; }

        /**
         * \brief Writes an arbitrary object to the stream.
         * 
         * This function lets you write any object to the stream as raw bytes. The object is converted to a pointer,
         * and \p sizeof(T) bytes are written to the stream. This function is provided as a convenience only; in most
         * cases you probably want to control the writes more directly.
         * 
         * \warning     This function does not perform endianness-conversion; the object is treated as an opaque blob.
         * \warning     Only use this function with POD (plain-old-data) types. Machine-specific data (such as pointers
         *              or vtables) are never compatible with other machines.
         * 
         * \param[in]   val     A reference to the object to serialize.
         */
        template<typename T>
        void            Write(const T& val);

        /**
         * \brief Reads an arbitrary object from the stream.
         * 
         * Counterpart to Write(). This function lets you read a POD object from the stream, treating it as an opaque
         * blob of bytes. This function is provided as a convenience only; in most cases you probably want finer
         * control over your writes.
         * 
         * \attention   Depending on your compiler, and compiler configuration, the object's copy constructor may be
         *              invoked to copy the read object back to the caller. This may or may not be a problem for you.
         */
        template<typename T>
        T               Read();

        /**
         * \brief Calls the ISerializable::Serialize() function on \p obj.
         * \param[in]   obj     The object to serialize
         */
        void            Write(ISerializable& obj);

        /// Writes one unsigned 8-bit integer to the stream.
        void            WriteByte(uint8_t val);

        /**
         * \brief Copies the contents of the specified BinaryStream into this one, writing it as an opaque blob starting
         * from the current stream position.
         * 
         * \param[in]   other   The BinaryStream whose contents to copy.
         */
        void            WriteBytes(const BinaryStream& other);

        /**
         * \brief Writes an array of bytes to the stream.
         * \param[in]   addr    The memory address at which to begin copying
         * \param[in]   len     The number of bytes to copy from \p addr
         */
        void            WriteBytes(const void* addr, size_t len);

        /// Writes one 16-bit integer to the stream.
        void            WriteInt16(uint16_t val);

        /// Writes one 32-bit integer to the stream.
        void            WriteInt32(uint32_t val);

        /// Writes one 64-bit integer to the stream.
        void            WriteInt64(uint64_t val);

        /**
         * \brief Writes a variable-length integer to the stream, up to 32 bits in total length.
         * 
         * This function uses a custom encoding, where 7 bits are written at a time. If more space is needed, the 8th
         * bit is set to indicate another byte is following. This allows for encoding 32-bit integers very compactly,
         * if it is expected that they generally are only a few digits long.
         * 
         * \param[in]   val     The value to write
         */
        void            Write7BitEncodedInt(int val);

        /// Writes a single bit to the stream.
        void            WriteBool(bool val);

        /// Writes a length-prefixed string to the stream.
        void            WriteString(const std::string& str);

        /**
         * \brief Calls the ISerializable::Deserialize() function on \p obj.
         * \param[in]   obj     The object to deserialize
         */
        void            Read(ISerializable& obj);

        /// Reads an 8-bit unsigned integer from the stream.
        uint8_t         ReadByte();

        /**
         * \brief Reads an array of bytes from the stream.
         * 
         * You are responsible for ensuring that \p buffer is large enough to hold \p count bytes.
         * 
         * \param[in]   buffer  A user-provided buffer where the read bytes can be written to
         * \param[in]   count   The number of bytes to read from the stream
         */
        void            ReadBytes(uint8_t* buffer, size_t count);

        /// Reads a 16-bit signed integer from the stream.
        int16_t         ReadInt16() { return static_cast<int16_t>(ReadUInt16()); }

        /// Reads a 32-bit signed integer from the stream.
        int32_t         ReadInt32() { return static_cast<int32_t>(ReadUInt32()); }

        /// Reads a 64-bit signed integer from the stream.
        int64_t         ReadInt64() { return static_cast<int64_t>(ReadUInt64()); }

        /// Reads a 16-bit unsigned integer from the stream.
        uint16_t        ReadUInt16();

        /// Reads a 32-bit unsigned integer from the stream.
        uint32_t        ReadUInt32();

        /// Reads a 64-bit unsigned integer from the stream.
        uint64_t        ReadUInt64();

        /// Reads a single bit from the stream.
        bool            ReadBool();

        /**
         * \brief Reads a variable-length integer from the stream.
         *
         * Counterpart to Write7BitEncodedInt(). See that function for encoding details.
         * \sa Write7BitEncodedInt()
         */
        int             Read7BitEncodedInt();

        /// Reads a length-prefixed string from the stream.
        std::string     ReadString();

    private:
        void            Align();

        void            Reallocate(size_t newCapacity);
        static bool     IsLittleEndian();

        uint8_t*        m_buffer;
        const uint8_t*  m_buffer_ro;
        size_t          m_capacity;
        size_t          m_length;
        size_t          m_position;
        unsigned        m_subBytePosition;
        bool            m_readonly;
    };

    template<typename T>
    void BinaryStream::Write(const T& val) {
        WriteBytes(&val, sizeof(T));
    }

    template<typename T>
    T BinaryStream::Read() {
        T ret;
        ReadBytes(&ret, sizeof(T));
        return ret;
    }

}
