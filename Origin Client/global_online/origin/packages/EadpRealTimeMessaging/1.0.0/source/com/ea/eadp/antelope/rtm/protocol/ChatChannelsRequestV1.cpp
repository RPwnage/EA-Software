// GENERATED CODE (Source: chat_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/ChatChannelsRequestV1.h>
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

ChatChannelsRequestV1::ChatChannelsRequestV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.ChatChannelsRequestV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_includeUnreadMessageCount(false)
{
}

void ChatChannelsRequestV1::clear()
{
    m_cachedByteSize_ = 0;
    m_includeUnreadMessageCount = false;
}

size_t ChatChannelsRequestV1::getByteSize()
{
    size_t total_size = 0;
    if (m_includeUnreadMessageCount != false)
    {
        total_size += 1 + 1;
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* ChatChannelsRequestV1::onSerialize(uint8_t* target) const
{
    if (getIncludeUnreadMessageCount() != false)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeBool(1, getIncludeUnreadMessageCount(), target);
    }
    return target;
}

bool ChatChannelsRequestV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    if (!input->readBool(&m_includeUnreadMessageCount))
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
::eadp::foundation::String ChatChannelsRequestV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (m_includeUnreadMessageCount != false)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  includeUnreadMessageCount: %d,\n", indent.c_str(), m_includeUnreadMessageCount);
    }
    result.append(indent);
    result.append("}");
    return result;
}

bool ChatChannelsRequestV1::getIncludeUnreadMessageCount() const
{
    return m_includeUnreadMessageCount;
}

void ChatChannelsRequestV1::setIncludeUnreadMessageCount(bool value)
{
    m_includeUnreadMessageCount = value;
}


}
}
}
}
}
}
