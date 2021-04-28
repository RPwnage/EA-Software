// GENERATED CODE (Source: chat_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/ChatTypingEventV1.h>
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

namespace rtm
{

namespace protocol
{

ChatTypingEventV1::ChatTypingEventV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.ChatTypingEventV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_event(1)
, m_customTypingEvent(m_allocator_.emptyString())
, m_fromPlayer(nullptr)
, m_fromDisplayName(m_allocator_.emptyString())
, m_fromNickName(m_allocator_.emptyString())
{
}

void ChatTypingEventV1::clear()
{
    m_cachedByteSize_ = 0;
    m_event = 1;
    m_customTypingEvent.clear();
    m_fromPlayer.reset();
    m_fromDisplayName.clear();
    m_fromNickName.clear();
}

size_t ChatTypingEventV1::getByteSize()
{
    size_t total_size = 0;
    if (m_event != 1)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getEnumSize(m_event);
    }
    if (!m_customTypingEvent.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_customTypingEvent);
    }
    if (m_fromPlayer != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_fromPlayer);
    }
    if (!m_fromDisplayName.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_fromDisplayName);
    }
    if (!m_fromNickName.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_fromNickName);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* ChatTypingEventV1::onSerialize(uint8_t* target) const
{
    if (m_event != 1)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeEnum(1, static_cast<int32_t>(getEvent()), target);
    }
    if (!getCustomTypingEvent().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(2, getCustomTypingEvent(), target);
    }
    if (m_fromPlayer != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(3, *m_fromPlayer, target);
    }
    if (!getFromDisplayName().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(4, getFromDisplayName(), target);
    }
    if (!getFromNickName().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(5, getFromNickName(), target);
    }
    return target;
}

bool ChatTypingEventV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
{
    clear();
    uint32_t tag;
    for (;;)
    {
        tag = input->readTag();
        if (tag > 127) goto handle_unusual;
        switch (::eadp::foundation::Internal::ProtobufWireFormat::getTagFieldNumber(tag))
        {
            case 1:
            {
                if (tag == 8)
                {
                    int value;
                    if (!input->readEnum(&value))
                    {
                        return false;
                    }
                    setEvent(static_cast<com::ea::eadp::antelope::rtm::protocol::TypingEventType>(value));
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_customTypingEvent;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_customTypingEvent:
                    if (!input->readString(&m_customTypingEvent))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(26)) goto parse_fromPlayer;
                break;
            }

            case 3:
            {
                if (tag == 26)
                {
                    parse_fromPlayer:
                    if (!input->readMessage(getFromPlayer().get())) return false;
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(34)) goto parse_fromDisplayName;
                break;
            }

            case 4:
            {
                if (tag == 34)
                {
                    parse_fromDisplayName:
                    if (!input->readString(&m_fromDisplayName))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(42)) goto parse_fromNickName;
                break;
            }

            case 5:
            {
                if (tag == 42)
                {
                    parse_fromNickName:
                    if (!input->readString(&m_fromNickName))
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
::eadp::foundation::String ChatTypingEventV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (m_event != 1)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  event: %d,\n", indent.c_str(), m_event);
    }
    if (!m_customTypingEvent.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  customTypingEvent: \"%s\",\n", indent.c_str(), m_customTypingEvent.c_str());
    }
    if (m_fromPlayer != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  fromPlayer: %s,\n", indent.c_str(), m_fromPlayer->toString(indent + "  ").c_str());
    }
    if (!m_fromDisplayName.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  fromDisplayName: \"%s\",\n", indent.c_str(), m_fromDisplayName.c_str());
    }
    if (!m_fromNickName.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  fromNickName: \"%s\",\n", indent.c_str(), m_fromNickName.c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

com::ea::eadp::antelope::rtm::protocol::TypingEventType ChatTypingEventV1::getEvent() const
{
    return static_cast<com::ea::eadp::antelope::rtm::protocol::TypingEventType>(m_event);
}
void ChatTypingEventV1::setEvent(com::ea::eadp::antelope::rtm::protocol::TypingEventType value)
{
    m_event = static_cast<int>(value);
}

const ::eadp::foundation::String& ChatTypingEventV1::getCustomTypingEvent() const
{
    return m_customTypingEvent;
}

void ChatTypingEventV1::setCustomTypingEvent(const ::eadp::foundation::String& value)
{
    m_customTypingEvent = value;
}

void ChatTypingEventV1::setCustomTypingEvent(const char8_t* value)
{
    m_customTypingEvent = m_allocator_.make<::eadp::foundation::String>(value);
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Player> ChatTypingEventV1::getFromPlayer()
{
    if (m_fromPlayer == nullptr)
    {
        m_fromPlayer = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::Player>(m_allocator_);
    }
    return m_fromPlayer;
}

void ChatTypingEventV1::setFromPlayer(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Player> val)
{
    m_fromPlayer = val;
}

bool ChatTypingEventV1::hasFromPlayer()
{
    return m_fromPlayer != nullptr;
}

void ChatTypingEventV1::releaseFromPlayer()
{
    m_fromPlayer.reset();
}


const ::eadp::foundation::String& ChatTypingEventV1::getFromDisplayName() const
{
    return m_fromDisplayName;
}

void ChatTypingEventV1::setFromDisplayName(const ::eadp::foundation::String& value)
{
    m_fromDisplayName = value;
}

void ChatTypingEventV1::setFromDisplayName(const char8_t* value)
{
    m_fromDisplayName = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& ChatTypingEventV1::getFromNickName() const
{
    return m_fromNickName;
}

void ChatTypingEventV1::setFromNickName(const ::eadp::foundation::String& value)
{
    m_fromNickName = value;
}

void ChatTypingEventV1::setFromNickName(const char8_t* value)
{
    m_fromNickName = m_allocator_.make<::eadp::foundation::String>(value);
}


}
}
}
}
}
}
