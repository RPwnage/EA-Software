// GENERATED CODE (Source: social_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/protocol/Player.h>
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

Player::Player(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.protocol.Player"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_playerId(m_allocator_.emptyString())
, m_productId(m_allocator_.emptyString())
{
}

void Player::clear()
{
    m_cachedByteSize_ = 0;
    m_playerId.clear();
    m_productId.clear();
}

size_t Player::getByteSize()
{
    size_t total_size = 0;
    if (!m_playerId.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_playerId);
    }
    if (!m_productId.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_productId);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* Player::onSerialize(uint8_t* target) const
{
    if (!getPlayerId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getPlayerId(), target);
    }
    if (!getProductId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(4, getProductId(), target);
    }
    return target;
}

bool Player::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    if (!input->readString(&m_playerId))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(34)) goto parse_productId;
                break;
            }

            case 4:
            {
                if (tag == 34)
                {
                    parse_productId:
                    if (!input->readString(&m_productId))
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
::eadp::foundation::String Player::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_playerId.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  playerId: \"%s\",\n", indent.c_str(), m_playerId.c_str());
    }
    if (!m_productId.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  productId: \"%s\",\n", indent.c_str(), m_productId.c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& Player::getPlayerId() const
{
    return m_playerId;
}

void Player::setPlayerId(const ::eadp::foundation::String& value)
{
    m_playerId = value;
}

void Player::setPlayerId(const char8_t* value)
{
    m_playerId = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& Player::getProductId() const
{
    return m_productId;
}

void Player::setProductId(const ::eadp::foundation::String& value)
{
    m_productId = value;
}

void Player::setProductId(const char8_t* value)
{
    m_productId = m_allocator_.make<::eadp::foundation::String>(value);
}


}
}
}
}
}
