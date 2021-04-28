// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#include "ChatChannelsRequestJob.h"
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/realtimemessaging/RealTimeMessagingError.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope;

namespace
{
using ChatChannelsResponse = ResponseT<SharedPtr<rtm::protocol::ChatChannelsV1>, const ErrorPtr&>;
constexpr char kChatChannelsRequestJobName[] = "ChatChannelsRequestJob";
}

ChatChannelsRequestJob::ChatChannelsRequestJob(IHub* hub,
                                               SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService,
                                               SharedPtr<rtm::protocol::ChatChannelsRequestV2> chatChannelsRequestV2,
                                               ChatService::ChatChannelsCallback channelsCallback) 
: RTMRequestJob(hub, EADPSDK_MOVE(rtmService))
, m_callback(channelsCallback)
, m_chatChannelsRequestV2(chatChannelsRequestV2)
{
}

void ChatChannelsRequestJob::setupOutgoingCommunication(const eadp::foundation::String& requestId)
{
    SharedPtr<rtm::protocol::CommunicationV1> v1 = getHub()->getAllocator().makeShared<rtm::protocol::CommunicationV1>(getHub()->getAllocator());
    v1->setRequestId(requestId);

    if (m_chatChannelsRequestV2)
    {
        v1->setChatChannelsRequestV2(m_chatChannelsRequestV2); 
    }
    else
    {
        SharedPtr<rtm::protocol::ChatChannelsRequestV1> chatChannelsRequestV1 = getHub()->getAllocator().makeShared<rtm::protocol::ChatChannelsRequestV1>(getHub()->getAllocator());
        chatChannelsRequestV1->setIncludeLastMessageInfo(true);
        chatChannelsRequestV1->setIncludeUnreadMessageCount(true);
        v1->setChatChannelsRequest(chatChannelsRequestV1);
    }

    SharedPtr<rtm::protocol::Communication> communication = getHub()->getAllocator().makeShared<rtm::protocol::Communication>(getHub()->getAllocator());
    communication->setV1(v1);

    m_communication = getHub()->getAllocator().makeShared<CommunicationWrapper>(communication);
}

void ChatChannelsRequestJob::onTimeout()
{
    String message = getHub()->getAllocator().make<String>("Chat Channels Request timed out.");
    EADPSDK_LOGE(getHub(), kChatChannelsRequestJobName, message);
    auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::SOCKET_REQUEST_TIMEOUT, message);

    ChatChannelsResponse::post(getHub(), kChatChannelsRequestJobName, m_callback, nullptr, error);
}

void ChatChannelsRequestJob::onComplete(SharedPtr<CommunicationWrapper> communicationWrapper)
{
    auto rtmCommunication = communicationWrapper->getRtmCommunication();
    if (!rtmCommunication
        || !rtmCommunication->hasV1()
        // Must return either chat channels or error response
        || (!rtmCommunication->getV1()->hasChatChannels() && !rtmCommunication->getV1()->hasError()))
    {
        String message = getHub()->getAllocator().make<String>("ChatChannels Response message received is of incorrect type.");
        EADPSDK_LOGE(getHub(), kChatChannelsRequestJobName, message);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::INVALID_SERVER_RESPONSE, message);
        ChatChannelsResponse::post(getHub(), kChatChannelsRequestJobName, m_callback, nullptr, error);
        return;
    }

    auto v1 = rtmCommunication->getV1();
    if (v1->hasChatChannels())
    {
        EADPSDK_LOGV(getHub(), kChatChannelsRequestJobName, "Received ChatChannels successfully");
        ChatChannelsResponse::post(getHub(), kChatChannelsRequestJobName, m_callback, v1->getChatChannels(), nullptr);
    }
    else
    {
        String message = v1->getError()->getErrorMessage();
        if (message.empty())
        {
            message = getHub()->getAllocator().make<String>("Error received from server in response to ChatChannels request.");
        }

        EADPSDK_LOGE(getHub(), kChatChannelsRequestJobName, message);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), v1->getError()->getErrorCode(), message);
        ChatChannelsResponse::post(getHub(), kChatChannelsRequestJobName, m_callback, nullptr, error);
    }
}

void ChatChannelsRequestJob::onSendError(const eadp::foundation::ErrorPtr& error)
{
    ChatChannelsResponse::post(getHub(), kChatChannelsRequestJobName, m_callback, nullptr, error);
}
