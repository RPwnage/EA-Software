// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#include "ChatLeaveRequestJob.h"
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/realtimemessaging/RealTimeMessagingError.h>
#include <eadp/foundation/internal/Utils.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope;

namespace
{
using ChatLeaveResponse = ResponseT<const ErrorPtr&>;
constexpr char kChatLeaveRequestJobName[] = "ChatLeaveRequestJob";
}

ChatLeaveRequestJob::ChatLeaveRequestJob(IHub* hub,
                                         SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService,
                                         const SharedPtr<rtm::protocol::ChatLeaveV1> chatLeaveRequest,
                                         ChatService::ErrorCallback callback) 
: RTMRequestJob(hub, EADPSDK_MOVE(rtmService))
, m_chatLeaveRequest(chatLeaveRequest)
, m_callback(callback)
{
}

void ChatLeaveRequestJob::setupOutgoingCommunication(const eadp::foundation::String& requestId)
{
    if (!m_chatLeaveRequest)
    {
        m_done = true;      // Mark current job done

        String message = getHub()->getAllocator().make<String>("Request not set or empty");
        EADPSDK_LOGE(getHub(), kChatLeaveRequestJobName, message);
        auto error = FoundationError::create(getHub()->getAllocator(), FoundationError::Code::INVALID_ARGUMENT, message);

        ChatLeaveResponse::post(getHub(), kChatLeaveRequestJobName, m_callback, error);
        return;
    }

    SharedPtr<rtm::protocol::CommunicationV1> v1 = getHub()->getAllocator().makeShared<rtm::protocol::CommunicationV1>(getHub()->getAllocator());
    v1->setRequestId(requestId);
    v1->setChatLeave(m_chatLeaveRequest);

    SharedPtr<rtm::protocol::Communication> communication = getHub()->getAllocator().makeShared<rtm::protocol::Communication>(getHub()->getAllocator());
    communication->setV1(v1);

    m_communication = getHub()->getAllocator().makeShared<CommunicationWrapper>(communication);
}

void ChatLeaveRequestJob::onTimeout()
{
    String message = getHub()->getAllocator().make<String>("Chat Leave Request timed out.");
    EADPSDK_LOGE(getHub(), kChatLeaveRequestJobName, message);
    auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::SOCKET_REQUEST_TIMEOUT, message);

    ChatLeaveResponse::post(getHub(), kChatLeaveRequestJobName, m_callback, error);
}

void ChatLeaveRequestJob::onComplete(SharedPtr<CommunicationWrapper> communicationWrapper)
{
    auto rtmCommunication = communicationWrapper->getRtmCommunication();
    if (!rtmCommunication
        || !rtmCommunication->hasV1()
        // Must return either success or error response
        || (!rtmCommunication->getV1()->hasSuccess() && !rtmCommunication->getV1()->hasError()))
    {
        String message = getHub()->getAllocator().make<String>("Chat Leave Response message received is of incorrect type.");
        EADPSDK_LOGE(getHub(), kChatLeaveRequestJobName, message);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::INVALID_SERVER_RESPONSE, message);
        ChatLeaveResponse::post(getHub(), kChatLeaveRequestJobName, m_callback, error);
        return;
    }

    auto v1 = rtmCommunication->getV1();
    if (v1->hasSuccess())
    {
        EADPSDK_LOGV(getHub(), kChatLeaveRequestJobName, "Chat Leave successful");
        ChatLeaveResponse::post(getHub(), kChatLeaveRequestJobName, m_callback, nullptr);
    }
    else
    {
        String message = getHub()->getAllocator().make<String>("Error received from server in response to Chat Leave request.");
        if (v1->getError()->getErrorCode().length() > 0)
        {
            eadp::foundation::Internal::Utils::appendSprintf(message, "\nErrorCode: %s", v1->getError()->getErrorCode().c_str());
        }
        if (v1->getError()->getErrorMessage().length() > 0)
        {
            eadp::foundation::Internal::Utils::appendSprintf(message, "\nReason: %s", v1->getError()->getErrorMessage().c_str());
        }

        EADPSDK_LOGE(getHub(), kChatLeaveRequestJobName, message);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), v1->getError()->getErrorCode(), message);
        ChatLeaveResponse::post(getHub(), kChatLeaveRequestJobName, m_callback, error);
    }
}

void ChatLeaveRequestJob::onSendError(const eadp::foundation::ErrorPtr& error)
{
    ChatLeaveResponse::post(getHub(), kChatLeaveRequestJobName, m_callback, error);
}
