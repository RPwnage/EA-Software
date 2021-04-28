// GENERATED CODE (Source: chat_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/UnmuteUserV1.h>
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

UnmuteUserV1::UnmuteUserV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.UnmuteUserV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_channelId(m_allocator_.emptyString())
, m_personaId(m_allocator_.emptyString())
, m_player(nullptr)
{
}

void UnmuteUserV1::clear()
{
    m_cachedByteSize_ = 0;
    m_channelId.clear();
    m_personaId.clear();
    m_player.reset();
}

size_t UnmuteUserV1::getByteSize()
{
    size_t total_size = 0;
    if (!m_channelId.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_channelId);
    }
    if (!m_personaId.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_personaId);
    }
    if (m_player != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_player);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* UnmuteUserV1::onSerialize(uint8_t* target) const
{
    if (!getChannelId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getChannelId(), target);
    }
    if (!getPersonaId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(2, getPersonaId(), target);
    }
    if (m_player != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(3, *m_player, target);
    }
    return target;
}

bool UnmuteUserV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                if (input->expectTag(18)) goto parse_personaId;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_personaId:
                    if (!input->readString(&m_personaId))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(26)) goto parse_player;
                break;
            }

            case 3:
            {
                if (tag == 26)
                {
                    parse_player:
                    if (!input->readMessage(getPlayer().get())) return false;
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
::eadp::foundation::String UnmuteUserV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_channelId.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  channelId: \"%s\",\n", indent.c_str(), m_channelId.c_str());
    }
    if (!m_personaId.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  personaId: \"%s\",\n", indent.c_str(), m_personaId.c_str());
    }
    if (m_player != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  player: %s,\n", indent.c_str(), m_player->toString(indent + "  ").c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& UnmuteUserV1::getChannelId() const
{
    return m_channelId;
}

void UnmuteUserV1::setChannelId(const ::eadp::foundation::String& value)
{
    m_channelId = value;
}

void UnmuteUserV1::setChannelId(const char8_t* value)
{
    m_channelId = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& UnmuteUserV1::getPersonaId() const
{
    return m_personaId;
}

void UnmuteUserV1::setPersonaId(const ::eadp::foundation::String& value)
{
    m_personaId = value;
}

void UnmuteUserV1::setPersonaId(const char8_t* value)
{
    m_personaId = m_allocator_.make<::eadp::foundation::String>(value);
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Player> UnmuteUserV1::getPlayer()
{
    if (m_player == nullptr)
    {
        m_player = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::Player>(m_allocator_);
    }
    return m_player;
}

void UnmuteUserV1::setPlayer(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Player> val)
{
    m_player = val;
}

bool UnmuteUserV1::hasPlayer()
{
    return m_player != nullptr;
}

void UnmuteUserV1::releasePlayer()
{
    m_player.reset();
}


}
}
}
}
}
}
