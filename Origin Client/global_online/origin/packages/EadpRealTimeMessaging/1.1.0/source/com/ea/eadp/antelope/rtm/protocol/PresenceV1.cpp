// GENERATED CODE (Source: presence_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/PresenceV1.h>
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

PresenceV1::PresenceV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.PresenceV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_personaId(m_allocator_.emptyString())
, m_status(m_allocator_.emptyString())
, m_timestamp(m_allocator_.emptyString())
, m_player(nullptr)
, m_basicPresenceType(1)
, m_userDefinedPresence(m_allocator_.emptyString())
, m_richPresence(nullptr)
, m_sessionKey(m_allocator_.emptyString())
, m_clientVersion(m_allocator_.emptyString())
, m_platform(1)
{
}

void PresenceV1::clear()
{
    m_cachedByteSize_ = 0;
    m_personaId.clear();
    m_status.clear();
    m_timestamp.clear();
    m_player.reset();
    m_basicPresenceType = 1;
    m_userDefinedPresence.clear();
    m_richPresence.reset();
    m_sessionKey.clear();
    m_clientVersion.clear();
    m_platform = 1;
}

size_t PresenceV1::getByteSize()
{
    size_t total_size = 0;
    if (!m_personaId.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_personaId);
    }
    if (!m_status.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_status);
    }
    if (!m_timestamp.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_timestamp);
    }
    if (m_player != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_player);
    }
    if (m_basicPresenceType != 1)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getEnumSize(m_basicPresenceType);
    }
    if (!m_userDefinedPresence.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_userDefinedPresence);
    }
    if (m_richPresence != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_richPresence);
    }
    if (!m_sessionKey.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_sessionKey);
    }
    if (!m_clientVersion.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_clientVersion);
    }
    if (m_platform != 1)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getEnumSize(m_platform);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* PresenceV1::onSerialize(uint8_t* target) const
{
    if (!getPersonaId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getPersonaId(), target);
    }
    if (!getStatus().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(2, getStatus(), target);
    }
    if (!getTimestamp().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(3, getTimestamp(), target);
    }
    if (m_player != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(4, *m_player, target);
    }
    if (m_basicPresenceType != 1)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeEnum(5, static_cast<int32_t>(getBasicPresenceType()), target);
    }
    if (!getUserDefinedPresence().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(6, getUserDefinedPresence(), target);
    }
    if (m_richPresence != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(7, *m_richPresence, target);
    }
    if (!getSessionKey().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(8, getSessionKey(), target);
    }
    if (!getClientVersion().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(9, getClientVersion(), target);
    }
    if (m_platform != 1)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeEnum(10, static_cast<int32_t>(getPlatform()), target);
    }
    return target;
}

bool PresenceV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    if (!input->readString(&m_personaId))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_status;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_status:
                    if (!input->readString(&m_status))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(26)) goto parse_timestamp;
                break;
            }

            case 3:
            {
                if (tag == 26)
                {
                    parse_timestamp:
                    if (!input->readString(&m_timestamp))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(34)) goto parse_player;
                break;
            }

            case 4:
            {
                if (tag == 34)
                {
                    parse_player:
                    if (!input->readMessage(getPlayer().get())) return false;
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(40)) goto parse_basicPresenceType;
                break;
            }

            case 5:
            {
                if (tag == 40)
                {
                    parse_basicPresenceType:
                    int value;
                    if (!input->readEnum(&value))
                    {
                        return false;
                    }
                    setBasicPresenceType(static_cast<com::ea::eadp::antelope::rtm::protocol::BasicPresenceType>(value));
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(50)) goto parse_userDefinedPresence;
                break;
            }

            case 6:
            {
                if (tag == 50)
                {
                    parse_userDefinedPresence:
                    if (!input->readString(&m_userDefinedPresence))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(58)) goto parse_richPresence;
                break;
            }

            case 7:
            {
                if (tag == 58)
                {
                    parse_richPresence:
                    if (!input->readMessage(getRichPresence().get())) return false;
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(66)) goto parse_sessionKey;
                break;
            }

            case 8:
            {
                if (tag == 66)
                {
                    parse_sessionKey:
                    if (!input->readString(&m_sessionKey))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(74)) goto parse_clientVersion;
                break;
            }

            case 9:
            {
                if (tag == 74)
                {
                    parse_clientVersion:
                    if (!input->readString(&m_clientVersion))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(80)) goto parse_platform;
                break;
            }

            case 10:
            {
                if (tag == 80)
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
::eadp::foundation::String PresenceV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_personaId.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  personaId: \"%s\",\n", indent.c_str(), m_personaId.c_str());
    }
    if (!m_status.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  status: \"%s\",\n", indent.c_str(), m_status.c_str());
    }
    if (!m_timestamp.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  timestamp: \"%s\",\n", indent.c_str(), m_timestamp.c_str());
    }
    if (m_player != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  player: %s,\n", indent.c_str(), m_player->toString(indent + "  ").c_str());
    }
    if (m_basicPresenceType != 1)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  basicPresenceType: %d,\n", indent.c_str(), m_basicPresenceType);
    }
    if (!m_userDefinedPresence.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  userDefinedPresence: \"%s\",\n", indent.c_str(), m_userDefinedPresence.c_str());
    }
    if (m_richPresence != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  richPresence: %s,\n", indent.c_str(), m_richPresence->toString(indent + "  ").c_str());
    }
    if (!m_sessionKey.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  sessionKey: \"%s\",\n", indent.c_str(), m_sessionKey.c_str());
    }
    if (!m_clientVersion.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  clientVersion: \"%s\",\n", indent.c_str(), m_clientVersion.c_str());
    }
    if (m_platform != 1)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  platform: %d,\n", indent.c_str(), m_platform);
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& PresenceV1::getPersonaId() const
{
    return m_personaId;
}

