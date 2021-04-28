// GENERATED CODE (Source: common_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/PersonaV1.h>
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

PersonaV1::PersonaV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.PersonaV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_personaId(m_allocator_.emptyString())
, m_displayName(m_allocator_.emptyString())
, m_nickName(m_allocator_.emptyString())
{
}

void PersonaV1::clear()
{
    m_cachedByteSize_ = 0;
    m_personaId.clear();
    m_displayName.clear();
    m_nickName.clear();
}

size_t PersonaV1::getByteSize()
{
    size_t total_size = 0;
    if (!m_personaId.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_personaId);
    }
    if (!m_displayName.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_displayName);
    }
    if (!m_nickName.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_nickName);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* PersonaV1::onSerialize(uint8_t* target) const
{
    if (!getPersonaId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getPersonaId(), target);
    }
    if (!getDisplayName().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(2, getDisplayName(), target);
    }
    if (!getNickName().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(3, getNickName(), target);
    }
    return target;
}

bool PersonaV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                if (input->expectTag(26)) goto parse_nickName;
                break;
            }

            case 3:
            {
                if (tag == 26)
                {
                    parse_nickName:
                    if (!input->readString(&m_nickName))
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
::eadp::foundation::String PersonaV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_personaId.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  personaId: \"%s\",\n", indent.c_str(), m_personaId.c_str());
    }
    if (!m_displayName.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  displayName: \"%s\",\n", indent.c_str(), m_displayName.c_str());
    }
    if (!m_nickName.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  nickName: \"%s\",\n", indent.c_str(), m_nickName.c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& PersonaV1::getPersonaId() const
{
    return m_personaId;
}

void PersonaV1::setPersonaId(const ::eadp::foundation::String& value)
{
    m_personaId = value;
}

void PersonaV1::setPersonaId(const char8_t* value)
{
    m_personaId = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& PersonaV1::getDisplayName() const
{
    return m_displayName;
}

void PersonaV1::setDisplayName(const ::eadp::foundation::String& value)
{
    m_displayName = value;
}

void PersonaV1::setDisplayName(const char8_t* value)
{
    m_displayName = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& PersonaV1::getNickName() const
{
    return m_nickName;
}

void PersonaV1::setNickName(const ::eadp::foundation::String& value)
{
    m_nickName = value;
}

void PersonaV1::setNickName(const char8_t* value)
{
    m_nickName = m_allocator_.make<::eadp::foundation::String>(value);
}


}
}
}
}
}
}
