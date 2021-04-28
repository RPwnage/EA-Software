// GENERATED CODE (Source: chat_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Allocator.h>
#include <eadp/foundation/Service.h>
#include <eadp/foundation/internal/ProtobufMessage.h>
#include <com/ea/eadp/antelope/rtm/protocol/ChannelMessageType.h>

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

/**
 *  Will return which chat channels a user is part of (ChatChannelsV1 e.g. 1:1 chats, group chats).
 *  This will not include any world chat channels the player might be part of.
 */
class EADPGENERATED_API ChatChannelsRequestV2 : public ::eadp::foundation::Internal::ProtobufMessage
{
public:

    /*!
     * Constructs an empty message. Memory allocated by the message will use the provided allocator.
     */
    explicit ChatChannelsRequestV2(const ::eadp::foundation::Allocator& allocator);

    /*!
     * Clear a message, so that all fields are reset to their default values.
     */
    void clear();

    /**@{
     */
    bool getIncludeLastMessageInfo() const;
    void setIncludeLastMessageInfo(bool value);
    /**@}*/

    /**@{
     *  Flag to indicate if the response will include unreadMessageCount and lastReadTimestamp.
     */
    bool getIncludeUnreadMessageInfo() const;
    void setIncludeUnreadMessageInfo(bool value);
    /**@}*/

    /**@{
     *  If this field is not present, all the channel message types will be included in the unread message count
     */
    ::eadp::foundation::Vector< com::ea::eadp::antelope::rtm::protocol::ChannelMessageType>& getUnreadMessageTypes();
    /**@}*/

    /**@{
     *  Flag to indicate if the response will include customProperties
     */
    bool getIncludeCustomProperties() const;
    void setIncludeCustomProperties(bool value);
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
    bool m_includeLastMessageInfo;
    bool m_includeUnreadMessageInfo;
    ::eadp::foundation::Vector< com::ea::eadp::antelope::rtm::protocol::ChannelMessageType> m_unreadMessageTypes;
    int32_t m_unreadMessageTypes__cachedByteSize;
    bool m_includeCustomProperties;

};

}
}
}
}
}
}
