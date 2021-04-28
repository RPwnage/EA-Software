// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#include "ChatMembersRequestJob.h"
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/realtimemessaging/RealTimeMessagingError.h>
#include <eadp/foundation/internal/Utils.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope;

using ChatMembersResponse = ResponseT<SharedPtr<rtm::protocol::ChatMembersV1>, const ErrorPtr&>;
constexpr char kChatMembersResponseJobName[] = "ChatMembersResponseJob";

ChatMembersRequestJob::ChatMembersRequestJob(IHub* hub,
    SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService,
    SharedPtr<rtm::protocol::ChatMembersRequestV1> chatMembersRequest,
    ChatService::ChatMembersCallback callback) 
: RTMRequestJob(hub, rtmService)
, m_chatMembersRequest(chatMembersRequest)
, m_callback(callback)
{
}

void ChatMembersRequestJob::setupOutgoingCommunication(const eadp::foundation::String& requestId)
{
    if (!m_chatMembersRequest)
    {
        m_done = true;      // Mark current job done

        String message = getHub()->getAllocator().make<String>("Request not set or empty");
        EADPSDK_LOGE(getHub(), "ChatMembersRequest", message);
        auto error = FoundationError::create(getHub()->getAllocator(), FoundationError::Code::NOT_INITIALIZED, message);

        ChatMembersResponse::post(getHub(), kChatMembersResponseJobName, m_callback, nullptr, error);
        return;
    }

    SharedPtr<rtm::protocol::CommunicationV1> v1 = getHub()->getAllocator().makeShared<rtm::protocol::CommunicationV1>(getHub()->getAllocator());
    v1->setRequestId(requestId);
    v1->setChatMembersRequest(m_chatMembersRequest);

    SharedPtr<rtm::protocol::Communication> communication = getHub()->getAllocator().makeShared<rtm::protocol::Communication>(getHub()->getAllocator());
    communication->setV1(v1);

    m_communication = getHub()->getAllocator().makeShared<CommunicationWrapper>(communication);
}


void ChatMembersRequestJob::onTimeout()
{
    String message = getHub()->getAllocator().make<String>("Chat Members Request timed out.");
    EADPSDK_LOGE(getHub(), "ChatMembersRequest", message);
    auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::SOCKET_REQUEST_TIMEOUT, message);

    ChatMembersResponse::post(getHub(), kChatMembersResponseJobName, m_callback, nullptr, error);
}

void ChatMembersRequestJob::onComplete(SharedPtr<CommunicationWrapper> communicationWrapper)
{
    if (communicationWrapper->hasRtmCommunication())
    {
        SharedPtr<rtm::protocol::Communication> communication = communicationWrapper->getRtmCommunication();
        if (communication->hasV1())
        {
            if (communication->getV1()->hasChatMembers())
            {
                EADPSDK_LOGV(getHub(), "ChatMembersRequest", "Chat Members successful");
                ChatMembersResponse::post(getHub(), kChatMembersResponseJobName, m_callback, communication->getV1()->getChatMembers(), nullptr);
            }
            else if (communication->getV1()->hasError())
            {
                String message = getHub()->getAllocator().make<String>("Error received from server in response to Chat Members request.");
                if (communication->getV1()->getError()->getErrorCode().length() > 0)
                {
                    eadp::foundation::Internal::Utils::appendSprintf(message, "\nErrorCode: %s", communication->getV1()->getError()->getErrorCode().c_str());
                }
                if (communication->getV1()->getError()->getErrorMessage().length() > 0)
                {
                    eadp::foundation::Internal::Utils::appendSprintf(message, "\nReason: %s", communication->getV1()->getError()->getErrorMessage().c_str());
                }

                EADPSDK_LOGE(getHub(), "ChatMembersRequest", message);

                auto error = RealTimeMessagingError::create(getHub()->getAllocator(), communication->getV1()->getError()->getErrorCode(), message);

                ChatMembersResponse::post(getHub(), kChatMembersResponseJobName, m_callback, nullptr, error);
            }
            return;
        }
    }


    String message = getHub()->getAllocator().make<String>("Chat Members Response message received is of incorrect type.");
    EADPSDK_LOGE(getHub(), "ChatMembersRequest", message);
    auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::INVALID_SERVER_RESPONSE, message);

    ChatMembersResponse::post(getHub(), kChatMembersResponseJobName, m_callback, nullptr, error);
}

void ChatMembersRequestJob::onSendError(const eadp::foundation::ErrorPtr& error)
{
    ChatMembersResponse::post(getHub(), kChatMembersResponseJobName, m_callback, nullptr, error);
}
