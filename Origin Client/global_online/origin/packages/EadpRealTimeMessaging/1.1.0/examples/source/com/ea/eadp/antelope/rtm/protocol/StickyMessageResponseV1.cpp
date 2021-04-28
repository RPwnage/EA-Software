// GENERATED CODE (Source: chat_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/StickyMessageResponseV1.h>
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

StickyMessageResponseV1::StickyMessageResponseV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.StickyMessageResponseV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_channelId(m_allocator_.emptyString())
, m_message(m_allocator_.make<::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::StickyMessageV1>>>())
{
}

void StickyMessageResponseV1::clear()
{
    m_cachedByteSize_ = 0;
    m_channelId.clear();
    m_message.clear();
}

size_t StickyMessageResponseV1::getByteSize()
{
    size_t total_size = 0;
    if (!m_channelId.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_channelId);
    }
    total_size += 1 * m_message.size();
    for (EADPSDK_SIZE_T i = 0; i < m_message.size(); i++)
    {
        total_size += ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_message[i]);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* StickyMessageResponseV1::onSerialize(uint8_t* target) const
{
    if (!getChannelId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getChannelId(), target);
    }
    for (EADPSDK_SIZE_T i = 0; i < m_message.size(); i++)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(2, *m_message[i], target);
    }
    return target;
}

bool StickyMessageResponseV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    if (!input->readString(&m_channelId))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_message;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_message:
                    parse_loop_message:
                    auto tmpMessage = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::StickyMessageV1>(m_allocator_);
                    if (!input->readMessage(tmpMessage.get()))
                    {
                        return false;
                    }
                    m_message.push_back(tmpMessage);
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_loop_message;
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
::eadp::foundation::String StickyMessageResponseV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_channelId.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  channelId: \"%s\",\n", indent.c_str(), m_channelId.c_str());
    }
    if (m_message.size() > 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  message: [\n", indent.c_str());
        for (auto iter = m_message.begin(); iter != m_message.end(); iter++)
        {
            ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s    %s,\n", indent.c_str(), (*iter)->toString(indent + "  ").c_str());
        }
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  ],\n", indent.c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& StickyMessageResponseV1::getChannelId() const
{
    return m_channelId;
}

void StickyMessageResponseV1::setChannelId(const ::eadp::foundation::String& value)
{
    m_channelId = value;
}

void StickyMessageResponseV1::setChannelId(const char8_t* value)
{
    m_channelId = m_allocator_.make<::eadp::foundation::String>(value);
}


::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::StickyMessageV1>>& StickyMessageResponseV1::getMessage()
{
    return m_message;
}

}
}
}
}
}
}
