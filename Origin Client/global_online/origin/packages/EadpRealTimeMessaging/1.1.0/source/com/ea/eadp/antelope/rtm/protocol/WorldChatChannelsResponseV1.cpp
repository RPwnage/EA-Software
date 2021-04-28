// GENERATED CODE (Source: chat_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/WorldChatChannelsResponseV1.h>
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

WorldChatChannelsResponseV1::WorldChatChannelsResponseV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.WorldChatChannelsResponseV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_worldName(m_allocator_.emptyString())
, m_shard(m_allocator_.make<::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatShard>>>())
, m_totalShardCount(0)
{
}

void WorldChatChannelsResponseV1::clear()
{
    m_cachedByteSize_ = 0;
    m_worldName.clear();
    m_shard.clear();
    m_totalShardCount = 0;
}

size_t WorldChatChannelsResponseV1::getByteSize()
{
    size_t total_size = 0;
    if (!m_worldName.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_worldName);
    }
    total_size += 1 * m_shard.size();
    for (EADPSDK_SIZE_T i = 0; i < m_shard.size(); i++)
    {
        total_size += ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_shard[i]);
    }
    if (m_totalShardCount != 0)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getInt32Size(m_totalShardCount);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* WorldChatChannelsResponseV1::onSerialize(uint8_t* target) const
{
    if (!getWorldName().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getWorldName(), target);
    }
    for (EADPSDK_SIZE_T i = 0; i < m_shard.size(); i++)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(2, *m_shard[i], target);
    }
    if (getTotalShardCount() != 0)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeInt32(3, getTotalShardCount(), target);
    }
    return target;
}

bool WorldChatChannelsResponseV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                if (input->expectTag(18)) goto parse_shard;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_shard:
                    parse_loop_shard:
                    auto tmpShard = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::WorldChatShard>(m_allocator_);
                    if (!input->readMessage(tmpShard.get()))
                    {
                        return false;
                    }
                    m_shard.push_back(tmpShard);
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_loop_shard;
                if (input->expectTag(24)) goto parse_totalShardCount;
                break;
            }

            case 3:
            {
                if (tag == 24)
                {
                    parse_totalShardCount:
                    if (!input->readInt32(&m_totalShardCount))
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
::eadp::foundation::String WorldChatChannelsResponseV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_worldName.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  worldName: \"%s\",\n", indent.c_str(), m_worldName.c_str());
    }
    if (m_shard.size() > 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  shard: [\n", indent.c_str());
        for (auto iter = m_shard.begin(); iter != m_shard.end(); iter++)
        {
            ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s    %s,\n", indent.c_str(), (*iter)->toString(indent + "  ").c_str());
        }
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  ],\n", indent.c_str());
    }
    if (m_totalShardCount != 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  totalShardCount: %d,\n", indent.c_str(), m_totalShardCount);
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& WorldChatChannelsResponseV1::getWorldName() const
{
    return m_worldName;
}

void WorldChatChannelsResponseV1::setWorldName(const ::eadp::foundation::String& value)
{
    m_worldName = value;
}

void WorldChatChannelsResponseV1::setWorldName(const char8_t* value)
{
    m_worldName = m_allocator_.make<::eadp::foundation::String>(value);
}


::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::WorldChatShard>>& WorldChatChannelsResponseV1::getShard()
{
    return m_shard;
}

int32_t WorldChatChannelsResponseV1::getTotalShardCount() const
{
    return m_totalShardCount;
}

void WorldChatChannelsResponseV1::setTotalShardCount(int32_t value)
{
    m_totalShardCount = value;
}


}
}
}
}
}
}
