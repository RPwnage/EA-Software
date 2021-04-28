// GENERATED CODE (Source: chat_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Allocator.h>
#include <eadp/foundation/Service.h>
#include <eadp/foundation/internal/ProtobufMessage.h>
#include <com/ea/eadp/antelope/rtm/protocol/ChannelType.h>

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

class EADPGENERATED_API ChatMembersRequestV1 : public ::eadp::foundation::Internal::ProtobufMessage
{
public:

    /*!
     * Constructs an empty message. Memory allocated by the message will use the provided allocator.
     */
    explicit ChatMembersRequestV1(const ::eadp::foundation::Allocator& allocator);

    /*!
     * Clear a message, so that all fields are reset to their default values.
     */
    void clear();

    /**@{
     */
    ::eadp::foundation::Vector< ::eadp::foundation::String>& getChannelId();
    void setChannelId(int index, const char8_t* value);
    void addChannelId(const char8_t* value);
    /**@}*/

    /**@{
     *  ChannelType for world chat pagination. All channels as a part of this request will be interpreted as this type.
     */
    com::ea::eadp::antelope::rtm::protocol::ChannelType getChannelType() const;
    void setChannelType(com::ea::eadp::antelope::rtm::protocol::ChannelType value);
    /**@}*/

    /**@{
     *  Pagination details. Allows the client to request (page_size) items starting at shard index (from_index).
     *  Responses for all channels passed as a part of this request will be paginated with these parameters.
     */
    int32_t getFromIndex() const;
    void setFromIndex(int32_t value);
    /**@}*/

    /**@{
     *  max page size is 25.
     */
    int32_t getPageSize() const;
    void setPageSize(int32_t value);
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
    ::eadp::foundation::Vector< ::eadp::foundation::String> m_channelId;
    int m_channelType;
    int32_t m_fromIndex;
    int32_t m_pageSize;

};

}
}
}
}
}
}
