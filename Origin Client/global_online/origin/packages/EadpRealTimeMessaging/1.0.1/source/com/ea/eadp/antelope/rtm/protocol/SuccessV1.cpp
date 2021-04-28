// GENERATED CODE (Source: common_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/SuccessV1.h>
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

SuccessV1::SuccessV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.SuccessV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_bodyCase(BodyCase::kNotSet)
{
}

void SuccessV1::clear()
{
    m_cachedByteSize_ = 0;
    clearBody();
}

size_t SuccessV1::getByteSize()
{
    size_t total_size = 0;
    if (hasLoginV2Success() && m_body.m_loginV2Success)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_loginV2Success);
    }
    if (hasChatInitiateSuccess() && m_body.m_chatInitiateSuccess)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_chatInitiateSuccess);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* SuccessV1::onSerialize(uint8_t* target) const
{
    if (hasLoginV2Success() && m_body.m_loginV2Success)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(1, *m_body.m_loginV2Success, target);
    }
    if (hasChatInitiateSuccess() && m_body.m_chatInitiateSuccess)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(2, *m_body.m_chatInitiateSuccess, target);
    }
    return target;
}

bool SuccessV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    if (!input->readMessage(getLoginV2Success().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_chatInitiateSuccess;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_chatInitiateSuccess:
                    clearBody();
                    if (!input->readMessage(getChatInitiateSuccess().get()))
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
::eadp::foundation::String SuccessV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (hasLoginV2Success() && m_body.m_loginV2Success)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  loginV2Success: %s,\n", indent.c_str(), m_body.m_loginV2Success->toString(indent + "  ").c_str());
    }
    if (hasChatInitiateSuccess() && m_body.m_chatInitiateSuccess)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  chatInitiateSuccess: %s,\n", indent.c_str(), m_body.m_chatInitiateSuccess->toString(indent + "  ").c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginV2Success> SuccessV1::getLoginV2Success()
{
    if (!hasLoginV2Success())
    {
        setLoginV2Success(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::LoginV2Success>(m_allocator_));
    }
    return m_body.m_loginV2Success;
}

bool SuccessV1::hasLoginV2Success() const
{
    return m_bodyCase == BodyCase::kLoginV2Success;
}

void SuccessV1::setLoginV2Success(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginV2Success> value)
{
    clearBody();
    new(&m_body.m_loginV2Success) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginV2Success>(value);
    m_bodyCase = BodyCase::kLoginV2Success;
}

void SuccessV1::releaseLoginV2Success()
{
    if (hasLoginV2Success())
    {
        m_body.m_loginV2Success.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatInitiateSuccessV1> SuccessV1::getChatInitiateSuccess()
{
    if (!hasChatInitiateSuccess())
    {
        setChatInitiateSuccess(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::ChatInitiateSuccessV1>(m_allocator_));
    }
    return m_body.m_chatInitiateSuccess;
}

bool SuccessV1::hasChatInitiateSuccess() const
{
    return m_bodyCase == BodyCase::kChatInitiateSuccess;
}

void SuccessV1::setChatInitiateSuccess(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatInitiateSuccessV1> value)
{
    clearBody();
    new(&m_body.m_chatInitiateSuccess) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatInitiateSuccessV1>(value);
    m_bodyCase = BodyCase::kChatInitiateSuccess;
}

void SuccessV1::releaseChatInitiateSuccess()
{
    if (hasChatInitiateSuccess())
    {
        m_body.m_chatInitiateSuccess.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


void SuccessV1::clearBody()
{
    switch(getBodyCase())
    {
        case BodyCase::kLoginV2Success:
            m_body.m_loginV2Success.reset();
            break;
        case BodyCase::kChatInitiateSuccess:
            m_body.m_chatInitiateSuccess.reset();
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
