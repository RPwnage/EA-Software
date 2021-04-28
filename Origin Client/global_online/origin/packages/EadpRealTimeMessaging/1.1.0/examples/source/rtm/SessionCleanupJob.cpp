// Copyright(C) 2019-2020 Electronic Arts, Inc. All rights reserved.

#include "SessionCleanupJob.h"
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/realtimemessaging/RealTimeMessagingError.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope;

using SessionCleanupResponse = ResponseT<const ErrorPtr&>;
constexpr char kSessionCleanupJobName[] = "SessionCleanupJob";

SessionCleanupJob::SessionCleanupJob(IHub* hub,
                                   SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService, 
                                   const String& sessionKey,
                                   Callback callback)
: RTMRequestJob(hub, EADPSDK_MOVE(rtmService))
, m_sessionKey(sessionKey)
, m_callback(EADPSDK_MOVE(callback))
{
}

void SessionCleanupJob::setupOutgoingCommunication(const eadp::foundation::String& requestId)
{
    auto allocator = getHub()->getAllocator();
    auto sessionCleanup = allocator.makeShared<rtm::protocol::SessionCleanupV1>(allocator);
    sessionCleanup->setSessionKey(m_sessionKey);

    SharedPtr<rtm::protocol::CommunicationV1> v1 = getHub()->getAllocator().makeShared<rtm::protocol::CommunicationV1>(getHub()->getAllocator());
    v1->setRequestId(requestId);
    v1->setSessionCleanupV1(sessionCleanup);

    

    SharedPtr<rtm::protocol::Communication> communication = getHub()->getAllocator().makeShared<rtm::protocol::Communication>(getHub()->getAllocator());
    communication->setV1(v1);

    m_communication = getHub()->getAllocator().makeShared<CommunicationWrapper>(communication);
}

void SessionCleanupJob::onTimeout()
{
    String message = getHub()->getAllocator().make<String>("Session Cleanup timed out.");
    EADPSDK_LOGE(getHub(), kSessionCleanupJobName, message);
    auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::SOCKET_REQUEST_TIMEOUT, message);

    SessionCleanupResponse::post(getHub(), kSessionCleanupJobName, m_callback, error);
}

void SessionCleanupJob::onComplete(SharedPtr<CommunicationWrapper> communicationWrapper)
{
    auto rtmCommunication = communicationWrapper->getRtmCommunication();
    if (!rtmCommunication
        || !rtmCommunication->hasV1())
    {
        String message = getHub()->getAllocator().make<String>("No response received.");
        EADPSDK_LOGE(getHub(), kSessionCleanupJobName, message);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::INVALID_SERVER_RESPONSE, message);
        SessionCleanupResponse::post(getHub(), kSessionCleanupJobName, m_callback, error);
        return;
    }

    auto v1 = rtmCommunication->getV1();
    if (v1->hasSuccess())
    {
        String message = getHub()->getAllocator().make<String>("Session Cleanup Successful");
        EADPSDK_LOGV(getHub(), kSessionCleanupJobName, message);
        SessionCleanupResponse::post(getHub(), kSessionCleanupJobName, m_callback, nullptr);
    }
    else
    {
        String message = v1->getError()->getErrorMessage();
        if (message.empty())
        {
            message = getHub()->getAllocator().make<String>("Error received from server in response to session cleanup.");
        }

        EADPSDK_LOGE(getHub(), kSessionCleanupJobName, message);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), v1->getError()->getErrorCode(), message);
        SessionCleanupResponse::post(getHub(), kSessionCleanupJobName, m_callback, error);
    }
}

void SessionCleanupJob::onSendError(const eadp::foundation::ErrorPtr& error)
{
    SessionCleanupResponse::post(getHub(), kSessionCleanupJobName, m_callback, error);
}
