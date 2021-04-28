// GENERATED CODE (Source: common_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/ChatInitiateSuccessV1.h>
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

ChatInitiateSuccessV1::ChatInitiateSuccessV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.ChatInitiateSuccessV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_channel(nullptr)
{
}

void ChatInitiateSuccessV1::clear()
{
    m_cachedByteSize_ = 0;
    m_channel.reset();
}

size_t ChatInitiateSuccessV1::getByteSize()
{
    size_t total_size = 0;
    if (m_channel != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_channel);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* ChatInitiateSuccessV1::onSerialize(uint8_t* target) const
{
    if (m_channel != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(1, *m_channel, target);
    }
    return target;
}

bool ChatInitiateSuccessV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    if (!input->readMessage(getChannel().get())) return false;
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
::eadp::foundation::String ChatInitiateSuccessV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (m_channel != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  channel: %s,\n", indent.c_str(), m_channel->toString(indent + "  ").c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChannelV1> ChatInitiateSuccessV1::getChannel()
{
    if (m_channel == nullptr)
    {
        m_channel = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::ChannelV1>(m_allocator_);
    }
    return m_channel;
}

void ChatInitiateSuccessV1::setChannel(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChannelV1> val)
{
    m_channel = val;
}

bool ChatInitiateSuccessV1::hasChannel()
{
    return m_channel != nullptr;
}

void ChatInitiateSuccessV1::releaseChannel()
{
    m_channel.reset();
}


}
}
}
}
}
}