void PresenceV1::setPersonaId(const ::eadp::foundation::String& value)
{
    m_personaId = value;
}

void PresenceV1::setPersonaId(const char8_t* value)
{
    m_personaId = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& PresenceV1::getStatus() const
{
    return m_status;
}

void PresenceV1::setStatus(const ::eadp::foundation::String& value)
{
    m_status = value;
}

void PresenceV1::setStatus(const char8_t* value)
{
    m_status = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& PresenceV1::getTimestamp() const
{
    return m_timestamp;
}

void PresenceV1::setTimestamp(const ::eadp::foundation::String& value)
{
    m_timestamp = value;
}

void PresenceV1::setTimestamp(const char8_t* value)
{
    m_timestamp = m_allocator_.make<::eadp::foundation::String>(value);
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Player> PresenceV1::getPlayer()
{
    if (m_player == nullptr)
    {
        m_player = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::Player>(m_allocator_);
    }
    return m_player;
}

void PresenceV1::setPlayer(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Player> val)
{
    m_player = val;
}

bool PresenceV1::hasPlayer()
{
    return m_player != nullptr;
}

void PresenceV1::releasePlayer()
{
    m_player.reset();
}


com::ea::eadp::antelope::rtm::protocol::BasicPresenceType PresenceV1::getBasicPresenceType() const
{
    return static_cast<com::ea::eadp::antelope::rtm::protocol::BasicPresenceType>(m_basicPresenceType);
}
void PresenceV1::setBasicPresenceType(com::ea::eadp::antelope::rtm::protocol::BasicPresenceType value)
{
    m_basicPresenceType = static_cast<int>(value);
}

const ::eadp::foundation::String& PresenceV1::getUserDefinedPresence() const
{
    return m_userDefinedPresence;
}

void PresenceV1::setUserDefinedPresence(const ::eadp::foundation::String& value)
{
    m_userDefinedPresence = value;
}

void PresenceV1::setUserDefinedPresence(const char8_t* value)
{
    m_userDefinedPresence = m_allocator_.make<::eadp::foundation::String>(value);
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::RichPresenceV1> PresenceV1::getRichPresence()
{
    if (m_richPresence == nullptr)
    {
        m_richPresence = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::RichPresenceV1>(m_allocator_);
    }
    return m_richPresence;
}

void PresenceV1::setRichPresence(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::RichPresenceV1> val)
{
    m_richPresence = val;
}

bool PresenceV1::hasRichPresence()
{
    return m_richPresence != nullptr;
}

void PresenceV1::releaseRichPresence()
{
    m_richPresence.reset();
}


const ::eadp::foundation::String& PresenceV1::getSessionKey() const
{
    return m_sessionKey;
}

void PresenceV1::setSessionKey(const ::eadp::foundation::String& value)
{
    m_sessionKey = value;
}

void PresenceV1::setSessionKey(const char8_t* value)
{
    m_sessionKey = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& PresenceV1::getClientVersion() const
{
    return m_clientVersion;
}

void PresenceV1::setClientVersion(const ::eadp::foundation::String& value)
{
    m_clientVersion = value;
}

void PresenceV1::setClientVersion(const char8_t* value)
{
    m_clientVersion = m_allocator_.make<::eadp::foundation::String>(value);
}


com::ea::eadp::antelope::rtm::protocol::PlatformV1 PresenceV1::getPlatform() const
{
    return static_cast<com::ea::eadp::antelope::rtm::protocol::PlatformV1>(m_platform);
}
void PresenceV1::setPlatform(com::ea::eadp::antelope::rtm::protocol::PlatformV1 value)
{
    m_platform = static_cast<int>(value);
}

}
}
}
}
}
}
