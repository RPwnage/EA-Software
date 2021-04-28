// GENERATED CODE (Source: rtm_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/Communication.h>
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

Communication::Communication(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.Communication"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_bodyCase(BodyCase::kNotSet)
{
}

void Communication::clear()
{
    m_cachedByteSize_ = 0;
    clearBody();
}

size_t Communication::getByteSize()
{
    size_t total_size = 0;
    if (hasV1() && m_body.m_v1)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_v1);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* Communication::onSerialize(uint8_t* target) const
{
    if (hasV1() && m_body.m_v1)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(1, *m_body.m_v1, target);
    }
    return target;
}

bool Communication::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    clearBody();
                    if (!input->readMessage(getV1().get()))
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
::eadp::foundation::String Communication::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (hasV1() && m_body.m_v1)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  v1: %s,\n", indent.c_str(), m_body.m_v1->toString(indent + "  ").c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::CommunicationV1> Communication::getV1()
{
    if (!hasV1())
    {
        setV1(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::CommunicationV1>(m_allocator_));
    }
    return m_body.m_v1;
}

bool Communication::hasV1() const
{
    return m_bodyCase == BodyCase::kV1;
}

void Communication::setV1(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::CommunicationV1> value)
{
    clearBody();
    new(&m_body.m_v1) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::CommunicationV1>(value);
    m_bodyCase = BodyCase::kV1;
}

void Communication::releaseV1()
{
    if (hasV1())
    {
        m_body.m_v1.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


void Communication::clearBody()
{
    switch(getBodyCase())
    {
        case BodyCase::kV1:
            m_body.m_v1.reset();
            break;
        case BodyCase::kNotSet:
        default:
            break;
    }
    m_bodyCase = BodyCase::kNotSet;
}


}
}
}
}
}
}
