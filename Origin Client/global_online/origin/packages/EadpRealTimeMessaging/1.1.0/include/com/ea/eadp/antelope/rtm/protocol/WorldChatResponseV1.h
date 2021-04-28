// GENERATED CODE (Source: chat_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Allocator.h>
#include <eadp/foundation/Service.h>
#include <eadp/foundation/internal/ProtobufMessage.h>
#include <com/ea/eadp/antelope/rtm/protocol/ChannelMuteListV1.h>

namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class ChannelMuteListV1;  } } } } } }

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
 *  Response for WorldChatAssignV1 requests
 */
class EADPGENERATED_API WorldChatResponseV1 : public ::eadp::foundation::Internal::ProtobufMessage
{
public:

    /*!
     * Constructs an empty message. Memory allocated by the message will use the provided allocator.
     */
    explicit WorldChatResponseV1(const ::eadp::foundation::Allocator& allocator);

    /*!
     * Clear a message, so that all fields are reset to their default values.
     */
    void clear();

    /**@{
     *  Information about the current assignment
     */
    const ::eadp::foundation::String& getChannelId() const;
    void setChannelId(const ::eadp::foundation::String& value);
    void setChannelId(const char8_t* value);
    /**@}*/

    /**@{
     */
    int32_t getRemainingDailyShardSwitchCount() const;
    void setRemainingDailyShardSwitchCount(int32_t value);
    /**@}*/

    /**@{
     */
    int32_t getShardIndex() const;
    void setShardIndex(int32_t value);
    /**@}*/

    /**@{
     */
    const ::eadp::foundation::String& getWorldName() const;
    void setWorldName(const ::eadp::foundation::String& value);
    void setWorldName(const char8_t* value);
    /**@}*/

    /**@{
     *  Information about the previous assignment
     */
    const ::eadp::foundation::String& getPreviousChannelId() const;
    void setPreviousChannelId(const ::eadp::foundation::String& value);
    void setPreviousChannelId(const char8_t* value);
    /**@}*/

    /**@{
     */
    int32_t getPreviousShardIndex() const;
    void setPreviousShardIndex(int32_t value);
    /**@}*/

    /**@{
     *  Information about muted users within this world name.
     */
    ::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChannelMuteListV1>>& getMuteList();
    /**@}*/

    /**@{
     *  For consistency this sends out the channel name as well.
     */
    const ::eadp::foundation::String& getChannelName() const;
    void setChannelName(const ::eadp::foundation::String& value);
    void setChannelName(const char8_t* value);
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
    int32_t m_remainingDailyShardSwitchCount;
    int32_t m_shardIndex;
    ::eadp::foundation::String m_worldName;
    ::eadp::foundation::String m_previousChannelId;
    int32_t m_previousShardIndex;
    ::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChannelMuteListV1>> m_muteList;
    ::eadp::foundation::String m_channelName;

};

}
}
}
}
}
}
