// GENERATED CODE (Source: chat_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/WorldChatResponseV1.h>
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

WorldChatResponseV1::WorldChatResponseV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.WorldChatResponseV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_channelId(m_allocator_.emptyString())
, m_remainingDailyShardSwitchCount(0)
, m_shardIndex(0)
, m_worldName(m_allocator_.emptyString())
, m_previousChannelId(m_allocator_.emptyString())
, m_previousShardIndex(0)
, m_muteList(m_allocator_.make<::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChannelMuteListV1>>>())
, m_channelName(m_allocator_.emptyString())
{
}

void WorldChatResponseV1::clear()
{
    m_cachedByteSize_ = 0;
    m_channelId.clear();
    m_remainingDailyShardSwitchCount = 0;
    m_shardIndex = 0;
    m_worldName.clear();
    m_previousChannelId.clear();
    m_previousShardIndex = 0;
    m_muteList.clear();
    m_channelName.clear();
}

size_t WorldChatResponseV1::getByteSize()
{
    size_t total_size = 0;
    if (!m_channelId.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_channelId);
    }
    if (m_remainingDailyShardSwitchCount != 0)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getInt32Size(m_remainingDailyShardSwitchCount);
    }
    if (m_shardIndex != 0)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getInt32Size(m_shardIndex);
    }
    if (!m_worldName.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_worldName);
    }
    if (!m_previousChannelId.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_previousChannelId);
    }
    if (m_previousShardIndex != 0)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getInt32Size(m_previousShardIndex);
    }
    total_size += 1 * m_muteList.size();
    for (EADPSDK_SIZE_T i = 0; i < m_muteList.size(); i++)
    {
        total_size += ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_muteList[i]);
    }
    if (!m_channelName.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_channelName);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* WorldChatResponseV1::onSerialize(uint8_t* target) const
{
    if (!getChannelId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getChannelId(), target);
    }
    if (getRemainingDailyShardSwitchCount() != 0)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeInt32(2, getRemainingDailyShardSwitchCount(), target);
    }
    if (getShardIndex() != 0)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeInt32(3, getShardIndex(), target);
    }
    if (!getWorldName().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(4, getWorldName(), target);
    }
    if (!getPreviousChannelId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(5, getPreviousChannelId(), target);
    }
    if (getPreviousShardIndex() != 0)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeInt32(6, getPreviousShardIndex(), target);
    }
    for (EADPSDK_SIZE_T i = 0; i < m_muteList.size(); i++)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(7, *m_muteList[i], target);
    }
    if (!getChannelName().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(8, getChannelName(), target);
    }
    return target;
}

