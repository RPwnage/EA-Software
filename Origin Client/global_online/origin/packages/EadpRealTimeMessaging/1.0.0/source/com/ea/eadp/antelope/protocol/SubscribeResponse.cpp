// GENERATED CODE (Source: social_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/protocol/SubscribeResponse.h>
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

SubscribeResponse::SubscribeResponse(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.protocol.SubscribeResponse"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_channelId(m_allocator_.emptyString())
, m_success(false)
, m_errorCode(m_allocator_.emptyString())
, m_errorMessage(m_allocator_.emptyString())
, m_mutedUsers(m_allocator_.make<::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::MutedUserV1>>>())
{
}

void SubscribeResponse::clear()
{
    m_cachedByteSize_ = 0;
    m_channelId.clear();
    m_success = false;
    m_errorCode.clear();
    m_errorMessage.clear();
    m_mutedUsers.clear();
}

size_t SubscribeResponse::getByteSize()
{
    size_t total_size = 0;
    if (!m_channelId.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_channelId);
    }
    if (m_success != false)
    {
        total_size += 1 + 1;
    }
    if (!m_errorCode.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_errorCode);
    }
    if (!m_errorMessage.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_errorMessage);
    }
    total_size += 1 * m_mutedUsers.size();
    for (EADPSDK_SIZE_T i = 0; i < m_mutedUsers.size(); i++)
    {
        total_size += ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_mutedUsers[i]);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* SubscribeResponse::onSerialize(uint8_t* target) const
{
    if (!getChannelId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getChannelId(), target);
    }
    if (getSuccess() != false)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeBool(2, getSuccess(), target);
    }
    if (!getErrorCode().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(3, getErrorCode(), target);
    }
    if (!getErrorMessage().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(4, getErrorMessage(), target);
    }
    for (EADPSDK_SIZE_T i = 0; i < m_mutedUsers.size(); i++)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(5, *m_mutedUsers[i], target);
    }
    return target;
}

bool SubscribeResponse::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    if (!input->readString(&m_channelId))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(16)) goto parse_success;
                break;
            }

            case 2:
            {
                if (tag == 16)
                {
                    parse_success:
                    if (!input->readBool(&m_success))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(26)) goto parse_errorCode;
                break;
            }

            case 3:
            {
                if (tag == 26)
                {
                    parse_errorCode:
                    if (!input->readString(&m_errorCode))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(34)) goto parse_errorMessage;
                break;
            }

            case 4:
            {
                if (tag == 34)
                {
                    parse_errorMessage:
                    if (!input->readString(&m_errorMessage))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(42)) goto parse_mutedUsers;
                break;
            }

            case 5:
            {
                if (tag == 42)
                {
                    parse_mutedUsers:
                    parse_loop_mutedUsers:
                    auto tmpMutedUsers = m_allocator_.makeShared<com::ea::eadp::antelope::protocol::MutedUserV1>(m_allocator_);
                    if (!input->readMessage(tmpMutedUsers.get()))
                    {
                        return false;
                    }
                    m_mutedUsers.push_back(tmpMutedUsers);
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(42)) goto parse_loop_mutedUsers;
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
::eadp::foundation::String SubscribeResponse::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_channelId.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  channelId: \"%s\",\n", indent.c_str(), m_channelId.c_str());
    }
    if (m_success != false)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  success: %d,\n", indent.c_str(), m_success);
    }
    if (!m_errorCode.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  errorCode: \"%s\",\n", indent.c_str(), m_errorCode.c_str());
    }
    if (!m_errorMessage.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  errorMessage: \"%s\",\n", indent.c_str(), m_errorMessage.c_str());
    }
    if (m_mutedUsers.size() > 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  mutedUsers: [\n", indent.c_str());
        for (auto iter = m_mutedUsers.begin(); iter != m_mutedUsers.end(); iter++)
        {
            ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s    %s,\n", indent.c_str(), (*iter)->toString(indent + "  ").c_str());
        }
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  ],\n", indent.c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& SubscribeResponse::getChannelId() const
{
    return m_channelId;
}

void SubscribeResponse::setChannelId(const ::eadp::foundation::String& value)
{
    m_channelId = value;
}

void SubscribeResponse::setChannelId(const char8_t* value)
{
    m_channelId = m_allocator_.make<::eadp::foundation::String>(value);
}


bool SubscribeResponse::getSuccess() const
{
    return m_success;
}

void SubscribeResponse::setSuccess(bool value)
{
    m_success = value;
}


const ::eadp::foundation::String& SubscribeResponse::getErrorCode() const
{
    return m_errorCode;
}

void SubscribeResponse::setErrorCode(const ::eadp::foundation::String& value)
{
    m_errorCode = value;
}

void SubscribeResponse::setErrorCode(const char8_t* value)
{
    m_errorCode = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& SubscribeResponse::getErrorMessage() const
{
    return m_errorMessage;
}

void SubscribeResponse::setErrorMessage(const ::eadp::foundation::String& value)
{
    m_errorMessage = value;
}

void SubscribeResponse::setErrorMessage(const char8_t* value)
{
    m_errorMessage = m_allocator_.make<::eadp::foundation::String>(value);
}


::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::MutedUserV1>>& SubscribeResponse::getMutedUsers()
{
    return m_mutedUsers;
}

}
}
}
}
}
