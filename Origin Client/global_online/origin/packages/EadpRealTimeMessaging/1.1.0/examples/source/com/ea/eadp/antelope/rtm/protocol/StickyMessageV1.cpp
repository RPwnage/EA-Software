// GENERATED CODE (Source: common_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/StickyMessageV1.h>
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

StickyMessageV1::StickyMessageV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.StickyMessageV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_stickyAuthorPersonaId(m_allocator_.emptyString())
, m_stickyAuthorDisplayName(m_allocator_.emptyString())
, m_timestamp(m_allocator_.emptyString())
, m_textMessage(nullptr)
, m_messageId(m_allocator_.emptyString())
, m_stickyAuthorNickName(m_allocator_.emptyString())
{
}

void StickyMessageV1::clear()
{
    m_cachedByteSize_ = 0;
    m_stickyAuthorPersonaId.clear();
    m_stickyAuthorDisplayName.clear();
    m_timestamp.clear();
    m_textMessage.reset();
    m_messageId.clear();
    m_stickyAuthorNickName.clear();
}

size_t StickyMessageV1::getByteSize()
{
    size_t total_size = 0;
    if (!m_stickyAuthorPersonaId.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_stickyAuthorPersonaId);
    }
    if (!m_stickyAuthorDisplayName.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_stickyAuthorDisplayName);
    }
    if (!m_timestamp.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_timestamp);
    }
    if (m_textMessage != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_textMessage);
    }
    if (!m_messageId.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_messageId);
    }
    if (!m_stickyAuthorNickName.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_stickyAuthorNickName);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* StickyMessageV1::onSerialize(uint8_t* target) const
{
    if (!getStickyAuthorPersonaId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getStickyAuthorPersonaId(), target);
    }
    if (!getStickyAuthorDisplayName().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(2, getStickyAuthorDisplayName(), target);
    }
    if (!getTimestamp().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(3, getTimestamp(), target);
    }
    if (m_textMessage != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(4, *m_textMessage, target);
    }
    if (!getMessageId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(5, getMessageId(), target);
    }
    if (!getStickyAuthorNickName().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(6, getStickyAuthorNickName(), target);
    }
    return target;
}

bool StickyMessageV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                if (tag == 10)
                {
                    if (!input->readString(&m_stickyAuthorPersonaId))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_stickyAuthorDisplayName;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_stickyAuthorDisplayName:
                    if (!input->readString(&m_stickyAuthorDisplayName))
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
                if (input->expectTag(34)) goto parse_textMessage;
                break;
            }

            case 4:
            {
                if (tag == 34)
                {
                    parse_textMessage:
                    if (!input->readMessage(getTextMessage().get())) return false;
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(42)) goto parse_messageId;
                break;
            }

            case 5:
            {
                if (tag == 42)
                {
                    parse_messageId:
                    if (!input->readString(&m_messageId))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(50)) goto parse_stickyAuthorNickName;
                break;
            }

            case 6:
            {
                if (tag == 50)
                {
                    parse_stickyAuthorNickName:
                    if (!input->readString(&m_stickyAuthorNickName))
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
::eadp::foundation::String StickyMessageV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_stickyAuthorPersonaId.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  stickyAuthorPersonaId: \"%s\",\n", indent.c_str(), m_stickyAuthorPersonaId.c_str());
    }
    if (!m_stickyAuthorDisplayName.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  stickyAuthorDisplayName: \"%s\",\n", indent.c_str(), m_stickyAuthorDisplayName.c_str());
    }
    if (!m_timestamp.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  timestamp: \"%s\",\n", indent.c_str(), m_timestamp.c_str());
    }
    if (m_textMessage != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  textMessage: %s,\n", indent.c_str(), m_textMessage->toString(indent + "  ").c_str());
    }
    if (!m_messageId.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  messageId: \"%s\",\n", indent.c_str(), m_messageId.c_str());
    }
    if (!m_stickyAuthorNickName.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  stickyAuthorNickName: \"%s\",\n", indent.c_str(), m_stickyAuthorNickName.c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& StickyMessageV1::getStickyAuthorPersonaId() const
{
    return m_stickyAuthorPersonaId;
}

void StickyMessageV1::setStickyAuthorPersonaId(const ::eadp::foundation::String& value)
{
    m_stickyAuthorPersonaId = value;
}

void StickyMessageV1::setStickyAuthorPersonaId(const char8_t* value)
{
    m_stickyAuthorPersonaId = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& StickyMessageV1::getStickyAuthorDisplayName() const
{
    return m_stickyAuthorDisplayName;
}

void StickyMessageV1::setStickyAuthorDisplayName(const ::eadp::foundation::String& value)
{
    m_stickyAuthorDisplayName = value;
}

void StickyMessageV1::setStickyAuthorDisplayName(const char8_t* value)
{
    m_stickyAuthorDisplayName = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& StickyMessageV1::getTimestamp() const
{
    return m_timestamp;
}

void StickyMessageV1::setTimestamp(const ::eadp::foundation::String& value)
{
    m_timestamp = value;
}

void StickyMessageV1::setTimestamp(const char8_t* value)
{
    m_timestamp = m_allocator_.make<::eadp::foundation::String>(value);
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::TextMessageV1> StickyMessageV1::getTextMessage()
{
    if (m_textMessage == nullptr)
    {
        m_textMessage = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::TextMessageV1>(m_allocator_);
    }
    return m_textMessage;
}

void StickyMessageV1::setTextMessage(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::TextMessageV1> val)
{
    m_textMessage = val;
}

bool StickyMessageV1::hasTextMessage()
{
    return m_textMessage != nullptr;
}

void StickyMessageV1::releaseTextMessage()
{
    m_textMessage.reset();
}


const ::eadp::foundation::String& StickyMessageV1::getMessageId() const
{
    return m_messageId;
}

void StickyMessageV1::setMessageId(const ::eadp::foundation::String& value)
{
    m_messageId = value;
}

void StickyMessageV1::setMessageId(const char8_t* value)
{
    m_messageId = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& StickyMessageV1::getStickyAuthorNickName() const
{
    return m_stickyAuthorNickName;
}

void StickyMessageV1::setStickyAuthorNickName(const ::eadp::foundation::String& value)
{
    m_stickyAuthorNickName = value;
}

void StickyMessageV1::setStickyAuthorNickName(const char8_t* value)
{
    m_stickyAuthorNickName = m_allocator_.make<::eadp::foundation::String>(value);
}


}
}
}
}
}
}
