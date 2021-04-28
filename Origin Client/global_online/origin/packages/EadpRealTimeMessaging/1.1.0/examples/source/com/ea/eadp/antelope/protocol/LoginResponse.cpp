// GENERATED CODE (Source: social_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/protocol/LoginResponse.h>
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

LoginResponse::LoginResponse(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.protocol.LoginResponse"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_success(false)
, m_channels(m_allocator_.make<::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::Channel>>>())
, m_muteList(m_allocator_.make<::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::ChannelMuteList>>>())
, m_errorCode(m_allocator_.emptyString())
, m_errorMessage(m_allocator_.emptyString())
{
}

void LoginResponse::clear()
{
    m_cachedByteSize_ = 0;
    m_success = false;
    m_channels.clear();
    m_muteList.clear();
    m_errorCode.clear();
    m_errorMessage.clear();
}

size_t LoginResponse::getByteSize()
{
    size_t total_size = 0;
    if (m_success != false)
    {
        total_size += 1 + 1;
    }
    total_size += 1 * m_channels.size();
    for (EADPSDK_SIZE_T i = 0; i < m_channels.size(); i++)
    {
        total_size += ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_channels[i]);
    }
    total_size += 1 * m_muteList.size();
    for (EADPSDK_SIZE_T i = 0; i < m_muteList.size(); i++)
    {
        total_size += ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_muteList[i]);
    }
    if (!m_errorCode.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_errorCode);
    }
    if (!m_errorMessage.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_errorMessage);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* LoginResponse::onSerialize(uint8_t* target) const
{
    if (getSuccess() != false)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeBool(1, getSuccess(), target);
    }
    for (EADPSDK_SIZE_T i = 0; i < m_channels.size(); i++)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(2, *m_channels[i], target);
    }
    for (EADPSDK_SIZE_T i = 0; i < m_muteList.size(); i++)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(3, *m_muteList[i], target);
    }
    if (!getErrorCode().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(4, getErrorCode(), target);
    }
    if (!getErrorMessage().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(5, getErrorMessage(), target);
    }
    return target;
}

bool LoginResponse::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    if (!input->readBool(&m_success))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_channels;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_channels:
                    parse_loop_channels:
                    auto tmpChannels = m_allocator_.makeShared<com::ea::eadp::antelope::protocol::Channel>(m_allocator_);
                    if (!input->readMessage(tmpChannels.get()))
                    {
                        return false;
                    }
                    m_channels.push_back(tmpChannels);
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_loop_channels;
                if (input->expectTag(26)) goto parse_loop_muteList;
                break;
            }

            case 3:
            {
                if (tag == 26)
                {
                    parse_loop_muteList:
                    auto tmpMuteList = m_allocator_.makeShared<com::ea::eadp::antelope::protocol::ChannelMuteList>(m_allocator_);
                    if (!input->readMessage(tmpMuteList.get()))
                    {
                        return false;
                    }
                    m_muteList.push_back(tmpMuteList);
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(26)) goto parse_loop_muteList;
                if (input->expectTag(34)) goto parse_errorCode;
                break;
            }

            case 4:
            {
                if (tag == 34)
                {
                    parse_errorCode:
                    if (!input->readString(&m_errorCode))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(42)) goto parse_errorMessage;
                break;
            }

            case 5:
            {
                if (tag == 42)
                {
                    parse_errorMessage:
                    if (!input->readString(&m_errorMessage))
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
::eadp::foundation::String LoginResponse::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (m_success != false)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  success: %d,\n", indent.c_str(), m_success);
    }
    if (m_channels.size() > 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  channels: [\n", indent.c_str());
        for (auto iter = m_channels.begin(); iter != m_channels.end(); iter++)
        {
            ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s    %s,\n", indent.c_str(), (*iter)->toString(indent + "  ").c_str());
        }
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  ],\n", indent.c_str());
    }
    if (m_muteList.size() > 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  muteList: [\n", indent.c_str());
        for (auto iter = m_muteList.begin(); iter != m_muteList.end(); iter++)
        {
            ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s    %s,\n", indent.c_str(), (*iter)->toString(indent + "  ").c_str());
        }
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  ],\n", indent.c_str());
    }
    if (!m_errorCode.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  errorCode: \"%s\",\n", indent.c_str(), m_errorCode.c_str());
    }
    if (!m_errorMessage.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  errorMessage: \"%s\",\n", indent.c_str(), m_errorMessage.c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

bool LoginResponse::getSuccess() const
{
    return m_success;
}

void LoginResponse::setSuccess(bool value)
{
    m_success = value;
}


::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::Channel>>& LoginResponse::getChannels()
{
    return m_channels;
}

::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::ChannelMuteList>>& LoginResponse::getMuteList()
{
    return m_muteList;
}

const ::eadp::foundation::String& LoginResponse::getErrorCode() const
{
    return m_errorCode;
}

void LoginResponse::setErrorCode(const ::eadp::foundation::String& value)
{
    m_errorCode = value;
}

void LoginResponse::setErrorCode(const char8_t* value)
{
    m_errorCode = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& LoginResponse::getErrorMessage() const
{
    return m_errorMessage;
}

void LoginResponse::setErrorMessage(const ::eadp::foundation::String& value)
{
    m_errorMessage = value;
}

void LoginResponse::setErrorMessage(const char8_t* value)
{
    m_errorMessage = m_allocator_.make<::eadp::foundation::String>(value);
}


}
}
}
}
}
