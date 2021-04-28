// GENERATED CODE (Source: presence_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/PresenceUpdateV1.h>
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

PresenceUpdateV1::PresenceUpdateV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.PresenceUpdateV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_status(m_allocator_.emptyString())
, m_basicPresenceType(1)
, m_userDefinedPresence(m_allocator_.emptyString())
, m_richPresence(nullptr)
{
}

void PresenceUpdateV1::clear()
{
    m_cachedByteSize_ = 0;
    m_status.clear();
    m_basicPresenceType = 1;
    m_userDefinedPresence.clear();
    m_richPresence.reset();
}

size_t PresenceUpdateV1::getByteSize()
{
    size_t total_size = 0;
    if (!m_status.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_status);
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
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* PresenceUpdateV1::onSerialize(uint8_t* target) const
{
    if (!getStatus().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getStatus(), target);
    }
    if (m_basicPresenceType != 1)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeEnum(2, static_cast<int32_t>(getBasicPresenceType()), target);
    }
    if (!getUserDefinedPresence().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(3, getUserDefinedPresence(), target);
    }
    if (m_richPresence != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(4, *m_richPresence, target);
    }
    return target;
}

bool PresenceUpdateV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    if (!input->readString(&m_status))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(16)) goto parse_basicPresenceType;
                break;
            }

            case 2:
            {
                if (tag == 16)
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
                if (input->expectTag(26)) goto parse_userDefinedPresence;
                break;
            }

            case 3:
            {
                if (tag == 26)
                {
                    parse_userDefinedPresence:
                    if (!input->readString(&m_userDefinedPresence))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(34)) goto parse_richPresence;
                break;
            }

            case 4:
            {
                if (tag == 34)
                {
                    parse_richPresence:
                    if (!input->readMessage(getRichPresence().get())) return false;
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
::eadp::foundation::String PresenceUpdateV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_status.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  status: \"%s\",\n", indent.c_str(), m_status.c_str());
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
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& PresenceUpdateV1::getStatus() const
{
    return m_status;
}

void PresenceUpdateV1::setStatus(const ::eadp::foundation::String& value)
{
    m_status = value;
}

void PresenceUpdateV1::setStatus(const char8_t* value)
{
    m_status = m_allocator_.make<::eadp::foundation::String>(value);
}


com::ea::eadp::antelope::rtm::protocol::BasicPresenceType PresenceUpdateV1::getBasicPresenceType() const
{
    return static_cast<com::ea::eadp::antelope::rtm::protocol::BasicPresenceType>(m_basicPresenceType);
}
void PresenceUpdateV1::setBasicPresenceType(com::ea::eadp::antelope::rtm::protocol::BasicPresenceType value)
{
    m_basicPresenceType = static_cast<int>(value);
}

const ::eadp::foundation::String& PresenceUpdateV1::getUserDefinedPresence() const
{
    return m_userDefinedPresence;
}

void PresenceUpdateV1::setUserDefinedPresence(const ::eadp::foundation::String& value)
{
    m_userDefinedPresence = value;
}

void PresenceUpdateV1::setUserDefinedPresence(const char8_t* value)
{
    m_userDefinedPresence = m_allocator_.make<::eadp::foundation::String>(value);
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::RichPresenceV1> PresenceUpdateV1::getRichPresence()
{
    if (m_richPresence == nullptr)
    {
        m_richPresence = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::RichPresenceV1>(m_allocator_);
    }
    return m_richPresence;
}

void PresenceUpdateV1::setRichPresence(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::RichPresenceV1> val)
{
    m_richPresence = val;
}

bool PresenceUpdateV1::hasRichPresence()
{
    return m_richPresence != nullptr;
}

void PresenceUpdateV1::releaseRichPresence()
{
    m_richPresence.reset();
}


}
}
}
}
}
}
