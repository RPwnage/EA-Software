// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#include "ChatInitiateRequestJob.h"
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/realtimemessaging/RealTimeMessagingError.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope;

namespace
{
using ChatInitiateResponse = ResponseT<SharedPtr<rtm::protocol::ChatInitiateSuccessV1>, const ErrorPtr&>;
constexpr char kChatInitiateRequestJobName[] = "ChatInitiateRequestJob";
}

ChatInitiateRequestJob::ChatInitiateRequestJob(IHub* hub,
                                               SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService,
                                               const eadp::foundation::SharedPtr<rtm::protocol::ChatInitiateV1> chatInitiateRequest,
                                               ChatService::InitiateChatCallback initiateChatCallback) 
: RTMRequestJob(hub, EADPSDK_MOVE(rtmService))
, m_callback(initiateChatCallback)
, m_chatInitiateV1(chatInitiateRequest)
{
}

void ChatInitiateRequestJob::setupOutgoingCommunication(const eadp::foundation::String& requestId)
{
    if (!m_chatInitiateV1)
    {
        m_done = true;      // Mark current job done

        String message = getHub()->getAllocator().make<String>("Request not set or empty");
        EADPSDK_LOGE(getHub(), kChatInitiateRequestJobName, message);
        auto error = FoundationError::create(getHub()->getAllocator(), FoundationError::Code::INVALID_ARGUMENT, message);

        ChatInitiateResponse::post(getHub(), kChatInitiateRequestJobName, m_callback, nullptr, error);
        return;
    }

    SharedPtr<rtm::protocol::CommunicationV1> v1 = getHub()->getAllocator().makeShared<rtm::protocol::CommunicationV1>(getHub()->getAllocator());
    v1->setRequestId(requestId);
    v1->setChatInitiate(m_chatInitiateV1);

    SharedPtr<rtm::protocol::Communication> communication = getHub()->getAllocator().makeShared<rtm::protocol::Communication>(getHub()->getAllocator());
    communication->setV1(v1);

    m_communication = getHub()->getAllocator().makeShared<CommunicationWrapper>(communication);
}

void ChatInitiateRequestJob::onTimeout()
{
    String message = getHub()->getAllocator().make<String>("Chat Initiate Request timed out.");
    EADPSDK_LOGE(getHub(), kChatInitiateRequestJobName, message);
    auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::SOCKET_REQUEST_TIMEOUT, message);

    ChatInitiateResponse::post(getHub(), kChatInitiateRequestJobName, m_callback, nullptr, error);
}

void ChatInitiateRequestJob::onComplete(SharedPtr<CommunicationWrapper> communicationWrapper)
{
    auto rtmCommunication = communicationWrapper->getRtmCommunication();
    if (!rtmCommunication
        || !rtmCommunication->hasV1()
        // Must return either chat initiate success or error response
        || (!rtmCommunication->getV1()->hasError()
            && !rtmCommunication->getV1()->hasSuccess()
            && !rtmCommunication->getV1()->getSuccess()->hasChatInitiateSuccess()))
    {
        String message = getHub()->getAllocator().make<String>("ChatInitiate Response message received is of incorrect type.");
        EADPSDK_LOGE(getHub(), kChatInitiateRequestJobName, message);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::INVALID_SERVER_RESPONSE, message);
        ChatInitiateResponse::post(getHub(), kChatInitiateRequestJobName, m_callback, nullptr, error);
        return;
    }

    auto v1 = rtmCommunication->getV1();
    if (v1->hasSuccess() && v1->getSuccess()->hasChatInitiateSuccess())
    {
        EADPSDK_LOGV(getHub(), kChatInitiateRequestJobName, "Chat Initiate Request sent successfully");
        ChatInitiateResponse::post(getHub(), kChatInitiateRequestJobName, m_callback, v1->getSuccess()->getChatInitiateSuccess(), nullptr);
    }
    else
    {
        String message = v1->getError()->getErrorMessage();
        if (message.empty())
        {
            message = getHub()->getAllocator().make<String>("Error received from server in response to ChatInitiate request.");
        }

        EADPSDK_LOGE(getHub(), kChatInitiateRequestJobName, message);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), v1->getError()->getErrorCode(), message);
        ChatInitiateResponse::post(getHub(), kChatInitiateRequestJobName, m_callback, nullptr, error);
    }
}

void ChatInitiateRequestJob::onSendError(const eadp::foundation::ErrorPtr& error)
{
    ChatInitiateResponse::post(getHub(), kChatInitiateRequestJobName, m_callback, nullptr, error);
}
