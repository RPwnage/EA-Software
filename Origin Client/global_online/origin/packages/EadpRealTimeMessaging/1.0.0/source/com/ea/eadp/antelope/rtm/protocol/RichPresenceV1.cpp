// GENERATED CODE (Source: presence_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/RichPresenceV1.h>
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

RichPresenceV1::RichPresenceV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.RichPresenceV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_game(m_allocator_.emptyString())
, m_platform(1)
, m_gameModeType(m_allocator_.emptyString())
, m_gameMode(m_allocator_.emptyString())
, m_gameSessionData(m_allocator_.emptyString())
, m_richPresenceType(1)
, m_startTimestamp(m_allocator_.emptyString())
, m_endTimestamp(m_allocator_.emptyString())
, m_customRichPresenceData(m_allocator_.emptyString())
{
}

void RichPresenceV1::clear()
{
    m_cachedByteSize_ = 0;
    m_game.clear();
    m_platform = 1;
    m_gameModeType.clear();
    m_gameMode.clear();
    m_gameSessionData.clear();
    m_richPresenceType = 1;
    m_startTimestamp.clear();
    m_endTimestamp.clear();
    m_customRichPresenceData.clear();
}

size_t RichPresenceV1::getByteSize()
{
    size_t total_size = 0;
    if (!m_game.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_game);
    }
    if (m_platform != 1)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getEnumSize(m_platform);
    }
    if (!m_gameModeType.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_gameModeType);
    }
    if (!m_gameMode.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_gameMode);
    }
    if (!m_gameSessionData.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_gameSessionData);
    }
    if (m_richPresenceType != 1)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getEnumSize(m_richPresenceType);
    }
    if (!m_startTimestamp.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_startTimestamp);
    }
    if (!m_endTimestamp.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_endTimestamp);
    }
    if (!m_customRichPresenceData.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_customRichPresenceData);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* RichPresenceV1::onSerialize(uint8_t* target) const
{
    if (!getGame().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getGame(), target);
    }
    if (m_platform != 1)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeEnum(2, static_cast<int32_t>(getPlatform()), target);
    }
    if (!getGameModeType().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(3, getGameModeType(), target);
    }
    if (!getGameMode().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(4, getGameMode(), target);
    }
    if (!getGameSessionData().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(5, getGameSessionData(), target);
    }
    if (m_richPresenceType != 1)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeEnum(6, static_cast<int32_t>(getRichPresenceType()), target);
    }
    if (!getStartTimestamp().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(7, getStartTimestamp(), target);
    }
    if (!getEndTimestamp().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(8, getEndTimestamp(), target);
    }
    if (!getCustomRichPresenceData().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(9, getCustomRichPresenceData(), target);
    }
    return target;
}

