// GENERATED CODE (Source: chat_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/WorldChatChannelsRequestV1.h>
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

WorldChatChannelsRequestV1::WorldChatChannelsRequestV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.WorldChatChannelsRequestV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_worldName(m_allocator_.emptyString())
, m_fromIndex(0)
, m_pageSize(10)
{
}

void WorldChatChannelsRequestV1::clear()
{
    m_cachedByteSize_ = 0;
    m_worldName.clear();
    m_fromIndex = 0;
    m_pageSize = 10;
}

size_t WorldChatChannelsRequestV1::getByteSize()
{
    size_t total_size = 0;
    if (!m_worldName.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_worldName);
    }
    if (m_fromIndex != 0)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getInt32Size(m_fromIndex);
    }
    if (m_pageSize != 10)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getInt32Size(m_pageSize);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* WorldChatChannelsRequestV1::onSerialize(uint8_t* target) const
{
    if (!getWorldName().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getWorldName(), target);
    }
    if (getFromIndex() != 0)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeInt32(2, getFromIndex(), target);
    }
    if (getPageSize() != 10)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeInt32(3, getPageSize(), target);
    }
    return target;
}

bool WorldChatChannelsRequestV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    if (!input->readString(&m_worldName))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(16)) goto parse_fromIndex;
                break;
            }

            case 2:
            {
                if (tag == 16)
                {
                    parse_fromIndex:
                    if (!input->readInt32(&m_fromIndex))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(24)) goto parse_pageSize;
                break;
            }

            case 3:
            {
                if (tag == 24)
                {
                    parse_pageSize:
                    if (!input->readInt32(&m_pageSize))
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
::eadp::foundation::String WorldChatChannelsRequestV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_worldName.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  worldName: \"%s\",\n", indent.c_str(), m_worldName.c_str());
    }
    if (m_fromIndex != 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  fromIndex: %d,\n", indent.c_str(), m_fromIndex);
    }
    if (m_pageSize != 10)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  pageSize: %d,\n", indent.c_str(), m_pageSize);
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& WorldChatChannelsRequestV1::getWorldName() const
{
    return m_worldName;
}

void WorldChatChannelsRequestV1::setWorldName(const ::eadp::foundation::String& value)
{
    m_worldName = value;
}

void WorldChatChannelsRequestV1::setWorldName(const char8_t* value)
{
    m_worldName = m_allocator_.make<::eadp::foundation::String>(value);
}


int32_t WorldChatChannelsRequestV1::getFromIndex() const
{
    return m_fromIndex;
}

void WorldChatChannelsRequestV1::setFromIndex(int32_t value)
{
    m_fromIndex = value;
}


int32_t WorldChatChannelsRequestV1::getPageSize() const
{
    return m_pageSize;
}

void WorldChatChannelsRequestV1::setPageSize(int32_t value)
{
    m_pageSize = value;
}


}
}
}
}
}
}
