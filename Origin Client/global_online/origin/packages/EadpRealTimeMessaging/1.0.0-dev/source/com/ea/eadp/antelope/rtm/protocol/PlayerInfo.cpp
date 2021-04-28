// GENERATED CODE (Source: player_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/PlayerInfo.h>
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

PlayerInfo::PlayerInfo(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.PlayerInfo"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_player(nullptr)
, m_displayName(m_allocator_.emptyString())
{
}

void PlayerInfo::clear()
{
    m_cachedByteSize_ = 0;
    m_player.reset();
    m_displayName.clear();
}

size_t PlayerInfo::getByteSize()
{
    size_t total_size = 0;
    if (m_player != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_player);
    }
    if (!m_displayName.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_displayName);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* PlayerInfo::onSerialize(uint8_t* target) const
{
    if (m_player != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(1, *m_player, target);
    }
    if (!getDisplayName().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(2, getDisplayName(), target);
    }
    return target;
}

bool PlayerInfo::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    if (!input->readMessage(getPlayer().get())) return false;
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_displayName;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_displayName:
                    if (!input->readString(&m_displayName))
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
::eadp::foundation::String PlayerInfo::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (m_player != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  player: %s,\n", indent.c_str(), m_player->toString(indent + "  ").c_str());
    }
    if (!m_displayName.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  displayName: \"%s\",\n", indent.c_str(), m_displayName.c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Player> PlayerInfo::getPlayer()
{
    if (m_player == nullptr)
    {
        m_player = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::Player>(m_allocator_);
    }
    return m_player;
}

void PlayerInfo::setPlayer(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Player> val)
{
    m_player = val;
}

bool PlayerInfo::hasPlayer()
{
    return m_player != nullptr;
}

void PlayerInfo::releasePlayer()
{
    m_player.reset();
}


const ::eadp::foundation::String& PlayerInfo::getDisplayName() const
{
    return m_displayName;
}

void PlayerInfo::setDisplayName(const ::eadp::foundation::String& value)
{
    m_displayName = value;
}

void PlayerInfo::setDisplayName(const char8_t* value)
{
    m_displayName = m_allocator_.make<::eadp::foundation::String>(value);
}


}
}
}
}
}
}
