// GENERATED CODE (Source: common_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/SessionResponseV1.h>
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

SessionResponseV1::SessionResponseV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.SessionResponseV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_connectedSessions(m_allocator_.make<::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionV1>>>())
{
}

void SessionResponseV1::clear()
{
    m_cachedByteSize_ = 0;
    m_connectedSessions.clear();
}

size_t SessionResponseV1::getByteSize()
{
    size_t total_size = 0;
    total_size += 1 * m_connectedSessions.size();
    for (EADPSDK_SIZE_T i = 0; i < m_connectedSessions.size(); i++)
    {
        total_size += ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_connectedSessions[i]);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* SessionResponseV1::onSerialize(uint8_t* target) const
{
    for (EADPSDK_SIZE_T i = 0; i < m_connectedSessions.size(); i++)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(1, *m_connectedSessions[i], target);
    }
    return target;
}

bool SessionResponseV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    parse_loop_connectedSessions:
                    auto tmpConnectedSessions = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::SessionV1>(m_allocator_);
                    if (!input->readMessage(tmpConnectedSessions.get()))
                    {
                        return false;
                    }
                    m_connectedSessions.push_back(tmpConnectedSessions);
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(10)) goto parse_loop_connectedSessions;
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
::eadp::foundation::String SessionResponseV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (m_connectedSessions.size() > 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  connectedSessions: [\n", indent.c_str());
        for (auto iter = m_connectedSessions.begin(); iter != m_connectedSessions.end(); iter++)
        {
            ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s    %s,\n", indent.c_str(), (*iter)->toString(indent + "  ").c_str());
        }
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  ],\n", indent.c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionV1>>& SessionResponseV1::getConnectedSessions()
{
    return m_connectedSessions;
}

}
}
}
}
}
}
