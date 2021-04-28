// GENERATED CODE (Source: social_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/protocol/MutedUserV1.h>
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

namespace protocol
{

MutedUserV1::MutedUserV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.protocol.MutedUserV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_personaId(m_allocator_.emptyString())
, m_mutedBy(m_allocator_.make<::eadp::foundation::Vector< com::ea::eadp::antelope::rtm::protocol::MutedByV1>>())
, m_player(nullptr)
{
}

void MutedUserV1::clear()
{
    m_cachedByteSize_ = 0;
    m_personaId.clear();
    m_mutedBy.clear();
    m_player.reset();
}

size_t MutedUserV1::getByteSize()
{
    size_t total_size = 0;
    if (!m_personaId.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_personaId);
    }
    {
        int dataSize = 0;
        for (EADPSDK_SIZE_T i = 0; i < m_mutedBy.size(); i++)
        {
            dataSize += ::eadp::foundation::Internal::ProtobufWireFormat::getEnumSize(static_cast<int32_t>(m_mutedBy[i]));
        }
        if (dataSize > 0)
        {
            total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getInt32Size(dataSize);
        }
        m_mutedBy__cachedByteSize = dataSize;
        total_size += dataSize;
    }
    if (m_player != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_player);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* MutedUserV1::onSerialize(uint8_t* target) const
{
    if (!getPersonaId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getPersonaId(), target);
    }
    if (m_mutedBy.size() > 0)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeTag(2, ::eadp::foundation::Internal::ProtobufWireFormat::WireType::LENGTH_DELIMITED, target);
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeInt32(m_mutedBy__cachedByteSize, target);
    }
    for (EADPSDK_SIZE_T i = 0; i < m_mutedBy.size(); i++)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeEnum(static_cast<int32_t>(m_mutedBy[i]), target);
    }
    if (m_player != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(3, *m_player, target);
    }
    return target;
}

bool MutedUserV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                if (input->expectTag(16)) goto parse_mutedBy;
                break;
            }

            case 2:
            {
                if (tag == 16)
                {
                    parse_mutedBy:
                    int elem;
                    if (!input->readEnum(&elem))
                    {
                        return false;
                    }
                    m_mutedBy.push_back(static_cast<com::ea::eadp::antelope::rtm::protocol::MutedByV1>(elem));
                } else if (tag == 18) {
                    if (!input->readPackedEnum<com::ea::eadp::antelope::rtm::protocol::MutedByV1>(&m_mutedBy)){
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(16)) goto parse_mutedBy;
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
::eadp::foundation::String MutedUserV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_personaId.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  personaId: \"%s\",\n", indent.c_str(), m_personaId.c_str());
    }
    if (m_mutedBy.size() > 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  mutedBy: [\n", indent.c_str());
        for (auto iter = m_mutedBy.begin(); iter != m_mutedBy.end(); iter++)
        {
            ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s    %d,\n", indent.c_str(), *iter);
        }
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  ],\n", indent.c_str());
    }
    if (m_player != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  player: %s,\n", indent.c_str(), m_player->toString(indent + "  ").c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& MutedUserV1::getPersonaId() const
{
    return m_personaId;
}

void MutedUserV1::setPersonaId(const ::eadp::foundation::String& value)
{
    m_personaId = value;
}

void MutedUserV1::setPersonaId(const char8_t* value)
{
    m_personaId = m_allocator_.make<::eadp::foundation::String>(value);
}


::eadp::foundation::Vector< com::ea::eadp::antelope::rtm::protocol::MutedByV1>& MutedUserV1::getMutedBy()
{
    return m_mutedBy;
}

::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::Player> MutedUserV1::getPlayer()
{
    if (m_player == nullptr)
    {
        m_player = m_allocator_.makeShared<com::ea::eadp::antelope::protocol::Player>(m_allocator_);
    }
    return m_player;
}

void MutedUserV1::setPlayer(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::Player> val)
{
    m_player = val;
}

bool MutedUserV1::hasPlayer()
{
    return m_player != nullptr;
}

void MutedUserV1::releasePlayer()
{
    m_player.reset();
}


}
}
}
}
}
