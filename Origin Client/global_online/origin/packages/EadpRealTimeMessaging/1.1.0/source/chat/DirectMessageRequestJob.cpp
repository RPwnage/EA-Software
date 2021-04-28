// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#include "DirectMessageRequestJob.h"
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/realtimemessaging/RealTimeMessagingError.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope;

namespace
{
using DirectMessageRequest = ResponseT<const ErrorPtr&>;
constexpr char kDirectMessageRequestJobName[] = "DirectMessageRequestJob";
}

DirectMessageRequestJob::DirectMessageRequestJob(IHub* hub,
                                                 SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService,
                                                 const eadp::foundation::SharedPtr<rtm::protocol::PointToPointMessageV1> message,
                                                 ChatService::ErrorCallback errorCallback) 
: RTMRequestJob(hub, EADPSDK_MOVE(rtmService))
, m_callback(errorCallback)
, m_message(message)
{
}

void DirectMessageRequestJob::setupOutgoingCommunication(const eadp::foundation::String& requestId)
{
    if (!m_message)
    {
        m_done = true;

        String errorMessage = getHub()->getAllocator().make<String>("Direct message not set or empty");
        EADPSDK_LOGE(getHub(), kDirectMessageRequestJobName, errorMessage);
        auto error = FoundationError::create(getHub()->getAllocator(), FoundationError::Code::INVALID_ARGUMENT, errorMessage);

        DirectMessageRequest::post(getHub(), kDirectMessageRequestJobName, m_callback, error);
        return;
    }

    SharedPtr<rtm::protocol::CommunicationV1> v1 = getHub()->getAllocator().makeShared<rtm::protocol::CommunicationV1>(getHub()->getAllocator());
    v1->setRequestId(requestId);
    v1->setPointToPointMessage(m_message);

    SharedPtr<rtm::protocol::Communication> communication = getHub()->getAllocator().makeShared<rtm::protocol::Communication>(getHub()->getAllocator());
    communication->setV1(v1);

    m_communication = getHub()->getAllocator().makeShared<CommunicationWrapper>(communication);
}

void DirectMessageRequestJob::onTimeout()
{
    String errorMessage = getHub()->getAllocator().make<String>("Direct Message timed out.");
    EADPSDK_LOGE(getHub(), kDirectMessageRequestJobName, errorMessage);
    auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::SOCKET_REQUEST_TIMEOUT, errorMessage);

    DirectMessageRequest::post(getHub(), kDirectMessageRequestJobName, m_callback, error);
}

void DirectMessageRequestJob::onComplete(SharedPtr<CommunicationWrapper> communicationWrapper)
{
    auto rtmCommunication = communicationWrapper->getRtmCommunication();
    if (!rtmCommunication
        || !rtmCommunication->hasV1()
        // Must return either success or error response
        || (!rtmCommunication->getV1()->hasSuccess() && !rtmCommunication->getV1()->hasError()))
    {
        String errorMessage = getHub()->getAllocator().make<String>("Direct Message response message received is of incorrect type.");
        EADPSDK_LOGE(getHub(), kDirectMessageRequestJobName, errorMessage);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::INVALID_SERVER_RESPONSE, errorMessage);
        DirectMessageRequest::post(getHub(), kDirectMessageRequestJobName, m_callback, error);
        return;
    }

    auto v1 = rtmCommunication->getV1();
    if (v1->hasSuccess())
    {
        EADPSDK_LOGV(getHub(), kDirectMessageRequestJobName, "Direct Message sent successfully");
        DirectMessageRequest::post(getHub(), kDirectMessageRequestJobName, m_callback, nullptr);
    }
    else
    {
        String errorMessage = v1->getError()->getErrorMessage();
        if (errorMessage.empty())
        {
            errorMessage = getHub()->getAllocator().make<String>("Error received from server in response to Direct Message.");
        }

        EADPSDK_LOGE(getHub(), kDirectMessageRequestJobName, errorMessage);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), v1->getError()->getErrorCode(), errorMessage);
        DirectMessageRequest::post(getHub(), kDirectMessageRequestJobName, m_callback, error);
    }
}

void DirectMessageRequestJob::onSendError(const eadp::foundation::ErrorPtr& error)
{
    DirectMessageRequest::post(getHub(), kDirectMessageRequestJobName, m_callback, error);
}
