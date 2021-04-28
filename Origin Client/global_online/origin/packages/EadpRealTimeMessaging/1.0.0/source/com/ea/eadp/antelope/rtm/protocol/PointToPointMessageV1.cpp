// GENERATED CODE (Source: point_to_point.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/PointToPointMessageV1.h>
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

PointToPointMessageV1::PointToPointMessageV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.PointToPointMessageV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_from(nullptr)
, m_to(nullptr)
, m_messageContentCase(MessageContentCase::kNotSet)
{
}

void PointToPointMessageV1::clear()
{
    m_cachedByteSize_ = 0;
    m_from.reset();
    m_to.reset();
    clearMessageContent();
}

size_t PointToPointMessageV1::getByteSize()
{
    size_t total_size = 0;
    if (m_from != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_from);
    }
    if (m_to != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_to);
    }
    if (hasCustomMessage() && m_messageContent.m_customMessage)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_messageContent.m_customMessage);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* PointToPointMessageV1::onSerialize(uint8_t* target) const
{
    if (m_from != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(1, *m_from, target);
    }
    if (m_to != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(2, *m_to, target);
    }
    if (hasCustomMessage() && m_messageContent.m_customMessage)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(3, *m_messageContent.m_customMessage, target);
    }
    return target;
}

bool PointToPointMessageV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    if (!input->readMessage(getFrom().get())) return false;
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_to;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_to:
                    if (!input->readMessage(getTo().get())) return false;
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(26)) goto parse_customMessage;
                break;
            }

            case 3:
            {
                if (tag == 26)
                {
                    parse_customMessage:
                    clearMessageContent();
                    if (!input->readMessage(getCustomMessage().get()))
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
::eadp::foundation::String PointToPointMessageV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (m_from != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  from: %s,\n", indent.c_str(), m_from->toString(indent + "  ").c_str());
    }
    if (m_to != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  to: %s,\n", indent.c_str(), m_to->toString(indent + "  ").c_str());
    }
    if (hasCustomMessage() && m_messageContent.m_customMessage)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  customMessage: %s,\n", indent.c_str(), m_messageContent.m_customMessage->toString(indent + "  ").c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::AddressV1> PointToPointMessageV1::getFrom()
{
    if (m_from == nullptr)
    {
        m_from = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::AddressV1>(m_allocator_);
    }
    return m_from;
}

void PointToPointMessageV1::setFrom(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::AddressV1> val)
{
    m_from = val;
}

bool PointToPointMessageV1::hasFrom()
{
    return m_from != nullptr;
}

void PointToPointMessageV1::releaseFrom()
{
    m_from.reset();
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::AddressV1> PointToPointMessageV1::getTo()
{
    if (m_to == nullptr)
    {
        m_to = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::AddressV1>(m_allocator_);
    }
    return m_to;
}

void PointToPointMessageV1::setTo(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::AddressV1> val)
{
    m_to = val;
}

bool PointToPointMessageV1::hasTo()
{
    return m_to != nullptr;
}

void PointToPointMessageV1::releaseTo()
{
    m_to.reset();
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::CustomMessage> PointToPointMessageV1::getCustomMessage()
{
    if (!hasCustomMessage())
    {
        setCustomMessage(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::CustomMessage>(m_allocator_));
    }
    return m_messageContent.m_customMessage;
}

bool PointToPointMessageV1::hasCustomMessage() const
{
    return m_messageContentCase == MessageContentCase::kCustomMessage;
}

void PointToPointMessageV1::setCustomMessage(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::CustomMessage> value)
{
    clearMessageContent();
    new(&m_messageContent.m_customMessage) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::CustomMessage>(value);
    m_messageContentCase = MessageContentCase::kCustomMessage;
}

void PointToPointMessageV1::releaseCustomMessage()
{
    if (hasCustomMessage())
    {
        m_messageContent.m_customMessage.reset();
        m_messageContentCase = MessageContentCase::kNotSet;
    }
}


void PointToPointMessageV1::clearMessageContent()
{
    switch(getMessageContentCase())
    {
        case MessageContentCase::kCustomMessage:
            m_messageContent.m_customMessage.reset();
            break;
        case MessageContentCase::kNotSet:
        default:
            break;
    }
    m_messageContentCase = MessageContentCase::kNotSet;
}


}
}
}
}
}
}
