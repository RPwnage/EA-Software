// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#include "DirectMessageRequestJob.h"
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/realtimemessaging/RealTimeMessagingError.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope;

using DirectMessageRequest = ResponseT<const ErrorPtr&>;
constexpr char kDirectMessageRequestJobName[] = "DirectMessageRequestJob";

DirectMessageRequestJob::DirectMessageRequestJob(IHub* hub,
    SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService,
    const eadp::foundation::SharedPtr<rtm::protocol::PointToPointMessageV1> message,
    ChatService::ErrorCallback errorCallback) 
: RTMRequestJob(hub, rtmService)
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
        EADPSDK_LOGE(getHub(), "DirectMessage", errorMessage);
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
    EADPSDK_LOGE(getHub(), "DirectMessage", errorMessage);
    auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::SOCKET_REQUEST_TIMEOUT, errorMessage);

    DirectMessageRequest::post(getHub(), kDirectMessageRequestJobName, m_callback, error);
}

void DirectMessageRequestJob::onComplete(SharedPtr<CommunicationWrapper> communicationWrapper)
{
    if (communicationWrapper->hasRtmCommunication())
    {
        SharedPtr<rtm::protocol::Communication> communication = communicationWrapper->getRtmCommunication();
        if (communication->hasV1())
        {
            if (communication->getV1()->hasSuccess())
            {
                EADPSDK_LOGV(getHub(), "DirectMessage", "Direct Message sent successfully");
                DirectMessageRequest::post(getHub(), kDirectMessageRequestJobName, m_callback, nullptr);
            }
            else if (communication->getV1()->hasError())
            {
                String errorMessage = communication->getV1()->getError()->getErrorMessage();
                if (errorMessage.empty())
                {
                    errorMessage = getHub()->getAllocator().make<String>("Error received from server in response to Direct Message.");
                }

                EADPSDK_LOGE(getHub(), "DirectMessage", errorMessage);

                auto error = RealTimeMessagingError::create(getHub()->getAllocator(), communication->getV1()->getError()->getErrorCode(), errorMessage);

                DirectMessageRequest::post(getHub(), kDirectMessageRequestJobName, m_callback, error);
            }
            return;
        }
    }

    String errorMessage = getHub()->getAllocator().make<String>("Direct Message response message received is of incorrect type.");
    EADPSDK_LOGE(getHub(), "DirectMessage", errorMessage);
    auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::INVALID_SERVER_RESPONSE, errorMessage);

    DirectMessageRequest::post(getHub(), kDirectMessageRequestJobName, m_callback, error);
}

void DirectMessageRequestJob::onSendError(const eadp::foundation::ErrorPtr& error)
{
    DirectMessageRequest::post(getHub(), kDirectMessageRequestJobName, m_callback, error);
}
