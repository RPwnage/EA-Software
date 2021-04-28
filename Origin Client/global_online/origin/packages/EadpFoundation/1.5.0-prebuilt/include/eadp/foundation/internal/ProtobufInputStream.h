// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <stdint.h>
#include <eadp/foundation/Config.h>

namespace eadp
{
namespace foundation
{
namespace Internal
{

class ProtobufMessage;

class EADPSDK_API ProtobufInputStream
{
public:
    ProtobufInputStream(uint8_t* buffer, size_t bufferSize);

    ~ProtobufInputStream() = default;

    bool isFullyConsumed();

    bool readInt32(int32_t *value);
    bool readInt64(int64_t *value);
    bool readUInt32(uint32_t *value);
    bool readUInt64(uint64_t *value);
    bool readSInt32(int32_t *value);
    bool readSInt64(int64_t *value);
    bool readFixed32(uint32_t *value);
    bool readFixed64(uint64_t *value);
    bool readSFixed32(int32_t *value);
    bool readSFixed64(int64_t *value);
    bool readFloat(float *value);
    bool readDouble(double *value);
    bool readBool(bool *value);
    bool readEnum(int32_t *value);
    bool readEnum(int32_t *value, int32_t *bytesConsumed);

    bool readBytes(eadp::foundation::String *value);
    bool readBytes(eadp::foundation::Vector<eadp::foundation::String>& value);

    bool readString(eadp::foundation::String *value);
    bool readString(eadp::foundation::Vector<eadp::foundation::String>& value);

    bool readPackedInt32(Vector<int32_t> *value);
    bool readPackedInt64(Vector<int64_t> *value);
    bool readPackedUInt32(Vector<uint32_t> *value);
    bool readPackedUInt64(Vector<uint64_t> *value);
    bool readPackedSInt32(Vector<int32_t> *value);
    bool readPackedSInt64(Vector<int64_t> *value);
    bool readPackedFixed32(Vector<uint32_t> *value);
    bool readPackedFixed64(Vector<uint64_t> *value);
    bool readPackedSFixed32(Vector<int32_t> *value);
    bool readPackedSFixed64(Vector<int64_t> *value);
    bool readPackedFloat(Vector<float> *value);
    bool readPackedDouble(Vector<double> *value);
    bool readPackedBool(Vector<bool> *value);

    template<typename T>
    bool readPackedEnum(Vector<T> *value)
    {
        int32_t length;
        if (!readInt32(&length))
        {
            return false;
        }
        while (length > 0)
        {
            int32_t element;
            int32_t bytesRead;
            if (!readEnum(&element, &bytesRead))
            {
                return false;
            }
            value->push_back(static_cast<T>(element));
            length -= bytesRead;
        }
        return length == 0;
    }

    bool readMessage(ProtobufMessage* value);

    bool expectAtEnd();
    bool expectTag(uint32_t tag);

    uint32_t readTag();

    bool skipField(uint32_t tag);

    using Limit = size_t;

    // Sets a new byte limit from the current position for bounds checking.
    // returns current limit, which should be restored when element read is complete
    Limit setLimit(uint32_t length);

    // Restores the previous bounds limit which was returned by a setLimit call.
    void restoreLimit(Limit limit);

private:
    uint8_t* getBufferPosition() const;
    uint32_t getBytesUntilLimit() const;

    bool readVarint(uint64_t* value);

    bool skipBytes(uint32_t count);

    uint8_t* m_buffer;
    size_t m_currLimit;
    size_t m_currPosition;

    bool m_endOfMessage;
};


}
}
}
