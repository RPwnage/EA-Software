// GENERATED CODE (Source: social_protocol.proto) - DO NOT EDIT DIRECTLY.
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

namespace protocol
{

class EADPGENERATED_API HistoryRequest : public ::eadp::foundation::Internal::ProtobufMessage
{
public:

    /*!
     * Constructs an empty message. Memory allocated by the message will use the provided allocator.
     */
    explicit HistoryRequest(const ::eadp::foundation::Allocator& allocator);

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
     */
    const ::eadp::foundation::String& getTimestamp() const;
    void setTimestamp(const ::eadp::foundation::String& value);
    void setTimestamp(const char8_t* value);
    /**@}*/

    /**@{
     */
    int32_t getRecordCount() const;
    void setRecordCount(int32_t value);
    /**@}*/

    /**@{
     *  The filter types will only accept ChannelMessageType values in it
     */
    const ::eadp::foundation::String& getPrimaryFilterType() const;
    void setPrimaryFilterType(const ::eadp::foundation::String& value);
    void setPrimaryFilterType(const char8_t* value);
    /**@}*/

    /**@{
     */
    const ::eadp::foundation::String& getSecondaryFilterType() const;
    void setSecondaryFilterType(const ::eadp::foundation::String& value);
    void setSecondaryFilterType(const char8_t* value);
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
    ::eadp::foundation::String m_timestamp;
    int32_t m_recordCount;
    ::eadp::foundation::String m_primaryFilterType;
    ::eadp::foundation::String m_secondaryFilterType;

};

}
}
}
}
}
