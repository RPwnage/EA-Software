// GENERATED CODE (Source: point_to_point.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/AddressV1.h>
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

AddressV1::AddressV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.AddressV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_domain(1)
, m_id(m_allocator_.emptyString())
, m_player(nullptr)
{
}

void AddressV1::clear()
{
    m_cachedByteSize_ = 0;
    m_domain = 1;
    m_id.clear();
    m_player.reset();
}

size_t AddressV1::getByteSize()
{
    size_t total_size = 0;
    if (m_domain != 1)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getEnumSize(m_domain);
    }
    if (!m_id.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_id);
    }
    if (m_player != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_player);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* AddressV1::onSerialize(uint8_t* target) const
{
    if (m_domain != 1)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeEnum(1, static_cast<int32_t>(getDomain()), target);
    }
    if (!getId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(2, getId(), target);
    }
    if (m_player != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(3, *m_player, target);
    }
    return target;
}

bool AddressV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    int value;
                    if (!input->readEnum(&value))
                    {
                        return false;
                    }
                    setDomain(static_cast<com::ea::eadp::antelope::rtm::protocol::Domain>(value));
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_id;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_id:
                    if (!input->readString(&m_id))
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
::eadp::foundation::String AddressV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (m_domain != 1)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  domain: %d,\n", indent.c_str(), m_domain);
    }
    if (!m_id.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  id: \"%s\",\n", indent.c_str(), m_id.c_str());
    }
    if (m_player != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  player: %s,\n", indent.c_str(), m_player->toString(indent + "  ").c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

com::ea::eadp::antelope::rtm::protocol::Domain AddressV1::getDomain() const
{
    return static_cast<com::ea::eadp::antelope::rtm::protocol::Domain>(m_domain);
}
void AddressV1::setDomain(com::ea::eadp::antelope::rtm::protocol::Domain value)
{
    m_domain = static_cast<int>(value);
}

const ::eadp::foundation::String& AddressV1::getId() const
{
    return m_id;
}

void AddressV1::setId(const ::eadp::foundation::String& value)
{
    m_id = value;
}

void AddressV1::setId(const char8_t* value)
{
    m_id = m_allocator_.make<::eadp::foundation::String>(value);
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Player> AddressV1::getPlayer()
{
    if (m_player == nullptr)
    {
        m_player = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::Player>(m_allocator_);
    }
    return m_player;
}

void AddressV1::setPlayer(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Player> val)
{
    m_player = val;
}

bool AddressV1::hasPlayer()
{
    return m_player != nullptr;
}

void AddressV1::releasePlayer()
{
    m_player.reset();
}


}
}
}
}
}
}