bool WorldChatResponseV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                if (input->expectTag(16)) goto parse_remainingDailyShardSwitchCount;
                break;
            }

            case 2:
            {
                if (tag == 16)
                {
                    parse_remainingDailyShardSwitchCount:
                    if (!input->readInt32(&m_remainingDailyShardSwitchCount))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(24)) goto parse_shardIndex;
                break;
            }

            case 3:
            {
                if (tag == 24)
                {
                    parse_shardIndex:
                    if (!input->readInt32(&m_shardIndex))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(34)) goto parse_worldName;
                break;
            }

            case 4:
            {
                if (tag == 34)
                {
                    parse_worldName:
                    if (!input->readString(&m_worldName))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(42)) goto parse_previousChannelId;
                break;
            }

            case 5:
            {
                if (tag == 42)
                {
                    parse_previousChannelId:
                    if (!input->readString(&m_previousChannelId))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(48)) goto parse_previousShardIndex;
                break;
            }

            case 6:
            {
                if (tag == 48)
                {
                    parse_previousShardIndex:
                    if (!input->readInt32(&m_previousShardIndex))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(58)) goto parse_muteList;
                break;
            }

            case 7:
            {
                if (tag == 58)
                {
                    parse_muteList:
                    parse_loop_muteList:
                    auto tmpMuteList = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::ChannelMuteListV1>(m_allocator_);
                    if (!input->readMessage(tmpMuteList.get()))
                    {
                        return false;
                    }
                    m_muteList.push_back(tmpMuteList);
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(58)) goto parse_loop_muteList;
                if (input->expectTag(66)) goto parse_channelName;
                break;
            }

            case 8:
            {
                if (tag == 66)
                {
                    parse_channelName:
                    if (!input->readString(&m_channelName))
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
::eadp::foundation::String WorldChatResponseV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_channelId.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  channelId: \"%s\",\n", indent.c_str(), m_channelId.c_str());
    }
    if (m_remainingDailyShardSwitchCount != 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  remainingDailyShardSwitchCount: %d,\n", indent.c_str(), m_remainingDailyShardSwitchCount);
    }
    if (m_shardIndex != 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  shardIndex: %d,\n", indent.c_str(), m_shardIndex);
    }
    if (!m_worldName.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  worldName: \"%s\",\n", indent.c_str(), m_worldName.c_str());
    }
    if (!m_previousChannelId.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  previousChannelId: \"%s\",\n", indent.c_str(), m_previousChannelId.c_str());
    }
    if (m_previousShardIndex != 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  previousShardIndex: %d,\n", indent.c_str(), m_previousShardIndex);
    }
    if (m_muteList.size() > 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  muteList: [\n", indent.c_str());
        for (auto iter = m_muteList.begin(); iter != m_muteList.end(); iter++)
        {
            ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s    %s,\n", indent.c_str(), (*iter)->toString(indent + "  ").c_str());
        }
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  ],\n", indent.c_str());
    }
    if (!m_channelName.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  channelName: \"%s\",\n", indent.c_str(), m_channelName.c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& WorldChatResponseV1::getChannelId() const
{
    return m_channelId;
}

void WorldChatResponseV1::setChannelId(const ::eadp::foundation::String& value)
{
    m_channelId = value;
}

void WorldChatResponseV1::setChannelId(const char8_t* value)
{
    m_channelId = m_allocator_.make<::eadp::foundation::String>(value);
}


int32_t WorldChatResponseV1::getRemainingDailyShardSwitchCount() const
{
    return m_remainingDailyShardSwitchCount;
}

void WorldChatResponseV1::setRemainingDailyShardSwitchCount(int32_t value)
{
    m_remainingDailyShardSwitchCount = value;
}


int32_t WorldChatResponseV1::getShardIndex() const
{
    return m_shardIndex;
}

void WorldChatResponseV1::setShardIndex(int32_t value)
{
    m_shardIndex = value;
}


const ::eadp::foundation::String& WorldChatResponseV1::getWorldName() const
{
    return m_worldName;
}

void WorldChatResponseV1::setWorldName(const ::eadp::foundation::String& value)
{
    m_worldName = value;
}

void WorldChatResponseV1::setWorldName(const char8_t* value)
{
    m_worldName = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& WorldChatResponseV1::getPreviousChannelId() const
{
    return m_previousChannelId;
}

void WorldChatResponseV1::setPreviousChannelId(const ::eadp::foundation::String& value)
{
    m_previousChannelId = value;
}

void WorldChatResponseV1::setPreviousChannelId(const char8_t* value)
{
    m_previousChannelId = m_allocator_.make<::eadp::foundation::String>(value);
}


int32_t WorldChatResponseV1::getPreviousShardIndex() const
{
    return m_previousShardIndex;
}

void WorldChatResponseV1::setPreviousShardIndex(int32_t value)
{
    m_previousShardIndex = value;
}


::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChannelMuteListV1>>& WorldChatResponseV1::getMuteList()
{
    return m_muteList;
}

const ::eadp::foundation::String& WorldChatResponseV1::getChannelName() const
{
    return m_channelName;
}

void WorldChatResponseV1::setChannelName(const ::eadp::foundation::String& value)
{
    m_channelName = value;
}

void WorldChatResponseV1::setChannelName(const char8_t* value)
{
    m_channelName = m_allocator_.make<::eadp::foundation::String>(value);
}


}
}
}
}
}
}
