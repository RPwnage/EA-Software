// Copyright(C) 2019-2020 Electronic Arts, Inc. All rights reserved.

#include "SessionRequestJob.h"
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/realtimemessaging/RealTimeMessagingError.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope;

using SessionResponse = ResponseT<eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionResponseV1>, const ErrorPtr&>;
constexpr char kSessionRequestJobName[] = "SessionRequestJob";

SessionRequestJob::SessionRequestJob(IHub* hub,
                                   SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService, 
                                   Callback callback)
: RTMRequestJob(hub, EADPSDK_MOVE(rtmService))
, m_callback(EADPSDK_MOVE(callback))
{
}

void SessionRequestJob::setupOutgoingCommunication(const eadp::foundation::String& requestId)
{
    auto allocator = getHub()->getAllocator();
    auto sessionRequest = allocator.makeShared<rtm::protocol::SessionRequestV1>(allocator);

    SharedPtr<rtm::protocol::CommunicationV1> v1 = getHub()->getAllocator().makeShared<rtm::protocol::CommunicationV1>(getHub()->getAllocator());
    v1->setRequestId(requestId);
    v1->setSessionRequestV1(sessionRequest);

    SharedPtr<rtm::protocol::Communication> communication = getHub()->getAllocator().makeShared<rtm::protocol::Communication>(getHub()->getAllocator());
    communication->setV1(v1);

    m_communication = getHub()->getAllocator().makeShared<CommunicationWrapper>(communication);
}

void SessionRequestJob::onTimeout()
{
    String message = getHub()->getAllocator().make<String>("Session Request timed out.");
    EADPSDK_LOGE(getHub(), kSessionRequestJobName, message);
    auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::SOCKET_REQUEST_TIMEOUT, message);

    SessionResponse::post(getHub(), kSessionRequestJobName, m_callback, nullptr, error);
}

void SessionRequestJob::onComplete(SharedPtr<CommunicationWrapper> communicationWrapper)
{
    auto rtmCommunication = communicationWrapper->getRtmCommunication();
    if (!rtmCommunication
        || !rtmCommunication->hasV1()
        // Must return either SessionResponseV1 or error response
        || (!rtmCommunication->getV1()->hasError()
            && !rtmCommunication->getV1()->hasSessionResponseV1()))
    {
        String message = getHub()->getAllocator().make<String>("Session Response message received is of incorrect type.");
        EADPSDK_LOGE(getHub(), kSessionRequestJobName, message);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::INVALID_SERVER_RESPONSE, message);
        SessionResponse::post(getHub(), kSessionRequestJobName, m_callback, nullptr, error);
        return;
    }

    auto v1 = rtmCommunication->getV1();
    if (v1->hasSessionResponseV1())
    {
        String message = getHub()->getAllocator().make<String>("Session Request Successful");
        EADPSDK_LOGV(getHub(), kSessionRequestJobName, message);
        SessionResponse::post(getHub(), kSessionRequestJobName, m_callback, v1->getSessionResponseV1(), nullptr);
    }
    else
    {
        String message = v1->getError()->getErrorMessage();
        if (message.empty())
        {
            message = getHub()->getAllocator().make<String>("Error received from server in response to session request.");
        }

        EADPSDK_LOGE(getHub(), kSessionRequestJobName, message);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), v1->getError()->getErrorCode(), message);
        SessionResponse::post(getHub(), kSessionRequestJobName, m_callback, nullptr, error);
    }
}

void SessionRequestJob::onSendError(const eadp::foundation::ErrorPtr& error)
{
    SessionResponse::post(getHub(), kSessionRequestJobName, m_callback, nullptr, error);
}
