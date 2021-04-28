// GENERATED CODE (Source: social_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Allocator.h>
#include <eadp/foundation/Service.h>
#include <eadp/foundation/internal/ProtobufMessage.h>
#include <com/ea/eadp/antelope/protocol/BinaryMessage.h>
#include <com/ea/eadp/antelope/protocol/ChannelMessageType.h>
#include <com/ea/eadp/antelope/protocol/GroupMembershipChange.h>
#include <com/ea/eadp/antelope/protocol/TextMessage.h>
#include <com/ea/eadp/antelope/rtm/protocol/ChannelMembershipChangeV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/ChatConnectedV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/ChatDisconnectedV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/ChatLeftV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/ChatTypingEventV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/ChatUserMutedV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/ChatUserUnmutedV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/CustomMessage.h>
#include <com/ea/eadp/antelope/rtm/protocol/StickyMessageChangedV1.h>

namespace com { namespace ea { namespace eadp { namespace antelope { namespace protocol { class BinaryMessage;  } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace protocol { class GroupMembershipChange;  } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace protocol { class TextMessage;  } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class ChannelMembershipChangeV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class ChatConnectedV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class ChatDisconnectedV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class ChatLeftV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class ChatTypingEventV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class ChatUserMutedV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class ChatUserUnmutedV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class CustomMessage;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class StickyMessageChangedV1;  } } } } } }

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

class EADPGENERATED_API ChannelMessage : public ::eadp::foundation::Internal::ProtobufMessage
{
public:

    /*!
     * Constructs an empty message. Memory allocated by the message will use the provided allocator.
     */
    explicit ChannelMessage(const ::eadp::foundation::Allocator& allocator);

    /*!
     * Clear a message, so that all fields are reset to their default values.
     */
    void clear();

    ~ChannelMessage()
    {
        clearContent();
    }

    /**@{
     */
    const ::eadp::foundation::String& getMessageId() const;
    void setMessageId(const ::eadp::foundation::String& value);
    void setMessageId(const char8_t* value);
    /**@}*/

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
    com::ea::eadp::antelope::protocol::ChannelMessageType getType() const;
    void setType(com::ea::eadp::antelope::protocol::ChannelMessageType value);
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::TextMessage> getTextMessage();
    void setTextMessage(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::TextMessage>);
    bool hasTextMessage() const;
    void releaseTextMessage();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::BinaryMessage> getBinaryMessage();
    void setBinaryMessage(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::BinaryMessage>);
    bool hasBinaryMessage() const;
    void releaseBinaryMessage();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::GroupMembershipChange> getMembershipChange();
    void setMembershipChange(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::GroupMembershipChange>);
    bool hasMembershipChange() const;
    void releaseMembershipChange();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatConnectedV1> getChatConnected();
    void setChatConnected(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatConnectedV1>);
    bool hasChatConnected() const;
    void releaseChatConnected();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatDisconnectedV1> getChatDisconnected();
    void setChatDisconnected(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatDisconnectedV1>);
    bool hasChatDisconnected() const;
    void releaseChatDisconnected();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatLeftV1> getChatLeft();
    void setChatLeft(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatLeftV1>);
    bool hasChatLeft() const;
    void releaseChatLeft();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::CustomMessage> getCustomMessage();
    void setCustomMessage(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::CustomMessage>);
    bool hasCustomMessage() const;
    void releaseCustomMessage();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatUserMutedV1> getChatUserMuted();
    void setChatUserMuted(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatUserMutedV1>);
    bool hasChatUserMuted() const;
    void releaseChatUserMuted();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatUserUnmutedV1> getChatUserUnmuted();
    void setChatUserUnmuted(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatUserUnmutedV1>);
    bool hasChatUserUnmuted() const;
    void releaseChatUserUnmuted();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChannelMembershipChangeV1> getChannelMembershipChange();
    void setChannelMembershipChange(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChannelMembershipChangeV1>);
    bool hasChannelMembershipChange() const;
    void releaseChannelMembershipChange();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::StickyMessageChangedV1> getStickyMessageChanged();
    void setStickyMessageChanged(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::StickyMessageChangedV1>);
    bool hasStickyMessageChanged() const;
    void releaseStickyMessageChanged();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatTypingEventV1> getChatTypingEvent();
    void setChatTypingEvent(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatTypingEventV1>);
    bool hasChatTypingEvent() const;
    void releaseChatTypingEvent();
    /**@}*/

    /*!
     * Case values for the fields potentially set on the associated union.
     */
    enum class ContentCase
    {
        kNotSet = 0,
        kTextMessage = 5,
        kBinaryMessage = 6,
        kMembershipChange = 7,
        kChatConnected = 8,
        kChatDisconnected = 9,
        kChatLeft = 10,
        kCustomMessage = 11,
        kChatUserMuted = 12,
        kChatUserUnmuted = 13,
        kChannelMembershipChange = 14,
        kStickyMessageChanged = 15,
        kChatTypingEvent = 16,
    };

    /*!
     * Returns the case corresponding to the currently set union field.
     */
    ContentCase getContentCase() { return static_cast<ContentCase>(m_contentCase); }

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
    ::eadp::foundation::String m_messageId;
    ::eadp::foundation::String m_channelId;
    ::eadp::foundation::String m_timestamp;
    int m_type;

    ContentCase m_contentCase;

    union ContentUnion
    {
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::TextMessage> m_textMessage;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::BinaryMessage> m_binaryMessage;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::GroupMembershipChange> m_membershipChange;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatConnectedV1> m_chatConnected;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatDisconnectedV1> m_chatDisconnected;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatLeftV1> m_chatLeft;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::CustomMessage> m_customMessage;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatUserMutedV1> m_chatUserMuted;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatUserUnmutedV1> m_chatUserUnmuted;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChannelMembershipChangeV1> m_channelMembershipChange;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::StickyMessageChangedV1> m_stickyMessageChanged;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatTypingEventV1> m_chatTypingEvent;

        ContentUnion() {}
        ~ContentUnion() {}
    } m_content;

    void clearContent();

};

}
}
}
}
}
