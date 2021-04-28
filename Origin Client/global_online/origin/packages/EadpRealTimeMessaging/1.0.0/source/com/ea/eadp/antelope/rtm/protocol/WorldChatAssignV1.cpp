// GENERATED CODE (Source: chat_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/WorldChatAssignV1.h>
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

WorldChatAssignV1::WorldChatAssignV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.WorldChatAssignV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_worldName(m_allocator_.emptyString())
, m_shardIndex(-1)
{
}

void WorldChatAssignV1::clear()
{
    m_cachedByteSize_ = 0;
    m_worldName.clear();
    m_shardIndex = -1;
}

size_t WorldChatAssignV1::getByteSize()
{
    size_t total_size = 0;
    if (!m_worldName.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_worldName);
    }
    if (m_shardIndex != -1)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getInt32Size(m_shardIndex);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* WorldChatAssignV1::onSerialize(uint8_t* target) const
{
    if (!getWorldName().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getWorldName(), target);
    }
    if (getShardIndex() != -1)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeInt32(2, getShardIndex(), target);
    }
    return target;
}

bool WorldChatAssignV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                if (input->expectTag(16)) goto parse_shardIndex;
                break;
            }

            case 2:
            {
                if (tag == 16)
                {
                    parse_shardIndex:
                    if (!input->readInt32(&m_shardIndex))
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
::eadp::foundation::String WorldChatAssignV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_worldName.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  worldName: \"%s\",\n", indent.c_str(), m_worldName.c_str());
    }
    if (m_shardIndex != -1)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  shardIndex: %d,\n", indent.c_str(), m_shardIndex);
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& WorldChatAssignV1::getWorldName() const
{
    return m_worldName;
}

void WorldChatAssignV1::setWorldName(const ::eadp::foundation::String& value)
{
    m_worldName = value;
}

void WorldChatAssignV1::setWorldName(const char8_t* value)
{
    m_worldName = m_allocator_.make<::eadp::foundation::String>(value);
}


int32_t WorldChatAssignV1::getShardIndex() const
{
    return m_shardIndex;
}

void WorldChatAssignV1::setShardIndex(int32_t value)
{
    m_shardIndex = value;
}


}
}
}
}
}
}
