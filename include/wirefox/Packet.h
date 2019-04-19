/**
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#pragma once
#include "Enumerations.h"
#include "Factory.h"

namespace wirefox {

    class BinaryStream;

    /**
     * \brief Represents a network message.
     * 
     * This object contains a message (datagram) that can be sent over or received from the network. Aside from raw data,
     * it also contains a PacketCommand, which describes the intended function of this packet.
     */
    class Packet {
    public:
        /// A factory tool. Use Factory::Create() and Factory::Destroy() to (de)allocate instances of Packet.
        /// Note that it is fine to create Packets on the stack; this Factory is simply recommended when allocating on the heap.
        typedef detail::Factory<Packet> Factory;

        /// Copy constructor. Creates a Packet that is a full copy of another Packet.
        WIREFOX_API Packet(const Packet& other);

        /// Move constructor. Creates a Packet that takes ownership of another Packet's contents.
        WIREFOX_API Packet(Packet&& rhs) noexcept;

        /// Creates a Packet that contains a copy of the specified data buffer.
        WIREFOX_API Packet(PacketCommand cmd, const uint8_t* data, size_t datalen);

        /// Creates a Packet that contains a copy of a BinaryStream's data buffer.
        WIREFOX_API Packet(PacketCommand cmd, const BinaryStream& data);

        /// Creates a Packet that takes ownership of a BinaryStream's data buffer.
        WIREFOX_API Packet(PacketCommand cmd, BinaryStream&& data);

        /// Destroys this Packet and releases its resources.
        WIREFOX_API ~Packet() = default;

        /// Copy assignment operator.
        WIREFOX_API Packet&             operator=(const Packet& rhs);
        /// Move assignment operator.
        WIREFOX_API Packet&             operator=(Packet&& rhs) noexcept;

        /// Returns the PacketCommand that was specified when this Packet was created.
        WIREFOX_API PacketCommand       GetCommand() const noexcept { return m_command; }

        /// Returns the data buffer that is owned by this Packet.
        WIREFOX_API const uint8_t*      GetBuffer() const noexcept { return m_data.get(); }

        /**
         * \brief Returns a \b read-only BinaryStream that wraps around this packet.
         * 
         * This function is faster and more efficient than constructing your own BinaryStream around GetBuffer(),
         * because this will not make a copy of the data.
         */
        WIREFOX_API BinaryStream        GetStream() const;

        /// Returns the length, in bytes, of the data buffer.
        WIREFOX_API size_t              GetLength() const noexcept { return m_length; }

        /// Returns the PeerID of whoever created and sent this Packet.
        /// \note If this Packet was never sent over the network (e.g. created by you), the sender is invalid.
        WIREFOX_API PeerID              GetSender() const noexcept { return m_sender; }

        /// Sets the PeerID of the sender associated with this Packet.
        WIREFOX_API void                SetSender(PeerID sender) noexcept { m_sender = sender; }

        /// [Internal use only.] Serializes this Packet to a BinaryStream.
        void                            ToDatagram(BinaryStream& outstream) const;

        /// [Internal use only.] Deserializes and returns a new Packet from a BinaryStream.
        static Packet                   FromDatagram(PeerID sender, BinaryStream& instream, size_t len);

    private:
        Packet(PeerID sender, BinaryStream& instream, size_t len);

        PeerID                      m_sender;
        PacketCommand               m_command;
        size_t                      m_length;
        std::unique_ptr<uint8_t[]>  m_data;
    };

}
