// GENERATED CODE (Source: social_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/protocol/Communication.h>
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

Communication::Communication(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.protocol.Communication"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_header(nullptr)
, m_bodyCase(BodyCase::kNotSet)
{
}

void Communication::clear()
{
    m_cachedByteSize_ = 0;
    m_header.reset();
    clearBody();
}

size_t Communication::getByteSize()
{
    size_t total_size = 0;
    if (m_header != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_header);
    }
    if (hasLoginRequest() && m_body.m_loginRequest)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_loginRequest);
    }
    if (hasLoginResponse() && m_body.m_loginResponse)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_loginResponse);
    }
    if (hasPublishTextRequest() && m_body.m_publishTextRequest)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_publishTextRequest);
    }
    if (hasPublishBinaryRequest() && m_body.m_publishBinaryRequest)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_publishBinaryRequest);
    }
    if (hasPublishResponse() && m_body.m_publishResponse)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_publishResponse);
    }
    if (hasChannelMessage() && m_body.m_channelMessage)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_channelMessage);
    }
    if (hasSubscribeRequest() && m_body.m_subscribeRequest)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_subscribeRequest);
    }
    if (hasSubscribeResponse() && m_body.m_subscribeResponse)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_subscribeResponse);
    }
    if (hasUnsubscribeRequest() && m_body.m_unsubscribeRequest)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_unsubscribeRequest);
    }
    if (hasUnsubscribeResponse() && m_body.m_unsubscribeResponse)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_unsubscribeResponse);
    }
    if (hasHistoryRequest() && m_body.m_historyRequest)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_historyRequest);
    }
    if (hasHistoryResponse() && m_body.m_historyResponse)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_historyResponse);
    }
    if (hasLogoutRequest() && m_body.m_logoutRequest)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_logoutRequest);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* Communication::onSerialize(uint8_t* target) const
{
    if (m_header != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(1, *m_header, target);
    }
    if (hasLoginRequest() && m_body.m_loginRequest)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(2, *m_body.m_loginRequest, target);
    }
    if (hasLoginResponse() && m_body.m_loginResponse)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(3, *m_body.m_loginResponse, target);
    }
    if (hasPublishTextRequest() && m_body.m_publishTextRequest)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(4, *m_body.m_publishTextRequest, target);
    }
    if (hasPublishBinaryRequest() && m_body.m_publishBinaryRequest)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(5, *m_body.m_publishBinaryRequest, target);
    }
    if (hasPublishResponse() && m_body.m_publishResponse)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(6, *m_body.m_publishResponse, target);
    }
    if (hasChannelMessage() && m_body.m_channelMessage)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(7, *m_body.m_channelMessage, target);
    }
    if (hasSubscribeRequest() && m_body.m_subscribeRequest)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(8, *m_body.m_subscribeRequest, target);
    }
    if (hasSubscribeResponse() && m_body.m_subscribeResponse)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(9, *m_body.m_subscribeResponse, target);
    }
    if (hasUnsubscribeRequest() && m_body.m_unsubscribeRequest)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(10, *m_body.m_unsubscribeRequest, target);
    }
    if (hasUnsubscribeResponse() && m_body.m_unsubscribeResponse)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(11, *m_body.m_unsubscribeResponse, target);
    }
    if (hasHistoryRequest() && m_body.m_historyRequest)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(12, *m_body.m_historyRequest, target);
    }
    if (hasHistoryResponse() && m_body.m_historyResponse)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(13, *m_body.m_historyResponse, target);
    }
    if (hasLogoutRequest() && m_body.m_logoutRequest)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(14, *m_body.m_logoutRequest, target);
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
                    if (!input->readMessage(getHeader().get())) return false;
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_loginRequest;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_loginRequest:
                    clearBody();
                    if (!input->readMessage(getLoginRequest().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(26)) goto parse_loginResponse;
                break;
            }

            case 3:
            {
                if (tag == 26)
                {
                    parse_loginResponse:
                    clearBody();
                    if (!input->readMessage(getLoginResponse().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(34)) goto parse_publishTextRequest;
                break;
            }

            case 4:
            {
                if (tag == 34)
                {
                    parse_publishTextRequest:
                    clearBody();
                    if (!input->readMessage(getPublishTextRequest().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(42)) goto parse_publishBinaryRequest;
                break;
            }

            case 5:
            {
                if (tag == 42)
                {
                    parse_publishBinaryRequest:
                    clearBody();
                    if (!input->readMessage(getPublishBinaryRequest().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(50)) goto parse_publishResponse;
                break;
            }

            case 6:
            {
                if (tag == 50)
                {
                    parse_publishResponse:
                    clearBody();
                    if (!input->readMessage(getPublishResponse().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(58)) goto parse_channelMessage;
                break;
            }

            case 7:
            {
                if (tag == 58)
                {
                    parse_channelMessage:
                    clearBody();
                    if (!input->readMessage(getChannelMessage().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(66)) goto parse_subscribeRequest;
                break;
            }

            case 8:
            {
                if (tag == 66)
                {
                    parse_subscribeRequest:
                    clearBody();
                    if (!input->readMessage(getSubscribeRequest().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(74)) goto parse_subscribeResponse;
                break;
            }

            case 9:
            {
                if (tag == 74)
                {
                    parse_subscribeResponse:
                    clearBody();
                    if (!input->readMessage(getSubscribeResponse().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(82)) goto parse_unsubscribeRequest;
                break;
            }

            case 10:
            {
                if (tag == 82)
                {
                    parse_unsubscribeRequest:
                    clearBody();
                    if (!input->readMessage(getUnsubscribeRequest().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(90)) goto parse_unsubscribeResponse;
                break;
            }

            case 11:
            {
                if (tag == 90)
                {
                    parse_unsubscribeResponse:
                    clearBody();
                    if (!input->readMessage(getUnsubscribeResponse().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(98)) goto parse_historyRequest;
                break;
            }

            case 12:
            {
                if (tag == 98)
                {
                    parse_historyRequest:
                    clearBody();
                    if (!input->readMessage(getHistoryRequest().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(106)) goto parse_historyResponse;
                break;
            }

            case 13:
            {
                if (tag == 106)
                {
                    parse_historyResponse:
                    clearBody();
                    if (!input->readMessage(getHistoryResponse().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(114)) goto parse_logoutRequest;
                break;
            }

            case 14:
            {
                if (tag == 114)
                {
                    parse_logoutRequest:
                    clearBody();
                    if (!input->readMessage(getLogoutRequest().get()))
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
    if (m_header != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  header: %s,\n", indent.c_str(), m_header->toString(indent + "  ").c_str());
    }
    if (hasLoginRequest() && m_body.m_loginRequest)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  loginRequest: %s,\n", indent.c_str(), m_body.m_loginRequest->toString(indent + "  ").c_str());
    }
    if (hasLoginResponse() && m_body.m_loginResponse)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  loginResponse: %s,\n", indent.c_str(), m_body.m_loginResponse->toString(indent + "  ").c_str());
    }
    if (hasPublishTextRequest() && m_body.m_publishTextRequest)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  publishTextRequest: %s,\n", indent.c_str(), m_body.m_publishTextRequest->toString(indent + "  ").c_str());
    }
    if (hasPublishBinaryRequest() && m_body.m_publishBinaryRequest)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  publishBinaryRequest: %s,\n", indent.c_str(), m_body.m_publishBinaryRequest->toString(indent + "  ").c_str());
    }
    if (hasPublishResponse() && m_body.m_publishResponse)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  publishResponse: %s,\n", indent.c_str(), m_body.m_publishResponse->toString(indent + "  ").c_str());
    }
    if (hasChannelMessage() && m_body.m_channelMessage)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  channelMessage: %s,\n", indent.c_str(), m_body.m_channelMessage->toString(indent + "  ").c_str());
    }
    if (hasSubscribeRequest() && m_body.m_subscribeRequest)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  subscribeRequest: %s,\n", indent.c_str(), m_body.m_subscribeRequest->toString(indent + "  ").c_str());
    }
    if (hasSubscribeResponse() && m_body.m_subscribeResponse)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  subscribeResponse: %s,\n", indent.c_str(), m_body.m_subscribeResponse->toString(indent + "  ").c_str());
    }
    if (hasUnsubscribeRequest() && m_body.m_unsubscribeRequest)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  unsubscribeRequest: %s,\n", indent.c_str(), m_body.m_unsubscribeRequest->toString(indent + "  ").c_str());
    }
    if (hasUnsubscribeResponse() && m_body.m_unsubscribeResponse)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  unsubscribeResponse: %s,\n", indent.c_str(), m_body.m_unsubscribeResponse->toString(indent + "  ").c_str());
    }
    if (hasHistoryRequest() && m_body.m_historyRequest)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  historyRequest: %s,\n", indent.c_str(), m_body.m_historyRequest->toString(indent + "  ").c_str());
    }
    if (hasHistoryResponse() && m_body.m_historyResponse)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  historyResponse: %s,\n", indent.c_str(), m_body.m_historyResponse->toString(indent + "  ").c_str());
    }
    if (hasLogoutRequest() && m_body.m_logoutRequest)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  logoutRequest: %s,\n", indent.c_str(), m_body.m_logoutRequest->toString(indent + "  ").c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::Header> Communication::getHeader()
{
    if (m_header == nullptr)
    {
        m_header = m_allocator_.makeShared<com::ea::eadp::antelope::protocol::Header>(m_allocator_);
    }
    return m_header;
}

void Communication::setHeader(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::Header> val)
{
    m_header = val;
}

bool Communication::hasHeader()
{
    return m_header != nullptr;
}

void Communication::releaseHeader()
{
    m_header.reset();
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::LoginRequest> Communication::getLoginRequest()
{
    if (!hasLoginRequest())
    {
        setLoginRequest(m_allocator_.makeShared<com::ea::eadp::antelope::protocol::LoginRequest>(m_allocator_));
    }
    return m_body.m_loginRequest;
}

bool Communication::hasLoginRequest() const
{
    return m_bodyCase == BodyCase::kLoginRequest;
}

void Communication::setLoginRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::LoginRequest> value)
{
    clearBody();
    new(&m_body.m_loginRequest) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::LoginRequest>(value);
    m_bodyCase = BodyCase::kLoginRequest;
}

void Communication::releaseLoginRequest()
{
    if (hasLoginRequest())
    {
        m_body.m_loginRequest.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::LoginResponse> Communication::getLoginResponse()
{
    if (!hasLoginResponse())
    {
        setLoginResponse(m_allocator_.makeShared<com::ea::eadp::antelope::protocol::LoginResponse>(m_allocator_));
    }
    return m_body.m_loginResponse;
}

bool Communication::hasLoginResponse() const
{
    return m_bodyCase == BodyCase::kLoginResponse;
}

void Communication::setLoginResponse(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::LoginResponse> value)
{
    clearBody();
    new(&m_body.m_loginResponse) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::LoginResponse>(value);
    m_bodyCase = BodyCase::kLoginResponse;
}

void Communication::releaseLoginResponse()
{
    if (hasLoginResponse())
    {
        m_body.m_loginResponse.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::PublishTextRequest> Communication::getPublishTextRequest()
{
    if (!hasPublishTextRequest())
    {
        setPublishTextRequest(m_allocator_.makeShared<com::ea::eadp::antelope::protocol::PublishTextRequest>(m_allocator_));
    }
    return m_body.m_publishTextRequest;
}

bool Communication::hasPublishTextRequest() const
{
    return m_bodyCase == BodyCase::kPublishTextRequest;
}

void Communication::setPublishTextRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::PublishTextRequest> value)
{
    clearBody();
    new(&m_body.m_publishTextRequest) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::PublishTextRequest>(value);
    m_bodyCase = BodyCase::kPublishTextRequest;
}

void Communication::releasePublishTextRequest()
{
    if (hasPublishTextRequest())
    {
        m_body.m_publishTextRequest.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::PublishBinaryRequest> Communication::getPublishBinaryRequest()
{
    if (!hasPublishBinaryRequest())
    {
        setPublishBinaryRequest(m_allocator_.makeShared<com::ea::eadp::antelope::protocol::PublishBinaryRequest>(m_allocator_));
    }
    return m_body.m_publishBinaryRequest;
}

bool Communication::hasPublishBinaryRequest() const
{
    return m_bodyCase == BodyCase::kPublishBinaryRequest;
}

void Communication::setPublishBinaryRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::PublishBinaryRequest> value)
{
    clearBody();
    new(&m_body.m_publishBinaryRequest) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::PublishBinaryRequest>(value);
    m_bodyCase = BodyCase::kPublishBinaryRequest;
}

void Communication::releasePublishBinaryRequest()
{
    if (hasPublishBinaryRequest())
    {
        m_body.m_publishBinaryRequest.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::PublishResponse> Communication::getPublishResponse()
{
    if (!hasPublishResponse())
    {
        setPublishResponse(m_allocator_.makeShared<com::ea::eadp::antelope::protocol::PublishResponse>(m_allocator_));
    }
    return m_body.m_publishResponse;
}

bool Communication::hasPublishResponse() const
{
    return m_bodyCase == BodyCase::kPublishResponse;
}

void Communication::setPublishResponse(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::PublishResponse> value)
{
    clearBody();
    new(&m_body.m_publishResponse) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::PublishResponse>(value);
    m_bodyCase = BodyCase::kPublishResponse;
}

void Communication::releasePublishResponse()
{
    if (hasPublishResponse())
    {
        m_body.m_publishResponse.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::ChannelMessage> Communication::getChannelMessage()
{
    if (!hasChannelMessage())
    {
        setChannelMessage(m_allocator_.makeShared<com::ea::eadp::antelope::protocol::ChannelMessage>(m_allocator_));
    }
    return m_body.m_channelMessage;
}

bool Communication::hasChannelMessage() const
{
    return m_bodyCase == BodyCase::kChannelMessage;
}

void Communication::setChannelMessage(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::ChannelMessage> value)
{
    clearBody();
    new(&m_body.m_channelMessage) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::ChannelMessage>(value);
    m_bodyCase = BodyCase::kChannelMessage;
}

void Communication::releaseChannelMessage()
{
    if (hasChannelMessage())
    {
        m_body.m_channelMessage.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::SubscribeRequest> Communication::getSubscribeRequest()
{
    if (!hasSubscribeRequest())
    {
        setSubscribeRequest(m_allocator_.makeShared<com::ea::eadp::antelope::protocol::SubscribeRequest>(m_allocator_));
    }
    return m_body.m_subscribeRequest;
}

bool Communication::hasSubscribeRequest() const
{
    return m_bodyCase == BodyCase::kSubscribeRequest;
}

void Communication::setSubscribeRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::SubscribeRequest> value)
{
    clearBody();
    new(&m_body.m_subscribeRequest) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::SubscribeRequest>(value);
    m_bodyCase = BodyCase::kSubscribeRequest;
}

void Communication::releaseSubscribeRequest()
{
    if (hasSubscribeRequest())
    {
        m_body.m_subscribeRequest.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::SubscribeResponse> Communication::getSubscribeResponse()
{
    if (!hasSubscribeResponse())
    {
        setSubscribeResponse(m_allocator_.makeShared<com::ea::eadp::antelope::protocol::SubscribeResponse>(m_allocator_));
    }
    return m_body.m_subscribeResponse;
}

bool Communication::hasSubscribeResponse() const
{
    return m_bodyCase == BodyCase::kSubscribeResponse;
}

void Communication::setSubscribeResponse(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::SubscribeResponse> value)
{
    clearBody();
    new(&m_body.m_subscribeResponse) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::SubscribeResponse>(value);
    m_bodyCase = BodyCase::kSubscribeResponse;
}

void Communication::releaseSubscribeResponse()
{
    if (hasSubscribeResponse())
    {
        m_body.m_subscribeResponse.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::UnsubscribeRequest> Communication::getUnsubscribeRequest()
{
    if (!hasUnsubscribeRequest())
    {
        setUnsubscribeRequest(m_allocator_.makeShared<com::ea::eadp::antelope::protocol::UnsubscribeRequest>(m_allocator_));
    }
    return m_body.m_unsubscribeRequest;
}

bool Communication::hasUnsubscribeRequest() const
{
    return m_bodyCase == BodyCase::kUnsubscribeRequest;
}

void Communication::setUnsubscribeRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::UnsubscribeRequest> value)
{
    clearBody();
    new(&m_body.m_unsubscribeRequest) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::UnsubscribeRequest>(value);
    m_bodyCase = BodyCase::kUnsubscribeRequest;
}

void Communication::releaseUnsubscribeRequest()
{
    if (hasUnsubscribeRequest())
    {
        m_body.m_unsubscribeRequest.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::UnsubscribeResponse> Communication::getUnsubscribeResponse()
{
    if (!hasUnsubscribeResponse())
    {
        setUnsubscribeResponse(m_allocator_.makeShared<com::ea::eadp::antelope::protocol::UnsubscribeResponse>(m_allocator_));
    }
    return m_body.m_unsubscribeResponse;
}

bool Communication::hasUnsubscribeResponse() const
{
    return m_bodyCase == BodyCase::kUnsubscribeResponse;
}

void Communication::setUnsubscribeResponse(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::UnsubscribeResponse> value)
{
    clearBody();
    new(&m_body.m_unsubscribeResponse) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::UnsubscribeResponse>(value);
    m_bodyCase = BodyCase::kUnsubscribeResponse;
}

void Communication::releaseUnsubscribeResponse()
{
    if (hasUnsubscribeResponse())
    {
        m_body.m_unsubscribeResponse.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::HistoryRequest> Communication::getHistoryRequest()
{
    if (!hasHistoryRequest())
    {
        setHistoryRequest(m_allocator_.makeShared<com::ea::eadp::antelope::protocol::HistoryRequest>(m_allocator_));
    }
    return m_body.m_historyRequest;
}

bool Communication::hasHistoryRequest() const
{
    return m_bodyCase == BodyCase::kHistoryRequest;
}

void Communication::setHistoryRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::HistoryRequest> value)
{
    clearBody();
    new(&m_body.m_historyRequest) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::HistoryRequest>(value);
    m_bodyCase = BodyCase::kHistoryRequest;
}

void Communication::releaseHistoryRequest()
{
    if (hasHistoryRequest())
    {
        m_body.m_historyRequest.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::HistoryResponse> Communication::getHistoryResponse()
{
    if (!hasHistoryResponse())
    {
        setHistoryResponse(m_allocator_.makeShared<com::ea::eadp::antelope::protocol::HistoryResponse>(m_allocator_));
    }
    return m_body.m_historyResponse;
}

bool Communication::hasHistoryResponse() const
{
    return m_bodyCase == BodyCase::kHistoryResponse;
}

void Communication::setHistoryResponse(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::HistoryResponse> value)
{
    clearBody();
    new(&m_body.m_historyResponse) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::HistoryResponse>(value);
    m_bodyCase = BodyCase::kHistoryResponse;
}

void Communication::releaseHistoryResponse()
{
    if (hasHistoryResponse())
    {
        m_body.m_historyResponse.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::LogoutRequest> Communication::getLogoutRequest()
{
    if (!hasLogoutRequest())
    {
        setLogoutRequest(m_allocator_.makeShared<com::ea::eadp::antelope::protocol::LogoutRequest>(m_allocator_));
    }
    return m_body.m_logoutRequest;
}

bool Communication::hasLogoutRequest() const
{
    return m_bodyCase == BodyCase::kLogoutRequest;
}

void Communication::setLogoutRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::LogoutRequest> value)
{
    clearBody();
    new(&m_body.m_logoutRequest) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::LogoutRequest>(value);
    m_bodyCase = BodyCase::kLogoutRequest;
}

void Communication::releaseLogoutRequest()
{
    if (hasLogoutRequest())
    {
        m_body.m_logoutRequest.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


void Communication::clearBody()
{
    switch(getBodyCase())
    {
        case BodyCase::kLoginRequest:
            m_body.m_loginRequest.reset();
            break;
        case BodyCase::kLoginResponse:
            m_body.m_loginResponse.reset();
            break;
        case BodyCase::kPublishTextRequest:
            m_body.m_publishTextRequest.reset();
            break;
        case BodyCase::kPublishBinaryRequest:
            m_body.m_publishBinaryRequest.reset();
            break;
        case BodyCase::kPublishResponse:
            m_body.m_publishResponse.reset();
            break;
        case BodyCase::kChannelMessage:
            m_body.m_channelMessage.reset();
            break;
        case BodyCase::kSubscribeRequest:
            m_body.m_subscribeRequest.reset();
            break;
        case BodyCase::kSubscribeResponse:
            m_body.m_subscribeResponse.reset();
            break;
        case BodyCase::kUnsubscribeRequest:
            m_body.m_unsubscribeRequest.reset();
            break;
        case BodyCase::kUnsubscribeResponse:
            m_body.m_unsubscribeResponse.reset();
            break;
        case BodyCase::kHistoryRequest:
            m_body.m_historyRequest.reset();
            break;
        case BodyCase::kHistoryResponse:
            m_body.m_historyResponse.reset();
            break;
        case BodyCase::kLogoutRequest:
            m_body.m_logoutRequest.reset();
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
