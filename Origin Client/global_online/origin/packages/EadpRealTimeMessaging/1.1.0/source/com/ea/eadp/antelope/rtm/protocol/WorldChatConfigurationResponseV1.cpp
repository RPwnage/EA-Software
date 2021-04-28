// GENERATED CODE (Source: chat_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/WorldChatConfigurationResponseV1.h>
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

WorldChatConfigurationResponseV1::WorldChatConfigurationResponseV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.WorldChatConfigurationResponseV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_remainingDailyShardSwitchCount(0)
, m_maxShards(0)
, m_maxShardSize(0)
, m_maxWorldNamesPerGame(0)
, m_maxDailyShardSwitchCount(0)
{
}

void WorldChatConfigurationResponseV1::clear()
{
    m_cachedByteSize_ = 0;
    m_remainingDailyShardSwitchCount = 0;
    m_maxShards = 0;
    m_maxShardSize = 0;
    m_maxWorldNamesPerGame = 0;
    m_maxDailyShardSwitchCount = 0;
}

size_t WorldChatConfigurationResponseV1::getByteSize()
{
    size_t total_size = 0;
    if (m_remainingDailyShardSwitchCount != 0)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getInt32Size(m_remainingDailyShardSwitchCount);
    }
    if (m_maxShards != 0)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getInt32Size(m_maxShards);
    }
    if (m_maxShardSize != 0)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getInt32Size(m_maxShardSize);
    }
    if (m_maxWorldNamesPerGame != 0)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getInt32Size(m_maxWorldNamesPerGame);
    }
    if (m_maxDailyShardSwitchCount != 0)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getInt32Size(m_maxDailyShardSwitchCount);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* WorldChatConfigurationResponseV1::onSerialize(uint8_t* target) const
{
    if (getRemainingDailyShardSwitchCount() != 0)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeInt32(1, getRemainingDailyShardSwitchCount(), target);
    }
    if (getMaxShards() != 0)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeInt32(2, getMaxShards(), target);
    }
    if (getMaxShardSize() != 0)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeInt32(3, getMaxShardSize(), target);
    }
    if (getMaxWorldNamesPerGame() != 0)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeInt32(4, getMaxWorldNamesPerGame(), target);
    }
    if (getMaxDailyShardSwitchCount() != 0)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeInt32(5, getMaxDailyShardSwitchCount(), target);
    }
    return target;
}

bool WorldChatConfigurationResponseV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    if (!input->readInt32(&m_remainingDailyShardSwitchCount))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(16)) goto parse_maxShards;
                break;
            }

            case 2:
            {
                if (tag == 16)
                {
                    parse_maxShards:
                    if (!input->readInt32(&m_maxShards))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(24)) goto parse_maxShardSize;
                break;
            }

            case 3:
            {
                if (tag == 24)
                {
                    parse_maxShardSize:
                    if (!input->readInt32(&m_maxShardSize))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(32)) goto parse_maxWorldNamesPerGame;
                break;
            }

            case 4:
            {
                if (tag == 32)
                {
                    parse_maxWorldNamesPerGame:
                    if (!input->readInt32(&m_maxWorldNamesPerGame))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(40)) goto parse_maxDailyShardSwitchCount;
                break;
            }

            case 5:
            {
                if (tag == 40)
                {
                    parse_maxDailyShardSwitchCount:
                    if (!input->readInt32(&m_maxDailyShardSwitchCount))
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
::eadp::foundation::String WorldChatConfigurationResponseV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (m_remainingDailyShardSwitchCount != 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  remainingDailyShardSwitchCount: %d,\n", indent.c_str(), m_remainingDailyShardSwitchCount);
    }
    if (m_maxShards != 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  maxShards: %d,\n", indent.c_str(), m_maxShards);
    }
    if (m_maxShardSize != 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  maxShardSize: %d,\n", indent.c_str(), m_maxShardSize);
    }
    if (m_maxWorldNamesPerGame != 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  maxWorldNamesPerGame: %d,\n", indent.c_str(), m_maxWorldNamesPerGame);
    }
    if (m_maxDailyShardSwitchCount != 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  maxDailyShardSwitchCount: %d,\n", indent.c_str(), m_maxDailyShardSwitchCount);
    }
    result.append(indent);
    result.append("}");
    return result;
}

int32_t WorldChatConfigurationResponseV1::getRemainingDailyShardSwitchCount() const
{
    return m_remainingDailyShardSwitchCount;
}

void WorldChatConfigurationResponseV1::setRemainingDailyShardSwitchCount(int32_t value)
{
    m_remainingDailyShardSwitchCount = value;
}


int32_t WorldChatConfigurationResponseV1::getMaxShards() const
{
    return m_maxShards;
}

void WorldChatConfigurationResponseV1::setMaxShards(int32_t value)
{
    m_maxShards = value;
}


int32_t WorldChatConfigurationResponseV1::getMaxShardSize() const
{
    return m_maxShardSize;
}

void WorldChatConfigurationResponseV1::setMaxShardSize(int32_t value)
{
    m_maxShardSize = value;
}


int32_t WorldChatConfigurationResponseV1::getMaxWorldNamesPerGame() const
{
    return m_maxWorldNamesPerGame;
}

void WorldChatConfigurationResponseV1::setMaxWorldNamesPerGame(int32_t value)
{
    m_maxWorldNamesPerGame = value;
}


int32_t WorldChatConfigurationResponseV1::getMaxDailyShardSwitchCount() const
{
    return m_maxDailyShardSwitchCount;
}

void WorldChatConfigurationResponseV1::setMaxDailyShardSwitchCount(int32_t value)
{
    m_maxDailyShardSwitchCount = value;
}


}
}
}
}
}
}