bool RichPresenceV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    if (!input->readString(&m_game))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(16)) goto parse_platform;
                break;
            }

            case 2:
            {
                if (tag == 16)
                {
                    parse_platform:
                    int value;
                    if (!input->readEnum(&value))
                    {
                        return false;
                    }
                    setPlatform(static_cast<com::ea::eadp::antelope::rtm::protocol::PlatformV1>(value));
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(26)) goto parse_gameModeType;
                break;
            }

            case 3:
            {
                if (tag == 26)
                {
                    parse_gameModeType:
                    if (!input->readString(&m_gameModeType))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(34)) goto parse_gameMode;
                break;
            }

            case 4:
            {
                if (tag == 34)
                {
                    parse_gameMode:
                    if (!input->readString(&m_gameMode))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(42)) goto parse_gameSessionData;
                break;
            }

            case 5:
            {
                if (tag == 42)
                {
                    parse_gameSessionData:
                    if (!input->readString(&m_gameSessionData))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(48)) goto parse_richPresenceType;
                break;
            }

            case 6:
            {
                if (tag == 48)
                {
                    parse_richPresenceType:
                    int value;
                    if (!input->readEnum(&value))
                    {
                        return false;
                    }
                    setRichPresenceType(static_cast<com::ea::eadp::antelope::rtm::protocol::RichPresenceType>(value));
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(58)) goto parse_startTimestamp;
                break;
            }

            case 7:
            {
                if (tag == 58)
                {
                    parse_startTimestamp:
                    if (!input->readString(&m_startTimestamp))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(66)) goto parse_endTimestamp;
                break;
            }

            case 8:
            {
                if (tag == 66)
                {
                    parse_endTimestamp:
                    if (!input->readString(&m_endTimestamp))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(74)) goto parse_customRichPresenceData;
                break;
            }

            case 9:
            {
                if (tag == 74)
                {
                    parse_customRichPresenceData:
                    if (!input->readString(&m_customRichPresenceData))
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
::eadp::foundation::String RichPresenceV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_game.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  game: \"%s\",\n", indent.c_str(), m_game.c_str());
    }
    if (m_platform != 1)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  platform: %d,\n", indent.c_str(), m_platform);
    }
    if (!m_gameModeType.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  gameModeType: \"%s\",\n", indent.c_str(), m_gameModeType.c_str());
    }
    if (!m_gameMode.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  gameMode: \"%s\",\n", indent.c_str(), m_gameMode.c_str());
    }
    if (!m_gameSessionData.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  gameSessionData: \"%s\",\n", indent.c_str(), m_gameSessionData.c_str());
    }
    if (m_richPresenceType != 1)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  richPresenceType: %d,\n", indent.c_str(), m_richPresenceType);
    }
    if (!m_startTimestamp.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  startTimestamp: \"%s\",\n", indent.c_str(), m_startTimestamp.c_str());
    }
    if (!m_endTimestamp.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  endTimestamp: \"%s\",\n", indent.c_str(), m_endTimestamp.c_str());
    }
    if (!m_customRichPresenceData.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  customRichPresenceData: \"%s\",\n", indent.c_str(), m_customRichPresenceData.c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& RichPresenceV1::getGame() const
{
    return m_game;
}

void RichPresenceV1::setGame(const ::eadp::foundation::String& value)
{
    m_game = value;
}

void RichPresenceV1::setGame(const char8_t* value)
{
    m_game = m_allocator_.make<::eadp::foundation::String>(value);
}


com::ea::eadp::antelope::rtm::protocol::PlatformV1 RichPresenceV1::getPlatform() const
{
    return static_cast<com::ea::eadp::antelope::rtm::protocol::PlatformV1>(m_platform);
}
void RichPresenceV1::setPlatform(com::ea::eadp::antelope::rtm::protocol::PlatformV1 value)
{
    m_platform = static_cast<int>(value);
}

const ::eadp::foundation::String& RichPresenceV1::getGameModeType() const
{
    return m_gameModeType;
}

void RichPresenceV1::setGameModeType(const ::eadp::foundation::String& value)
{
    m_gameModeType = value;
}

void RichPresenceV1::setGameModeType(const char8_t* value)
{
    m_gameModeType = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& RichPresenceV1::getGameMode() const
{
    return m_gameMode;
}

void RichPresenceV1::setGameMode(const ::eadp::foundation::String& value)
{
    m_gameMode = value;
}

void RichPresenceV1::setGameMode(const char8_t* value)
{
    m_gameMode = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& RichPresenceV1::getGameSessionData() const
{
    return m_gameSessionData;
}

void RichPresenceV1::setGameSessionData(const ::eadp::foundation::String& value)
{
    m_gameSessionData = value;
}

void RichPresenceV1::setGameSessionData(const char8_t* value)
{
    m_gameSessionData = m_allocator_.make<::eadp::foundation::String>(value);
}


com::ea::eadp::antelope::rtm::protocol::RichPresenceType RichPresenceV1::getRichPresenceType() const
{
    return static_cast<com::ea::eadp::antelope::rtm::protocol::RichPresenceType>(m_richPresenceType);
}
void RichPresenceV1::setRichPresenceType(com::ea::eadp::antelope::rtm::protocol::RichPresenceType value)
{
    m_richPresenceType = static_cast<int>(value);
}

const ::eadp::foundation::String& RichPresenceV1::getStartTimestamp() const
{
    return m_startTimestamp;
}

void RichPresenceV1::setStartTimestamp(const ::eadp::foundation::String& value)
{
    m_startTimestamp = value;
}

void RichPresenceV1::setStartTimestamp(const char8_t* value)
{
    m_startTimestamp = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& RichPresenceV1::getEndTimestamp() const
{
    return m_endTimestamp;
}

void RichPresenceV1::setEndTimestamp(const ::eadp::foundation::String& value)
{
    m_endTimestamp = value;
}

void RichPresenceV1::setEndTimestamp(const char8_t* value)
{
    m_endTimestamp = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& RichPresenceV1::getCustomRichPresenceData() const
{
    return m_customRichPresenceData;
}

void RichPresenceV1::setCustomRichPresenceData(const ::eadp::foundation::String& value)
{
    m_customRichPresenceData = value;
}

void RichPresenceV1::setCustomRichPresenceData(const char8_t* value)
{
    m_customRichPresenceData = m_allocator_.make<::eadp::foundation::String>(value);
}


}
}
}
}
}
}
