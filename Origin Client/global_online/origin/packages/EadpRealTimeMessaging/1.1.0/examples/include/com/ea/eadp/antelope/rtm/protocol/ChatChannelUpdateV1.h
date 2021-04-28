// GENERATED CODE (Source: chat_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Allocator.h>
#include <eadp/foundation/Service.h>
#include <eadp/foundation/internal/ProtobufMessage.h>

namespace com
{

namespace ea
{

namespace eadp
{

namespace antelope
{

namespace rtm
{

namespace protocol
{

class EADPGENERATED_API ChatChannelUpdateV1 : public ::eadp::foundation::Internal::ProtobufMessage
{
public:

    /*!
     * Constructs an empty message. Memory allocated by the message will use the provided allocator.
     */
    explicit ChatChannelUpdateV1(const ::eadp::foundation::Allocator& allocator);

    /*!
     * Clear a message, so that all fields are reset to their default values.
     */
    void clear();

    /**@{
     */
    const ::eadp::foundation::String& getChannelId() const;
    void setChannelId(const ::eadp::foundation::String& value);
    void setChannelId(const char8_t* value);
    /**@}*/

    /**@{
     *  format: yyyy-MM-ddTHH:mm:ss.SSSZ
     *  eg. "2019-12-10T15:40:42.422-08:00"
     */
    const ::eadp::foundation::String& getLastReadTimestamp() const;
    void setLastReadTimestamp(const ::eadp::foundation::String& value);
    void setLastReadTimestamp(const char8_t* value);
    /**@}*/

    /**@{
     *  length limit 1000
     *  If customProperties field is not present in the request, the existing customProperties on server will remain.
     *  To clear the existing customProperties, send empty string for customProperties field.
     */
    const ::eadp::foundation::String& getCustomProperties() const;
    void setCustomProperties(const ::eadp::foundation::String& value);
    void setCustomProperties(const char8_t* value);
    /**@}*/

    /*!
     * Serializes the message contents into a buffer.
    Called by serialize.
     */
    uint8_t* onSerialize(uint8_t* target) const override;

    /*!
     * Deserializes the message contents from an input stream.
    Called by deserialize.
     */
    bool onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input) override;

    /*!
     * Returns a previously calculated serialized size of the message in protobuf wire format.
     */
    size_t getCachedSize() const override
    {
        return m_cachedByteSize_;
    }

    /*!
     * Calculates and returns the serialized size of the message in protobuf wire format.
     */
    size_t getByteSize() override;

    /*!
     * Returns a string representing the contents of the message for debugging and logging.
     */
    ::eadp::foundation::String toString(const ::eadp::foundation::String& indent) const override;

private:
    ::eadp::foundation::Allocator m_allocator_;
    size_t m_cachedByteSize_;
    ::eadp::foundation::String m_channelId;
    ::eadp::foundation::String m_lastReadTimestamp;
    ::eadp::foundation::String m_customProperties;

};

}
}
}
}
}
}
