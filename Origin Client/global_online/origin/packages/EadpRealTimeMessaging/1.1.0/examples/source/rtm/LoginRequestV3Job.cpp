// Copyright(C) 2020 Electronic Arts, Inc. All rights reserved

#include "LoginRequestV3Job.h"
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/realtimemessaging/RealTimeMessagingError.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope;

using LoginResponseV3 = ResponseT<eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginV3Response>, eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginErrorV1>, const ErrorPtr&>;
constexpr char kLoginRequestV3JobName[] = "LoginRequestV3Job";

LoginRequestV3Job::LoginRequestV3Job(IHub* hub,
                                 SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService,
                                 const String& productId,
                                 const String& clientVersion,
                                 const String& accessToken,
                                 bool isReconnect,
                                 bool forceDisconnect,
                                 const String& sessionKey,
                                 const Callback& loginCallback) 
: RTMRequestJob(hub, EADPSDK_MOVE(rtmService))
, m_callback(loginCallback)
, m_productId(productId)
, m_clientVersion(clientVersion)
, m_accessToken(accessToken)
, m_isReconnect(isReconnect)
, m_forceDisconnect(forceDisconnect)
, m_sessionKey(sessionKey)
{
    RTMRequestJob::skipConnectivityCheck();     // To be called by LoginRequestJob only
}

void LoginRequestV3Job::setupOutgoingCommunication(const eadp::foundation::String& requestId)
{
    if (m_accessToken.empty())
    {
        EADPSDK_LOGE(getHub(), kLoginRequestV3JobName, "AccessToken is not set or empty");
        LoginResponseV3::post(getHub(),
            kLoginRequestV3JobName,
            m_callback,
            nullptr, 
            nullptr,
            FoundationError::create(getHub()->getAllocator(), FoundationError::Code::INVALID_ARGUMENT, "AccessToken is not set or empty."));
        m_done = true; // Mark current job done
        return;
    }

    if (m_productId.empty())
    {
        EADPSDK_LOGE(getHub(), kLoginRequestV3JobName, "ProductId is not set");
        LoginResponseV3::post(getHub(),
            kLoginRequestV3JobName,
            m_callback,
            nullptr, 
            nullptr,
            FoundationError::create(getHub()->getAllocator(), FoundationError::Code::INVALID_ARGUMENT, "ProductId is not set."));
        m_done = true; // Mark current job done
        return;
    }

    if (m_clientVersion.empty())
    {
        EADPSDK_LOGE(getHub(), kLoginRequestV3JobName, "ClientVersion is not set");
        LoginResponseV3::post(getHub(),
            kLoginRequestV3JobName,
            m_callback,
            nullptr,
            nullptr,
            FoundationError::create(getHub()->getAllocator(), FoundationError::Code::INVALID_ARGUMENT, "ClientVersion is not set."));
        m_done = true; // Mark current job done
        return;
    }

    SharedPtr<rtm::protocol::LoginRequestV3> loginRequestV3 = getHub()->getAllocator().makeShared<rtm::protocol::LoginRequestV3>(getHub()->getAllocator());
    loginRequestV3->setToken(m_accessToken);
    loginRequestV3->setReconnect(m_isReconnect);
    loginRequestV3->setHeartbeat(true);
    loginRequestV3->setUserType(rtm::protocol::UserType::NUCLEUS);
    loginRequestV3->setProductId(m_productId);
    loginRequestV3->setPlatform(rtm::protocol::PlatformV1::PC);
    loginRequestV3->setClientVersion(m_clientVersion);

    if (m_forceDisconnect)
    {
        if (m_sessionKey.empty())
        {
            EADPSDK_LOGE(getHub(), kLoginRequestV3JobName, "SessionKey is not set");
            LoginResponseV3::post(getHub(),
                kLoginRequestV3JobName,
                m_callback,
                nullptr,
                nullptr,
                FoundationError::create(getHub()->getAllocator(), FoundationError::Code::INVALID_ARGUMENT, "SessionKey is not set."));
            m_done = true; // Mark current job done
            return;
        }

        loginRequestV3->setForceDisconnectSessionKey(m_sessionKey);
    }    

    if (m_isReconnect)
    {
        if (m_sessionKey.empty())
        {
            EADPSDK_LOGE(getHub(), kLoginRequestV3JobName, "SessionKey is not set");
            LoginResponseV3::post(getHub(),
                kLoginRequestV3JobName,
                m_callback,
                nullptr,
                nullptr,
                FoundationError::create(getHub()->getAllocator(), FoundationError::Code::INVALID_ARGUMENT, "SessionKey is not set."));
            m_done = true; // Mark current job done
            return;
        }

        loginRequestV3->setSessionKey(m_sessionKey);
    }

    SharedPtr<rtm::protocol::CommunicationV1> v1 = getHub()->getAllocator().makeShared<rtm::protocol::CommunicationV1>(getHub()->getAllocator());
    v1->setRequestId(requestId);
    v1->setLoginRequestV3(loginRequestV3);

    SharedPtr<rtm::protocol::Communication> communication = getHub()->getAllocator().makeShared<rtm::protocol::Communication>(getHub()->getAllocator());
    communication->setV1(v1);

    m_communication = getHub()->getAllocator().makeShared<CommunicationWrapper>(communication);
}

