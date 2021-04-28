// Copyright(C) 2019-2020 Electronic Arts, Inc. All rights reserved

#include "LoginRequestV2Job.h"
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/realtimemessaging/RealTimeMessagingError.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope;

using LoginResponseV2 = ResponseT<eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginV2Success>, const ErrorPtr&>;

constexpr char kLoginRequestV2JobName[] = "LoginRequestV2Job";

LoginRequestV2Job::LoginRequestV2Job(IHub* hub,
                                 SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService,
                                 const String& productId,
                                 const String& accessToken,
                                 bool isReconnect,
                                 bool forceDisconnect,
                                 Callback loginCallback) 
: RTMRequestJob(hub, EADPSDK_MOVE(rtmService))
, m_callback(loginCallback)
, m_productId(productId)
, m_accessToken(accessToken)
, m_isReconnect(isReconnect)
, m_forceDisconnect(forceDisconnect)
{
    RTMRequestJob::skipConnectivityCheck();     // To be called by LoginRequestJob only
}

void LoginRequestV2Job::setupOutgoingCommunication(const eadp::foundation::String& requestId)
{
    if (m_accessToken.empty())
    {
        EADPSDK_LOGE(getHub(), kLoginRequestV2JobName, "AccessToken is not set or empty");
        LoginResponseV2::post(getHub(),
            kLoginRequestV2JobName,
            m_callback,
            nullptr, 
            FoundationError::create(getHub()->getAllocator(), FoundationError::Code::INVALID_ARGUMENT, "AccessToken is not set or empty."));
        m_done = true; // Mark current job done
        return;
    }

    if (m_productId.empty())
    {
        EADPSDK_LOGE(getHub(), kLoginRequestV2JobName, "ProductId is not set");
        LoginResponseV2::post(getHub(),
            kLoginRequestV2JobName,
            m_callback,
            nullptr, 
            FoundationError::create(getHub()->getAllocator(), FoundationError::Code::INVALID_ARGUMENT, "ProductId is not set."));
        m_done = true; // Mark current job done
        return;
    }

    SharedPtr<rtm::protocol::LoginRequestV2> loginRequestV2 = getHub()->getAllocator().makeShared<rtm::protocol::LoginRequestV2>(getHub()->getAllocator());
    loginRequestV2->setToken(m_accessToken);
    loginRequestV2->setReconnect(m_isReconnect);
    loginRequestV2->setHeartbeat(true);
    loginRequestV2->setUserType(rtm::protocol::UserType::NUCLEUS);
    loginRequestV2->setProductId(m_productId);
    if (!m_isReconnect)
    {
        loginRequestV2->setSingleSessionForceLogout(m_forceDisconnect);
    }

    SharedPtr<rtm::protocol::CommunicationV1> v1 = getHub()->getAllocator().makeShared<rtm::protocol::CommunicationV1>(getHub()->getAllocator());
    v1->setRequestId(requestId);
    v1->setLoginRequestV2(loginRequestV2);

    SharedPtr<rtm::protocol::Communication> communication = getHub()->getAllocator().makeShared<rtm::protocol::Communication>(getHub()->getAllocator());
    communication->setV1(v1);

    m_communication = getHub()->getAllocator().makeShared<CommunicationWrapper>(communication);
}

void LoginRequestV2Job::onTimeout()
{
    String message = getHub()->getAllocator().make<String>("Login Request timed out.");
    EADPSDK_LOGE(getHub(), kLoginRequestV2JobName, message);
    auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::SOCKET_REQUEST_TIMEOUT, message);

    LoginResponseV2::post(getHub(), kLoginRequestV2JobName, m_callback, nullptr, error);
}

void LoginRequestV2Job::onComplete(SharedPtr<CommunicationWrapper> communicationWrapper)
{
    auto rtmCommunication = communicationWrapper->getRtmCommunication(); 
    if (!rtmCommunication
        || !rtmCommunication->hasV1()
        // Must return either login success or error response
        || (!rtmCommunication->getV1()->hasError()
            && !rtmCommunication->getV1()->hasSuccess()
            && !rtmCommunication->getV1()->getSuccess()->hasLoginV2Success()))
    {
        String message = getHub()->getAllocator().make<String>("Login Response message received is of incorrect type.");
        EADPSDK_LOGE(getHub(), kLoginRequestV2JobName, message);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::INVALID_SERVER_RESPONSE, message);
        LoginResponseV2::post(getHub(), kLoginRequestV2JobName, m_callback, nullptr, error);
        return;
    }

    auto v1 = rtmCommunication->getV1();
    if (v1->hasSuccess() && v1->getSuccess()->hasLoginV2Success())
    {

        String message = getHub()->getAllocator().make<String>("Login Successful");
        EADPSDK_LOGV(getHub(), kLoginRequestV2JobName, message);
        LoginResponseV2::post(getHub(), kLoginRequestV2JobName, m_callback, v1->getSuccess()->getLoginV2Success(), nullptr);
    }
    else
    {
        String message = v1->getError()->getErrorMessage();
        if (message.empty())
        {
            message = getHub()->getAllocator().make<String>("Error received from server in response to login request.");
        }

        EADPSDK_LOGE(getHub(), kLoginRequestV2JobName, message);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), v1->getError()->getErrorCode(), message);
        LoginResponseV2::post(getHub(), kLoginRequestV2JobName, m_callback, nullptr, error);
    }
}

void LoginRequestV2Job::onSendError(const eadp::foundation::ErrorPtr& error)
{
    LoginResponseV2::post(getHub(), kLoginRequestV2JobName, m_callback, nullptr, error);
}
