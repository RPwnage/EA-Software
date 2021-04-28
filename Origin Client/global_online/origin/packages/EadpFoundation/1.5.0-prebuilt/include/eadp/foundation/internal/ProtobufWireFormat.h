// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Config.h>
#include <eadp/foundation/internal/ProtobufMessage.h>

namespace eadp
{
namespace foundation
{
namespace Internal
{

class EADPSDK_API ProtobufWireFormat
{
public:
    enum WireType
    {
        VARINT = 0,
        FIXED64 = 1,
        LENGTH_DELIMITED = 2,
        START_GROUP = 3,
        END_GROUP = 4,
        FIXED32 = 5,
    };

    static uint32_t getTagFieldNumber(uint32_t tag);
    static uint8_t* writeTag(int fieldNumber, WireType type, uint8_t* target);

    static uint32_t getInt32Size(int32_t);
    static uint8_t* writeInt32(int32_t value, uint8_t* target);
    static uint8_t* writeInt32(int32_t fieldNumber, int32_t value, uint8_t* target);

    static uint32_t getInt64Size(int64_t);
    static uint8_t* writeInt64(int64_t value, uint8_t* target);
    static uint8_t* writeInt64(int fieldNumber, int64_t value, uint8_t* target);

    static uint32_t getUInt32Size(uint32_t);
    static uint8_t* writeUInt32(uint32_t value, uint8_t* target);
    static uint8_t* writeUInt32(int fieldNumber, uint32_t value, uint8_t* target);

    static uint32_t getUInt64Size(uint64_t);
    static uint8_t* writeUInt64(uint64_t value, uint8_t* target);
    static uint8_t* writeUInt64(int fieldNumber, uint64_t value, uint8_t* target);

    static uint32_t getSInt32Size(int32_t);
    static uint8_t* writeSInt32(int32_t value, uint8_t* target);
    static uint8_t* writeSInt32(int fieldNumber, int32_t value, uint8_t* target);

    static uint32_t getSInt64Size(int64_t);
    static uint8_t* writeSInt64(int64_t value, uint8_t* target);
    static uint8_t* writeSInt64(int fieldNumber, int64_t value, uint8_t* target);

    static uint8_t* writeFixed32(uint32_t value, uint8_t* target);
    static uint8_t* writeFixed32(int fieldNumber, uint32_t value, uint8_t* target);

    static uint8_t* writeFixed64(uint64_t value, uint8_t* target);
    static uint8_t* writeFixed64(int fieldNumber, uint64_t value, uint8_t* target);

    static uint8_t* writeSFixed32(int32_t value, uint8_t* target);
    static uint8_t* writeSFixed32(int fieldNumber, int32_t value, uint8_t* target);

    static uint8_t* writeSFixed64(int64_t value, uint8_t* target);
    static uint8_t* writeSFixed64(int fieldNumber, int64_t value, uint8_t* target);

    static uint8_t* writeFloat(float value, uint8_t* target);
    static uint8_t* writeFloat(int fieldNumber, float value, uint8_t* target);

    static uint8_t* writeDouble(double value, uint8_t* target);
    static uint8_t* writeDouble(int fieldNumber, double value, uint8_t* target);

    static uint8_t* writeBool(bool value, uint8_t* target);
    static uint8_t* writeBool(int fieldNumber, bool value, uint8_t* target);

    static uint32_t getEnumSize(int32_t);
    static uint8_t* writeEnum(int32_t value, uint8_t* target);
    static uint8_t* writeEnum(int fieldNumber, int32_t value, uint8_t* target);

    static uint32_t getBytesSize(const String& value);
    static uint8_t* writeBytes(int32_t fieldNumber, const String& value, uint8_t* buffer);

    static uint32_t getStringSize(const String& value);
    static uint8_t* writeString(int32_t fieldNumber, const String& value, uint8_t* buffer);

    static uint32_t getMessageSize(ProtobufMessage& value);
    static uint8_t* writeMessage(int32_t fieldNumber, const ProtobufMessage& value, uint8_t* target);

};

}
}
}
