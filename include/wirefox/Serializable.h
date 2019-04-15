#pragma once
#include "BinaryStream.h"

namespace wirefox {

    /**
     * \brief A wrapper meant to help you add network serialization features to your objects.
     * 
     * You can pass an object that derives from ISerializable, directly to
     * BinaryStream::Write(ISerializable&). The advantage of this pattern is that you can keep
     * all the explicit reading/writing logic inside the class itself, without having to
     * manually declare an interface like this one.
     * 
     * In the implementation, you should use the WriteFoo() and ReadFoo() functions as normal.
     * 
     * \note This construct is completely optional. Feel free to not use it if you want.
     */
    class WIREFOX_API ISerializable {
    protected:
        ~ISerializable() = default;

    public:
        ISerializable() = default;

        /**
         * \brief Write the object to an output stream.
         * 
         * In a derived class, write all relevant state and data that needs to be sent over the
         * network, to the output stream.
         * 
         * \param[out]  outstream   A stream where data can be written to.
         */
        virtual void Serialize(BinaryStream& outstream) = 0;

        /**
         * \brief Read object state from an input stream.
         * 
         * In a derived class, read the state you wrote in Serialize(), and 'fill in' this
         * instance of your class.
         * 
         * \param[in]   instream    A stream you can read from.
         */
        virtual void Deserialize(BinaryStream& instream) = 0;
    };

}