void LoginRequestV3Job::onTimeout()
{
    String message = getHub()->getAllocator().make<String>("Login Request timed out.");
    EADPSDK_LOGE(getHub(), kLoginRequestV3JobName, message);
    auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::SOCKET_REQUEST_TIMEOUT, message);

    LoginResponseV3::post(getHub(), kLoginRequestV3JobName, m_callback, nullptr, nullptr, error);
}

void LoginRequestV3Job::onComplete(SharedPtr<CommunicationWrapper> communicationWrapper)
{
    auto rtmCommunication = communicationWrapper->getRtmCommunication(); 
    if (!rtmCommunication
        || !rtmCommunication->hasV1()
        // Must return either LoginV3Response or error response
        || (!rtmCommunication->getV1()->hasError()
            && !rtmCommunication->getV1()->hasSuccess()
            && !rtmCommunication->getV1()->getSuccess()->hasLoginV3Response()))
    {
        String message = getHub()->getAllocator().make<String>("Login Response message received is of incorrect type.");
        EADPSDK_LOGE(getHub(), kLoginRequestV3JobName, message);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::INVALID_SERVER_RESPONSE, message);
        LoginResponseV3::post(getHub(), kLoginRequestV3JobName, m_callback, nullptr, nullptr, error);
        return;
    }

    auto v1 = rtmCommunication->getV1();
    if (v1->hasSuccess() && v1->getSuccess()->hasLoginV3Response())
    {

        String message = getHub()->getAllocator().make<String>("Login Successful");
        EADPSDK_LOGV(getHub(), kLoginRequestV3JobName, message);
        LoginResponseV3::post(getHub(), kLoginRequestV3JobName, m_callback, v1->getSuccess()->getLoginV3Response(), nullptr, nullptr);
    }
    else
    {
        String message = v1->getError()->getErrorMessage();
        if (message.empty())
        {
            message = getHub()->getAllocator().make<String>("Error received from server in response to login request.");
        }

        EADPSDK_LOGE(getHub(), kLoginRequestV3JobName, message);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), v1->getError()->getErrorCode(), message);

        LoginResponseV3::post(getHub(), kLoginRequestV3JobName, m_callback, nullptr, v1->getError()->getLoginError(), error);
    }
}

void LoginRequestV3Job::onSendError(const eadp::foundation::ErrorPtr& error)
{
    LoginResponseV3::post(getHub(), kLoginRequestV3JobName, m_callback, nullptr, nullptr, error);
}
