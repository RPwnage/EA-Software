// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#include "ChatInitiateRequestJob.h"
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/realtimemessaging/RealTimeMessagingError.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope;

using ChatInitiateResponse = ResponseT<SharedPtr<rtm::protocol::ChatInitiateSuccessV1>, const ErrorPtr&>;
constexpr char kChatInitiateResponseJobName[] = "ChatInitiateResponseJob";

ChatInitiateRequestJob::ChatInitiateRequestJob(IHub* hub,
    SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService,
    const eadp::foundation::SharedPtr<rtm::protocol::ChatInitiateV1> chatInitiateRequest,
    ChatService::InitiateChatCallback initiateChatCallback) 
: RTMRequestJob(hub, rtmService)
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
        EADPSDK_LOGE(getHub(), "ChatInitiateRequest", message);
        auto error = FoundationError::create(getHub()->getAllocator(), FoundationError::Code::NOT_INITIALIZED, message);

        ChatInitiateResponse::post(getHub(), kChatInitiateResponseJobName, m_callback, nullptr, error);
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
    EADPSDK_LOGE(getHub(), "ChatInitiateRequest", message);
    auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::SOCKET_REQUEST_TIMEOUT, message);

    ChatInitiateResponse::post(getHub(), kChatInitiateResponseJobName, m_callback, nullptr, error);
}

void ChatInitiateRequestJob::onComplete(SharedPtr<CommunicationWrapper> communicationWrapper)
{
    if (communicationWrapper->hasRtmCommunication())
    {
        SharedPtr<rtm::protocol::Communication> communication = communicationWrapper->getRtmCommunication();
        if (communication->hasV1())
        {
            if (communication->getV1()->hasSuccess()
                && communication->getV1()->getSuccess()->hasChatInitiateSuccess())
            {
                EADPSDK_LOGV(getHub(), "ChatInitiateRequest", "Chat Initiate Request sent successfully");
                ChatInitiateResponse::post(getHub(), kChatInitiateResponseJobName, m_callback, communication->getV1()->getSuccess()->getChatInitiateSuccess(), nullptr);
            }
            else if (communication->getV1()->hasError())
            {
                String message = communication->getV1()->getError()->getErrorMessage();
                if (message.empty())
                {
                    message = getHub()->getAllocator().make<String>("Error received from server in response to ChatInitiate request.");
                }

                EADPSDK_LOGE(getHub(), "ChatInitiateRequest", message);

                auto error = RealTimeMessagingError::create(getHub()->getAllocator(), communication->getV1()->getError()->getErrorCode(), message);

                ChatInitiateResponse::post(getHub(), kChatInitiateResponseJobName, m_callback, nullptr, error);
            }
            return;
        }
    }

    String message = getHub()->getAllocator().make<String>("ChatInitiate Response message received is of incorrect type.");
    EADPSDK_LOGE(getHub(), "ChatInitiateRequest", message);
    auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::INVALID_SERVER_RESPONSE, message);

    ChatInitiateResponse::post(getHub(), kChatInitiateResponseJobName, m_callback, nullptr, error);

}

void ChatInitiateRequestJob::onSendError(const eadp::foundation::ErrorPtr& error)
{
    ChatInitiateResponse::post(getHub(), kChatInitiateResponseJobName, m_callback, nullptr, error);
}
