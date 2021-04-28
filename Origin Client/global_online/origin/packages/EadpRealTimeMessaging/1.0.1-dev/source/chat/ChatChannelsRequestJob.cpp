// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#include "ChatChannelsRequestJob.h"
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/realtimemessaging/RealTimeMessagingError.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope;

using ChatChannelsResponse = ResponseT<SharedPtr<rtm::protocol::ChatChannelsV1>, const ErrorPtr&>;
constexpr char kChatChannelsResponseJobName[] = "ChatChannelsResponseJob";

ChatChannelsRequestJob::ChatChannelsRequestJob(IHub* hub,
    SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService,
    ChatService::ChatChannelsCallback channelsCallback) 
: RTMRequestJob(hub, rtmService)
, m_callback(channelsCallback)
{
}

void ChatChannelsRequestJob::setupOutgoingCommunication(const eadp::foundation::String& requestId)
{
    SharedPtr<rtm::protocol::ChatChannelsRequestV1> chatChannelsRequestV1 = getHub()->getAllocator().makeShared<rtm::protocol::ChatChannelsRequestV1>(getHub()->getAllocator());
    SharedPtr<rtm::protocol::CommunicationV1> v1 = getHub()->getAllocator().makeShared<rtm::protocol::CommunicationV1>(getHub()->getAllocator());
    v1->setRequestId(requestId);
    v1->setChatChannelsRequest(chatChannelsRequestV1);

    SharedPtr<rtm::protocol::Communication> communication = getHub()->getAllocator().makeShared<rtm::protocol::Communication>(getHub()->getAllocator());
    communication->setV1(v1);

    m_communication = getHub()->getAllocator().makeShared<CommunicationWrapper>(communication);
}


void ChatChannelsRequestJob::onTimeout()
{
    String message = getHub()->getAllocator().make<String>("Chat Channels Request timed out.");
    EADPSDK_LOGE(getHub(), "ChatChannelsRequestJob", message);
    auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::SOCKET_REQUEST_TIMEOUT, message);

    ChatChannelsResponse::post(getHub(), kChatChannelsResponseJobName, m_callback, nullptr, error);
}

void ChatChannelsRequestJob::onComplete(SharedPtr<CommunicationWrapper> communicationWrapper)
{
    if (communicationWrapper->hasRtmCommunication())
    {
        SharedPtr<rtm::protocol::Communication> communication = communicationWrapper->getRtmCommunication();
        if (communication->hasV1())
        {
            if (communication->getV1()->hasChatChannels())
            {
                EADPSDK_LOGV(getHub(), "ChatChannelsRequestJob", "Received ChatChannels successfully");
                ChatChannelsResponse::post(getHub(), kChatChannelsResponseJobName, m_callback, communication->getV1()->getChatChannels(), nullptr);
            }
            else if (communication->getV1()->hasError())
            {
                String message = communication->getV1()->getError()->getErrorMessage();
                if (message.empty())
                {
                    message = getHub()->getAllocator().make<String>("Error received from server in response to ChatChannels request.");
                }

                EADPSDK_LOGE(getHub(), "ChatChannelsRequestJob", message);

                auto error = RealTimeMessagingError::create(getHub()->getAllocator(), communication->getV1()->getError()->getErrorCode(), message);

                ChatChannelsResponse::post(getHub(), kChatChannelsResponseJobName, m_callback, nullptr, error);
            }
            return;
        }
    }

    String message = getHub()->getAllocator().make<String>("ChatChannels Response message received is of incorrect type.");
    EADPSDK_LOGE(getHub(), "ChatChannelsRequestJob", message);
    auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::INVALID_SERVER_RESPONSE, message);

    ChatChannelsResponse::post(getHub(), kChatChannelsResponseJobName, m_callback, nullptr, error);

}

void ChatChannelsRequestJob::onSendError(const eadp::foundation::ErrorPtr& error)
{
    ChatChannelsResponse::post(getHub(), kChatChannelsResponseJobName, m_callback, nullptr, error);
}
