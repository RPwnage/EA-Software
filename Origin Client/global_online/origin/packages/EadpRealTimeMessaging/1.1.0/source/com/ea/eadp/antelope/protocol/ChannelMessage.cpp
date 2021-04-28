// GENERATED CODE (Source: social_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/protocol/ChannelMessage.h>
#include <eadp/foundation/internal/ProtobufWireFormat.h>
#include <eadp/foundation/internal/Utils.h>
#include <eadp/foundation/Hub.h>

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

ChannelMessage::ChannelMessage(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.protocol.ChannelMessage"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_messageId(m_allocator_.emptyString())
, m_channelId(m_allocator_.emptyString())
, m_timestamp(m_allocator_.emptyString())
, m_type(1)
, m_contentCase(ContentCase::kNotSet)
{
}

void ChannelMessage::clear()
{
    m_cachedByteSize_ = 0;
    m_messageId.clear();
    m_channelId.clear();
    m_timestamp.clear();
    m_type = 1;
    clearContent();
}

size_t ChannelMessage::getByteSize()
{
    size_t total_size = 0;
    if (!m_messageId.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_messageId);
    }
    if (!m_channelId.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_channelId);
    }
    if (!m_timestamp.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_timestamp);
    }
    if (m_type != 1)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getEnumSize(m_type);
    }
    if (hasTextMessage() && m_content.m_textMessage)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_content.m_textMessage);
    }
    if (hasBinaryMessage() && m_content.m_binaryMessage)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_content.m_binaryMessage);
    }
    if (hasMembershipChange() && m_content.m_membershipChange)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_content.m_membershipChange);
    }
    if (hasChatConnected() && m_content.m_chatConnected)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_content.m_chatConnected);
    }
    if (hasChatDisconnected() && m_content.m_chatDisconnected)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_content.m_chatDisconnected);
    }
    if (hasChatLeft() && m_content.m_chatLeft)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_content.m_chatLeft);
    }
    if (hasCustomMessage() && m_content.m_customMessage)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_content.m_customMessage);
    }
    if (hasChatUserMuted() && m_content.m_chatUserMuted)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_content.m_chatUserMuted);
    }
    if (hasChatUserUnmuted() && m_content.m_chatUserUnmuted)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_content.m_chatUserUnmuted);
    }
    if (hasChannelMembershipChange() && m_content.m_channelMembershipChange)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_content.m_channelMembershipChange);
    }
    if (hasStickyMessageChanged() && m_content.m_stickyMessageChanged)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_content.m_stickyMessageChanged);
    }
    if (hasChatTypingEvent() && m_content.m_chatTypingEvent)
    {
        total_size += 2 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_content.m_chatTypingEvent);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* ChannelMessage::onSerialize(uint8_t* target) const
{
    if (!getMessageId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getMessageId(), target);
    }
    if (!getChannelId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(2, getChannelId(), target);
    }
    if (!getTimestamp().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(3, getTimestamp(), target);
    }
    if (m_type != 1)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeEnum(4, static_cast<int32_t>(getType()), target);
    }
    if (hasTextMessage() && m_content.m_textMessage)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(5, *m_content.m_textMessage, target);
    }
    if (hasBinaryMessage() && m_content.m_binaryMessage)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(6, *m_content.m_binaryMessage, target);
    }
    if (hasMembershipChange() && m_content.m_membershipChange)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(7, *m_content.m_membershipChange, target);
    }
    if (hasChatConnected() && m_content.m_chatConnected)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(8, *m_content.m_chatConnected, target);
    }
    if (hasChatDisconnected() && m_content.m_chatDisconnected)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(9, *m_content.m_chatDisconnected, target);
    }
    if (hasChatLeft() && m_content.m_chatLeft)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(10, *m_content.m_chatLeft, target);
    }
    if (hasCustomMessage() && m_content.m_customMessage)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(11, *m_content.m_customMessage, target);
    }
    if (hasChatUserMuted() && m_content.m_chatUserMuted)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(12, *m_content.m_chatUserMuted, target);
    }
    if (hasChatUserUnmuted() && m_content.m_chatUserUnmuted)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(13, *m_content.m_chatUserUnmuted, target);
    }
    if (hasChannelMembershipChange() && m_content.m_channelMembershipChange)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(14, *m_content.m_channelMembershipChange, target);
    }
    if (hasStickyMessageChanged() && m_content.m_stickyMessageChanged)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(15, *m_content.m_stickyMessageChanged, target);
    }
    if (hasChatTypingEvent() && m_content.m_chatTypingEvent)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(16, *m_content.m_chatTypingEvent, target);
    }
    return target;
}

