// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#include "LoginRequestJob.h"
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/realtimemessaging/RealTimeMessagingError.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope;

using LoginResponse = ResponseT<eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginV2Success>, const ErrorPtr&>;

constexpr char kLoginResponseJobName[] = "LoginResponseJob";

LoginRequestJob::LoginRequestJob(IHub* hub,
    SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService,
    const String productId,
    String accessToken,
    bool isReconnect,
    bool forceDisconnect,
    Callback loginCallback) 
: RTMRequestJob(hub, rtmService)
, m_callback(loginCallback)
, m_productId(productId)
, m_accessToken(accessToken)
, m_isReconnect(isReconnect)
, m_forceDisconnect(forceDisconnect)
{
    RTMRequestJob::skipConnectivityCheck();     // To be called by LoginRequestJob only
}

void LoginRequestJob::setupOutgoingCommunication(const eadp::foundation::String& requestId)
{
    if (m_accessToken.empty())
    {
        EADPSDK_LOGE(getHub(), "RTMService", "AccessToken is not set or empty");
        LoginResponse::post(getHub(),
            kLoginResponseJobName,
            m_callback,
            nullptr, 
            FoundationError::create(getHub()->getAllocator(), FoundationError::Code::INVALID_ARGUMENT, "AccessToken is not set or empty."));
        m_done = true; // Mark current job done
        return;
    }

    if (m_productId.empty())
    {
        EADPSDK_LOGE(getHub(), "RTMService", "ProductId is not set");
        LoginResponse::post(getHub(),
            kLoginResponseJobName,
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


void LoginRequestJob::onTimeout()
{
    String message = getHub()->getAllocator().make<String>("Login Request timed out.");
    EADPSDK_LOGE(getHub(), "LoginRequestJob", message);
    auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::SOCKET_REQUEST_TIMEOUT, message);

    LoginResponse::post(getHub(), kLoginResponseJobName, m_callback, nullptr, error);
}

void LoginRequestJob::onComplete(SharedPtr<CommunicationWrapper> communicationWrapper)
{
    if (communicationWrapper->hasRtmCommunication())
    {
        SharedPtr<com::ea::eadp::antelope::rtm::protocol::Communication> communication = communicationWrapper->getRtmCommunication();
        if (communication->hasV1())
        {
            if (communication->getV1()->hasSuccess()
                && communication->getV1()->getSuccess()->hasLoginV2Success())
            {

                String message = getHub()->getAllocator().make<String>("Login Successful");
                EADPSDK_LOGV(getHub(), "LoginRequestJob", message);
                LoginResponse::post(getHub(), kLoginResponseJobName, m_callback, communication->getV1()->getSuccess()->getLoginV2Success(), nullptr);
            }
            else if (communication->getV1()->hasError())
            {
                String message = communication->getV1()->getError()->getErrorMessage();
                if (message.empty())
                {
                    message = getHub()->getAllocator().make<String>("Error received from server in response to login request.");
                }

                EADPSDK_LOGE(getHub(), "LoginRequestJob", message);

                auto error = RealTimeMessagingError::create(getHub()->getAllocator(), communication->getV1()->getError()->getErrorCode(), message);

                LoginResponse::post(getHub(), kLoginResponseJobName, m_callback, nullptr, error);
            }
            return;
        }
    }

    String message = getHub()->getAllocator().make<String>("Login Response message received is of incorrect type.");
    EADPSDK_LOGE(getHub(), "LoginRequestJob", message);
    auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::INVALID_SERVER_RESPONSE, message);

    LoginResponse::post(getHub(), kLoginResponseJobName, m_callback, nullptr, error);

}

void LoginRequestJob::onSendError(const eadp::foundation::ErrorPtr& error)
{
    LoginResponse::post(getHub(), kLoginResponseJobName, m_callback, nullptr, error);
}
