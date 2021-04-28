// GENERATED CODE (Source: moderation_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/GetRolesResponse.h>
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

GetRolesResponse::GetRolesResponse(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.GetRolesResponse"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_roles(m_allocator_.make<::eadp::foundation::Vector< com::ea::eadp::antelope::rtm::protocol::ModerationRoleType>>())
{
}

void GetRolesResponse::clear()
{
    m_cachedByteSize_ = 0;
    m_roles.clear();
}

size_t GetRolesResponse::getByteSize()
{
    size_t total_size = 0;
    {
        int dataSize = 0;
        for (EADPSDK_SIZE_T i = 0; i < m_roles.size(); i++)
        {
            dataSize += ::eadp::foundation::Internal::ProtobufWireFormat::getEnumSize(static_cast<int32_t>(m_roles[i]));
        }
        if (dataSize > 0)
        {
            total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getInt32Size(dataSize);
        }
        m_roles__cachedByteSize = dataSize;
        total_size += dataSize;
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* GetRolesResponse::onSerialize(uint8_t* target) const
{
    if (m_roles.size() > 0)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeTag(1, ::eadp::foundation::Internal::ProtobufWireFormat::WireType::LENGTH_DELIMITED, target);
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeInt32(m_roles__cachedByteSize, target);
    }
    for (EADPSDK_SIZE_T i = 0; i < m_roles.size(); i++)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeEnum(static_cast<int32_t>(m_roles[i]), target);
    }
    return target;
}

bool GetRolesResponse::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    parse_roles:
                    int elem;
                    if (!input->readEnum(&elem))
                    {
                        return false;
                    }
                    m_roles.push_back(static_cast<com::ea::eadp::antelope::rtm::protocol::ModerationRoleType>(elem));
                } else if (tag == 10) {
                    if (!input->readPackedEnum<com::ea::eadp::antelope::rtm::protocol::ModerationRoleType>(&m_roles)){
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(8)) goto parse_roles;
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
::eadp::foundation::String GetRolesResponse::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (m_roles.size() > 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  roles: [\n", indent.c_str());
        for (auto iter = m_roles.begin(); iter != m_roles.end(); iter++)
        {
            ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s    %d,\n", indent.c_str(), *iter);
        }
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  ],\n", indent.c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

::eadp::foundation::Vector< com::ea::eadp::antelope::rtm::protocol::ModerationRoleType>& GetRolesResponse::getRoles()
{
    return m_roles;
}

}
}
}
}
}
}
