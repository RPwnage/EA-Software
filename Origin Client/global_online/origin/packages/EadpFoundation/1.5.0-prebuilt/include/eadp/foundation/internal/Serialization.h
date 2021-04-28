// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <stdint.h>

#include <eadp/foundation/Error.h>
#include <eadp/foundation/internal/ProtobufMessage.h>

namespace eadp
{
namespace foundation
{
namespace Internal
{

// Something to pass around the bytes. In order to be efficient, this class needs to implement move semantics correctly. 
class EADPSDK_API RawBuffer
{
public:
    RawBuffer(); /**< will construct an invalid buffer, for container and other special use case only, be really careful and double check your code if using it */
    explicit RawBuffer(const Allocator& allocator);
    RawBuffer(const Allocator& allocator, uint32_t size);
    RawBuffer(const RawBuffer&) = delete; // cannot copy
    RawBuffer(RawBuffer&& buffer);
    ~RawBuffer();

    void allocate(uint32_t bufferSize);
    void free();

    uint8_t* getBytes() const;
    char* getBytesAsCharPtr() const;
    uint32_t getSize() const;

    bool empty() const;
    uint8_t& operator[] (const int index);
    const uint8_t& operator[] (const int index) const;
    RawBuffer& operator= (const RawBuffer& buffer) = delete;
    RawBuffer& operator= (RawBuffer&& buffer);

    friend void swap(RawBuffer& first, RawBuffer& second);

private:
    Allocator m_allocator;
    uint32_t m_size;
    uint8_t* m_bytes;
};

class EADPSDK_API Encoder
{
public:
    Encoder(Allocator& allocator) : m_allocator(allocator) {}

    Encoder(const Encoder&) = delete;
    Encoder(Encoder&&) = delete;
    Encoder& operator= (const Encoder&) = delete;
    Encoder& operator= (Encoder&&) = delete;

    ErrorPtr encode(ProtobufMessage* msg, RawBuffer& buffer);

private:
    Allocator& m_allocator;
};

class EADPSDK_API Decoder
{
public:
    Decoder(Allocator& allocator) : m_allocator(allocator) {}
    Decoder(const Decoder&) = delete;
    Decoder(Decoder&&) = delete;
    Decoder& operator= (const Decoder&) = delete;
    Decoder& operator= (Decoder&&) = delete;

    ErrorPtr decode(const RawBuffer& buffer, ProtobufMessage* msg);

private:
    Allocator& m_allocator;
};


}
}
}
