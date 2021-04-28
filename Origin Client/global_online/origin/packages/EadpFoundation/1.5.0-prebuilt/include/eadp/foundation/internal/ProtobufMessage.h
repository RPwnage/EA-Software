// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <stdint.h>
#include <eadp/foundation/Hub.h>
#include <eadp/foundation/Service.h>
#include <eadp/foundation/internal/ProtobufInputStream.h>

// Define EADP_FOUNDATION_PROTOBUF_DEPRECATED_ATTR to nothing to suppress the gRPC API deprecation warning.
// This macro could be defined as C++ deprecation mark to enable C++ warning for gRPC API deprecation mark.
// ref: https://en.cppreference.com/w/cpp/language/attributes/deprecated
#ifndef EADP_FOUNDATION_PROTOBUF_DEPRECATED_ATTR   
#define EADP_FOUNDATION_PROTOBUF_DEPRECATED_ATTR 
#endif // !EADP_FOUNDATION_PROTOBUF_DEPRECATED_ATTR

namespace eadp
{
namespace foundation
{
namespace Internal
{

class EADPSDK_API ProtobufMessage
{
public:
    /*!
     * Allocates a shared pointer to a message of a specified type.
     */
    template<typename T>
    static SharedPtr<T> makeShared(eadp::foundation::IService* service)
    {
        eadp::foundation::Allocator allocator = service->getHub()->getAllocator();
        return allocator.makeShared<T>(allocator);
    }

    /*!
    * Allocates a unique pointer to a message of a specified type.
    */
    template<typename T>
    static UniquePtr<T> makeUnique(eadp::foundation::IService* service)
    {
        eadp::foundation::Allocator allocator = service->getHub()->getAllocator();
        return allocator.makeUnique<T>(allocator);
    }

    /*!
    * Serializes the message contents into a buffer.
    */
    bool serialize(uint8_t* buffer, size_t bufferSize) const;

    /*!
    * Deserializes the message contents from a buffer.
    */
    bool deserialize(uint8_t* buffer, size_t bufferSize);

    /*!
    * Calculates the serialized size of the message in protobuf wire format.
    */
    virtual size_t getByteSize() = 0;

    /*!
    * Returns a previously calculated serialized size of the message in protobuf wire format.
    */
    virtual size_t getCachedSize() const = 0;

    /*!
    * Serializes the message contents into a buffer. Called by serialize.
    */
    virtual uint8_t* onSerialize(uint8_t* target) const = 0;

    /*!
    * Deserializes the message contents from an input stream. Called by deserialize.
    */
    virtual bool onDeserialize(ProtobufInputStream* input) = 0;

    /*!
    * Formats the message contents into a string for logging and debugging.
    */
    virtual String toString(const String& indent) const = 0;

    /*!
    * Returns the qualified type name of the message.
    */
    const String& getTypeName() { return m_typeName; }

    virtual ~ProtobufMessage() = default;

protected:
    /*!
    * Protected constructor which initializes the qualified type name of the message.
    */
    explicit ProtobufMessage(String&& typeName);

private:
    String m_typeName;
};

}
}
}