bool ChannelMessage::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
{
    clear();
    uint32_t tag;
    for (;;)
    {
        tag = input->readTag();
        if (tag > 16383) goto handle_unusual;
        switch (::eadp::foundation::Internal::ProtobufWireFormat::getTagFieldNumber(tag))
        {
            case 1:
            {
                if (tag == 10)
                {
                    if (!input->readString(&m_messageId))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_channelId;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_channelId:
                    if (!input->readString(&m_channelId))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(26)) goto parse_timestamp;
                break;
            }

            case 3:
            {
                if (tag == 26)
                {
                    parse_timestamp:
                    if (!input->readString(&m_timestamp))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(32)) goto parse_type;
                break;
            }

            case 4:
            {
                if (tag == 32)
                {
                    parse_type:
                    int value;
                    if (!input->readEnum(&value))
                    {
                        return false;
                    }
                    setType(static_cast<com::ea::eadp::antelope::protocol::ChannelMessageType>(value));
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(42)) goto parse_textMessage;
                break;
            }

            case 5:
            {
                if (tag == 42)
                {
                    parse_textMessage:
                    clearContent();
                    if (!input->readMessage(getTextMessage().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(50)) goto parse_binaryMessage;
                break;
            }

            case 6:
            {
                if (tag == 50)
                {
                    parse_binaryMessage:
                    clearContent();
                    if (!input->readMessage(getBinaryMessage().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(58)) goto parse_membershipChange;
                break;
            }

            case 7:
            {
                if (tag == 58)
                {
                    parse_membershipChange:
                    clearContent();
                    if (!input->readMessage(getMembershipChange().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(66)) goto parse_chatConnected;
                break;
            }

            case 8:
            {
                if (tag == 66)
                {
                    parse_chatConnected:
                    clearContent();
                    if (!input->readMessage(getChatConnected().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(74)) goto parse_chatDisconnected;
                break;
            }

            case 9:
            {
                if (tag == 74)
                {
                    parse_chatDisconnected:
                    clearContent();
                    if (!input->readMessage(getChatDisconnected().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(82)) goto parse_chatLeft;
                break;
            }

            case 10:
            {
                if (tag == 82)
                {
                    parse_chatLeft:
                    clearContent();
                    if (!input->readMessage(getChatLeft().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(90)) goto parse_customMessage;
                break;
            }

            case 11:
            {
                if (tag == 90)
                {
                    parse_customMessage:
                    clearContent();
                    if (!input->readMessage(getCustomMessage().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(98)) goto parse_chatUserMuted;
                break;
            }

            case 12:
            {
                if (tag == 98)
                {
                    parse_chatUserMuted:
                    clearContent();
                    if (!input->readMessage(getChatUserMuted().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(106)) goto parse_chatUserUnmuted;
                break;
            }

            case 13:
            {
                if (tag == 106)
                {
                    parse_chatUserUnmuted:
                    clearContent();
                    if (!input->readMessage(getChatUserUnmuted().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(114)) goto parse_channelMembershipChange;
                break;
            }

            case 14:
            {
                if (tag == 114)
                {
                    parse_channelMembershipChange:
                    clearContent();
                    if (!input->readMessage(getChannelMembershipChange().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(122)) goto parse_stickyMessageChanged;
                break;
            }

            case 15:
            {
                if (tag == 122)
                {
                    parse_stickyMessageChanged:
                    clearContent();
                    if (!input->readMessage(getStickyMessageChanged().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(130)) goto parse_chatTypingEvent;
                break;
            }

            case 16:
            {
                if (tag == 130)
                {
                    parse_chatTypingEvent:
                    clearContent();
                    if (!input->readMessage(getChatTypingEvent().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectAtEnd()) return true;
                break;
            }

            default: {
            handle_unusual:
                if (tag == 0)
                {
                    return true;
                }
                if(!input->skipField(tag)) return false;
                break;
            }
        }
    }
}
::eadp::foundation::String ChannelMessage::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_messageId.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  messageId: \"%s\",\n", indent.c_str(), m_messageId.c_str());
    }
    if (!m_channelId.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  channelId: \"%s\",\n", indent.c_str(), m_channelId.c_str());
    }
    if (!m_timestamp.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  timestamp: \"%s\",\n", indent.c_str(), m_timestamp.c_str());
    }
    if (m_type != 1)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  type: %d,\n", indent.c_str(), m_type);
    }
    if (hasTextMessage() && m_content.m_textMessage)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  textMessage: %s,\n", indent.c_str(), m_content.m_textMessage->toString(indent + "  ").c_str());
    }
    if (hasBinaryMessage() && m_content.m_binaryMessage)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  binaryMessage: %s,\n", indent.c_str(), m_content.m_binaryMessage->toString(indent + "  ").c_str());
    }
    if (hasMembershipChange() && m_content.m_membershipChange)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  membershipChange: %s,\n", indent.c_str(), m_content.m_membershipChange->toString(indent + "  ").c_str());
    }
    if (hasChatConnected() && m_content.m_chatConnected)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  chatConnected: %s,\n", indent.c_str(), m_content.m_chatConnected->toString(indent + "  ").c_str());
    }
    if (hasChatDisconnected() && m_content.m_chatDisconnected)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  chatDisconnected: %s,\n", indent.c_str(), m_content.m_chatDisconnected->toString(indent + "  ").c_str());
    }
    if (hasChatLeft() && m_content.m_chatLeft)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  chatLeft: %s,\n", indent.c_str(), m_content.m_chatLeft->toString(indent + "  ").c_str());
    }
    if (hasCustomMessage() && m_content.m_customMessage)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  customMessage: %s,\n", indent.c_str(), m_content.m_customMessage->toString(indent + "  ").c_str());
    }
    if (hasChatUserMuted() && m_content.m_chatUserMuted)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  chatUserMuted: %s,\n", indent.c_str(), m_content.m_chatUserMuted->toString(indent + "  ").c_str());
    }
    if (hasChatUserUnmuted() && m_content.m_chatUserUnmuted)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  chatUserUnmuted: %s,\n", indent.c_str(), m_content.m_chatUserUnmuted->toString(indent + "  ").c_str());
    }
    if (hasChannelMembershipChange() && m_content.m_channelMembershipChange)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  channelMembershipChange: %s,\n", indent.c_str(), m_content.m_channelMembershipChange->toString(indent + "  ").c_str());
    }
    if (hasStickyMessageChanged() && m_content.m_stickyMessageChanged)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  stickyMessageChanged: %s,\n", indent.c_str(), m_content.m_stickyMessageChanged->toString(indent + "  ").c_str());
    }
    if (hasChatTypingEvent() && m_content.m_chatTypingEvent)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  chatTypingEvent: %s,\n", indent.c_str(), m_content.m_chatTypingEvent->toString(indent + "  ").c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& ChannelMessage::getMessageId() const
{
    return m_messageId;
}

void ChannelMessage::setMessageId(const ::eadp::foundation::String& value)
{
    m_messageId = value;
}

void ChannelMessage::setMessageId(const char8_t* value)
{
    m_messageId = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& ChannelMessage::getChannelId() const
{
    return m_channelId;
}

void ChannelMessage::setChannelId(const ::eadp::foundation::String& value)
{
    m_channelId = value;
}

void ChannelMessage::setChannelId(const char8_t* value)
{
    m_channelId = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& ChannelMessage::getTimestamp() const
{
    return m_timestamp;
}

void ChannelMessage::setTimestamp(const ::eadp::foundation::String& value)
{
    m_timestamp = value;
}

void ChannelMessage::setTimestamp(const char8_t* value)
{
    m_timestamp = m_allocator_.make<::eadp::foundation::String>(value);
}


com::ea::eadp::antelope::protocol::ChannelMessageType ChannelMessage::getType() const
{
    return static_cast<com::ea::eadp::antelope::protocol::ChannelMessageType>(m_type);
}
void ChannelMessage::setType(com::ea::eadp::antelope::protocol::ChannelMessageType value)
{
    m_type = static_cast<int>(value);
}

::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::TextMessage> ChannelMessage::getTextMessage()
{
    if (!hasTextMessage())
    {
        setTextMessage(m_allocator_.makeShared<com::ea::eadp::antelope::protocol::TextMessage>(m_allocator_));
    }
    return m_content.m_textMessage;
}

bool ChannelMessage::hasTextMessage() const
{
    return m_contentCase == ContentCase::kTextMessage;
}

void ChannelMessage::setTextMessage(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::TextMessage> value)
{
    clearContent();
    new(&m_content.m_textMessage) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::TextMessage>(value);
    m_contentCase = ContentCase::kTextMessage;
}

void ChannelMessage::releaseTextMessage()
{
    if (hasTextMessage())
    {
        m_content.m_textMessage.reset();
        m_contentCase = ContentCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::BinaryMessage> ChannelMessage::getBinaryMessage()
{
    if (!hasBinaryMessage())
    {
        setBinaryMessage(m_allocator_.makeShared<com::ea::eadp::antelope::protocol::BinaryMessage>(m_allocator_));
    }
    return m_content.m_binaryMessage;
}

bool ChannelMessage::hasBinaryMessage() const
{
    return m_contentCase == ContentCase::kBinaryMessage;
}

void ChannelMessage::setBinaryMessage(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::BinaryMessage> value)
{
    clearContent();
    new(&m_content.m_binaryMessage) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::BinaryMessage>(value);
    m_contentCase = ContentCase::kBinaryMessage;
}

void ChannelMessage::releaseBinaryMessage()
{
    if (hasBinaryMessage())
    {
        m_content.m_binaryMessage.reset();
        m_contentCase = ContentCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::GroupMembershipChange> ChannelMessage::getMembershipChange()
{
    if (!hasMembershipChange())
    {
        setMembershipChange(m_allocator_.makeShared<com::ea::eadp::antelope::protocol::GroupMembershipChange>(m_allocator_));
    }
    return m_content.m_membershipChange;
}

bool ChannelMessage::hasMembershipChange() const
{
    return m_contentCase == ContentCase::kMembershipChange;
}

void ChannelMessage::setMembershipChange(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::GroupMembershipChange> value)
{
    clearContent();
    new(&m_content.m_membershipChange) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::GroupMembershipChange>(value);
    m_contentCase = ContentCase::kMembershipChange;
}

void ChannelMessage::releaseMembershipChange()
{
    if (hasMembershipChange())
    {
        m_content.m_membershipChange.reset();
        m_contentCase = ContentCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatConnectedV1> ChannelMessage::getChatConnected()
{
    if (!hasChatConnected())
    {
        setChatConnected(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::ChatConnectedV1>(m_allocator_));
    }
    return m_content.m_chatConnected;
}

bool ChannelMessage::hasChatConnected() const
{
    return m_contentCase == ContentCase::kChatConnected;
}

void ChannelMessage::setChatConnected(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatConnectedV1> value)
{
    clearContent();
    new(&m_content.m_chatConnected) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatConnectedV1>(value);
    m_contentCase = ContentCase::kChatConnected;
}

void ChannelMessage::releaseChatConnected()
{
    if (hasChatConnected())
    {
        m_content.m_chatConnected.reset();
        m_contentCase = ContentCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatDisconnectedV1> ChannelMessage::getChatDisconnected()
{
    if (!hasChatDisconnected())
    {
        setChatDisconnected(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::ChatDisconnectedV1>(m_allocator_));
    }
    return m_content.m_chatDisconnected;
}

bool ChannelMessage::hasChatDisconnected() const
{
    return m_contentCase == ContentCase::kChatDisconnected;
}

void ChannelMessage::setChatDisconnected(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatDisconnectedV1> value)
{
    clearContent();
    new(&m_content.m_chatDisconnected) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatDisconnectedV1>(value);
    m_contentCase = ContentCase::kChatDisconnected;
}

void ChannelMessage::releaseChatDisconnected()
{
    if (hasChatDisconnected())
    {
        m_content.m_chatDisconnected.reset();
        m_contentCase = ContentCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatLeftV1> ChannelMessage::getChatLeft()
{
    if (!hasChatLeft())
    {
        setChatLeft(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::ChatLeftV1>(m_allocator_));
    }
    return m_content.m_chatLeft;
}

bool ChannelMessage::hasChatLeft() const
{
    return m_contentCase == ContentCase::kChatLeft;
}

void ChannelMessage::setChatLeft(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatLeftV1> value)
{
    clearContent();
    new(&m_content.m_chatLeft) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatLeftV1>(value);
    m_contentCase = ContentCase::kChatLeft;
}

void ChannelMessage::releaseChatLeft()
{
    if (hasChatLeft())
    {
        m_content.m_chatLeft.reset();
        m_contentCase = ContentCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::CustomMessage> ChannelMessage::getCustomMessage()
{
    if (!hasCustomMessage())
    {
        setCustomMessage(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::CustomMessage>(m_allocator_));
    }
    return m_content.m_customMessage;
}

bool ChannelMessage::hasCustomMessage() const
{
    return m_contentCase == ContentCase::kCustomMessage;
}

void ChannelMessage::setCustomMessage(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::CustomMessage> value)
{
    clearContent();
    new(&m_content.m_customMessage) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::CustomMessage>(value);
    m_contentCase = ContentCase::kCustomMessage;
}

void ChannelMessage::releaseCustomMessage()
{
    if (hasCustomMessage())
    {
        m_content.m_customMessage.reset();
        m_contentCase = ContentCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatUserMutedV1> ChannelMessage::getChatUserMuted()
{
    if (!hasChatUserMuted())
    {
        setChatUserMuted(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::ChatUserMutedV1>(m_allocator_));
    }
    return m_content.m_chatUserMuted;
}

bool ChannelMessage::hasChatUserMuted() const
{
    return m_contentCase == ContentCase::kChatUserMuted;
}

void ChannelMessage::setChatUserMuted(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatUserMutedV1> value)
{
    clearContent();
    new(&m_content.m_chatUserMuted) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatUserMutedV1>(value);
    m_contentCase = ContentCase::kChatUserMuted;
}

void ChannelMessage::releaseChatUserMuted()
{
    if (hasChatUserMuted())
    {
        m_content.m_chatUserMuted.reset();
        m_contentCase = ContentCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatUserUnmutedV1> ChannelMessage::getChatUserUnmuted()
{
    if (!hasChatUserUnmuted())
    {
        setChatUserUnmuted(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::ChatUserUnmutedV1>(m_allocator_));
    }
    return m_content.m_chatUserUnmuted;
}

bool ChannelMessage::hasChatUserUnmuted() const
{
    return m_contentCase == ContentCase::kChatUserUnmuted;
}

void ChannelMessage::setChatUserUnmuted(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatUserUnmutedV1> value)
{
    clearContent();
    new(&m_content.m_chatUserUnmuted) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatUserUnmutedV1>(value);
    m_contentCase = ContentCase::kChatUserUnmuted;
}

void ChannelMessage::releaseChatUserUnmuted()
{
    if (hasChatUserUnmuted())
    {
        m_content.m_chatUserUnmuted.reset();
        m_contentCase = ContentCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChannelMembershipChangeV1> ChannelMessage::getChannelMembershipChange()
{
    if (!hasChannelMembershipChange())
    {
        setChannelMembershipChange(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::ChannelMembershipChangeV1>(m_allocator_));
    }
    return m_content.m_channelMembershipChange;
}

bool ChannelMessage::hasChannelMembershipChange() const
{
    return m_contentCase == ContentCase::kChannelMembershipChange;
}

void ChannelMessage::setChannelMembershipChange(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChannelMembershipChangeV1> value)
{
    clearContent();
    new(&m_content.m_channelMembershipChange) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChannelMembershipChangeV1>(value);
    m_contentCase = ContentCase::kChannelMembershipChange;
}

void ChannelMessage::releaseChannelMembershipChange()
{
    if (hasChannelMembershipChange())
    {
        m_content.m_channelMembershipChange.reset();
        m_contentCase = ContentCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::StickyMessageChangedV1> ChannelMessage::getStickyMessageChanged()
{
    if (!hasStickyMessageChanged())
    {
        setStickyMessageChanged(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::StickyMessageChangedV1>(m_allocator_));
    }
    return m_content.m_stickyMessageChanged;
}

bool ChannelMessage::hasStickyMessageChanged() const
{
    return m_contentCase == ContentCase::kStickyMessageChanged;
}

void ChannelMessage::setStickyMessageChanged(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::StickyMessageChangedV1> value)
{
    clearContent();
    new(&m_content.m_stickyMessageChanged) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::StickyMessageChangedV1>(value);
    m_contentCase = ContentCase::kStickyMessageChanged;
}

void ChannelMessage::releaseStickyMessageChanged()
{
    if (hasStickyMessageChanged())
    {
        m_content.m_stickyMessageChanged.reset();
        m_contentCase = ContentCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatTypingEventV1> ChannelMessage::getChatTypingEvent()
{
    if (!hasChatTypingEvent())
    {
        setChatTypingEvent(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::ChatTypingEventV1>(m_allocator_));
    }
    return m_content.m_chatTypingEvent;
}

bool ChannelMessage::hasChatTypingEvent() const
{
    return m_contentCase == ContentCase::kChatTypingEvent;
}

void ChannelMessage::setChatTypingEvent(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatTypingEventV1> value)
{
    clearContent();
    new(&m_content.m_chatTypingEvent) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatTypingEventV1>(value);
    m_contentCase = ContentCase::kChatTypingEvent;
}

void ChannelMessage::releaseChatTypingEvent()
{
    if (hasChatTypingEvent())
    {
        m_content.m_chatTypingEvent.reset();
        m_contentCase = ContentCase::kNotSet;
    }
}


void ChannelMessage::clearContent()
{
    switch(getContentCase())
    {
        case ContentCase::kTextMessage:
            m_content.m_textMessage.reset();
            break;
        case ContentCase::kBinaryMessage:
            m_content.m_binaryMessage.reset();
            break;
        case ContentCase::kMembershipChange:
            m_content.m_membershipChange.reset();
            break;
        case ContentCase::kChatConnected:
            m_content.m_chatConnected.reset();
            break;
        case ContentCase::kChatDisconnected:
            m_content.m_chatDisconnected.reset();
            break;
        case ContentCase::kChatLeft:
            m_content.m_chatLeft.reset();
            break;
        case ContentCase::kCustomMessage:
            m_content.m_customMessage.reset();
            break;
        case ContentCase::kChatUserMuted:
            m_content.m_chatUserMuted.reset();
            break;
        case ContentCase::kChatUserUnmuted:
            m_content.m_chatUserUnmuted.reset();
            break;
        case ContentCase::kChannelMembershipChange:
            m_content.m_channelMembershipChange.reset();
            break;
        case ContentCase::kStickyMessageChanged:
            m_content.m_stickyMessageChanged.reset();
            break;
        case ContentCase::kChatTypingEvent:
            m_content.m_chatTypingEvent.reset();
            break;
        case ContentCase::kNotSet:
        default:
            break;
    }
    m_contentCase = ContentCase::kNotSet;
}


}
}
}
}
}
