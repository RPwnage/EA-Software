// GENERATED CODE (Source: social_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/protocol/MutedUser.h>
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

MutedUser::MutedUser(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.protocol.MutedUser"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_personaId(m_allocator_.emptyString())
, m_mutedBy(m_allocator_.make<::eadp::foundation::Vector< com::ea::eadp::antelope::protocol::MutedBy>>())
{
}

void MutedUser::clear()
{
    m_cachedByteSize_ = 0;
    m_personaId.clear();
    m_mutedBy.clear();
}

size_t MutedUser::getByteSize()
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
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* MutedUser::onSerialize(uint8_t* target) const
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
    return target;
}

bool MutedUser::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    m_mutedBy.push_back(static_cast<com::ea::eadp::antelope::protocol::MutedBy>(elem));
                } else if (tag == 18) {
                    if (!input->readPackedEnum<com::ea::eadp::antelope::protocol::MutedBy>(&m_mutedBy)){
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(16)) goto parse_mutedBy;
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
::eadp::foundation::String MutedUser::toString(const ::eadp::foundation::String& indent) const
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
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& MutedUser::getPersonaId() const
{
    return m_personaId;
}

void MutedUser::setPersonaId(const ::eadp::foundation::String& value)
{
    m_personaId = value;
}

void MutedUser::setPersonaId(const char8_t* value)
{
    m_personaId = m_allocator_.make<::eadp::foundation::String>(value);
}


::eadp::foundation::Vector< com::ea::eadp::antelope::protocol::MutedBy>& MutedUser::getMutedBy()
{
    return m_mutedBy;
}

}
}
}
}
}
