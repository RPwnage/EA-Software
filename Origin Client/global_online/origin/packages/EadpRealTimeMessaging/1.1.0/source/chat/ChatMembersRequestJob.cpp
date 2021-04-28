// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#include "ChatMembersRequestJob.h"
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/realtimemessaging/RealTimeMessagingError.h>
#include <eadp/foundation/internal/Utils.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope;

namespace
{
using ChatMembersResponse = ResponseT<SharedPtr<rtm::protocol::ChatMembersV1>, const ErrorPtr&>;
constexpr char kChatMembersRequestJobName[] = "ChatMembersRequestJob";
}

ChatMembersRequestJob::ChatMembersRequestJob(IHub* hub,
                                             SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService,
                                             SharedPtr<rtm::protocol::ChatMembersRequestV1> chatMembersRequest,
                                             ChatService::ChatMembersCallback callback) 
: RTMRequestJob(hub, EADPSDK_MOVE(rtmService))
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
        EADPSDK_LOGE(getHub(), kChatMembersRequestJobName, message);
        auto error = FoundationError::create(getHub()->getAllocator(), FoundationError::Code::NOT_INITIALIZED, message);

        ChatMembersResponse::post(getHub(), kChatMembersRequestJobName, m_callback, nullptr, error);
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
    EADPSDK_LOGE(getHub(), kChatMembersRequestJobName, message);
    auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::SOCKET_REQUEST_TIMEOUT, message);

    ChatMembersResponse::post(getHub(), kChatMembersRequestJobName, m_callback, nullptr, error);
}

void ChatMembersRequestJob::onComplete(SharedPtr<CommunicationWrapper> communicationWrapper)
{
    auto rtmCommunication = communicationWrapper->getRtmCommunication();
    if (!rtmCommunication
        || !rtmCommunication->hasV1()
        // Must return either chat members or error response
        || (!rtmCommunication->getV1()->hasChatMembers() && !rtmCommunication->getV1()->hasError()))
    {
        String message = getHub()->getAllocator().make<String>("Chat Members Response message received is of incorrect type.");
        EADPSDK_LOGE(getHub(), kChatMembersRequestJobName, message);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::INVALID_SERVER_RESPONSE, message);
        ChatMembersResponse::post(getHub(), kChatMembersRequestJobName, m_callback, nullptr, error);
        return;
    }

    auto v1 = rtmCommunication->getV1();
    if (v1->hasChatMembers())
    {
        EADPSDK_LOGV(getHub(), kChatMembersRequestJobName, "Chat Members successful");
        ChatMembersResponse::post(getHub(), kChatMembersRequestJobName, m_callback, v1->getChatMembers(), nullptr);
    }
    else
    {
        String message = getHub()->getAllocator().make<String>("Error received from server in response to Chat Members request.");
        if (v1->getError()->getErrorCode().length() > 0)
        {
            eadp::foundation::Internal::Utils::appendSprintf(message, "\nErrorCode: %s", v1->getError()->getErrorCode().c_str());
        }
        if (v1->getError()->getErrorMessage().length() > 0)
        {
            eadp::foundation::Internal::Utils::appendSprintf(message, "\nReason: %s", v1->getError()->getErrorMessage().c_str());
        }

        EADPSDK_LOGE(getHub(), kChatMembersRequestJobName, message);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), v1->getError()->getErrorCode(), message);
        ChatMembersResponse::post(getHub(), kChatMembersRequestJobName, m_callback, nullptr, error);
    }
}

void ChatMembersRequestJob::onSendError(const eadp::foundation::ErrorPtr& error)
{
    ChatMembersResponse::post(getHub(), kChatMembersRequestJobName, m_callback, nullptr, error);
}
