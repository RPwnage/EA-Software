// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#include "ChannelSubscribeRequestJob.h"
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/realtimemessaging/RealTimeMessagingError.h>
#include <eadp/foundation/internal/Utils.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope;

namespace
{
using ChannelSubscribeResponse = ResponseT<const ErrorPtr&>;
constexpr char kChannelSubscribeRequestJobName[] = "ChannelSubscribeRequestJob";
}

ChannelSubscribeRequestJob::ChannelSubscribeRequestJob(IHub* hub,
                                                       SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService,
                                                       const eadp::foundation::SharedPtr<protocol::SubscribeRequest> subscribeRequest,
                                                       ChatService::ErrorCallback errorCallback) 
: RTMRequestJob(hub, EADPSDK_MOVE(rtmService))
, m_callback(errorCallback)
, m_subscribeRequest(subscribeRequest)
{
}

void ChannelSubscribeRequestJob::setupOutgoingCommunication(const eadp::foundation::String& requestId)
{
    if (!m_subscribeRequest)
    {
        m_done = true;      // Mark current job done

        String message = getHub()->getAllocator().make<String>("Request not set or empty");
        EADPSDK_LOGE(getHub(), kChannelSubscribeRequestJobName, message);
        auto error = FoundationError::create(getHub()->getAllocator(), FoundationError::Code::INVALID_ARGUMENT, message);

        ChannelSubscribeResponse::post(getHub(), kChannelSubscribeRequestJobName, m_callback, error);
        return;
    }

    if (m_subscribeRequest->getChannelId().empty())
    {
        m_done = true;      // Mark current job done

        String message = getHub()->getAllocator().make<String>("ChannelId cannot be empty");
        EADPSDK_LOGE(getHub(), kChannelSubscribeRequestJobName, message);
        auto error = FoundationError::create(getHub()->getAllocator(), FoundationError::Code::INVALID_ARGUMENT, message);

        ChannelSubscribeResponse::post(getHub(), kChannelSubscribeRequestJobName, m_callback, error);
        return;
    }

    SharedPtr<protocol::Header> header = getHub()->getAllocator().makeShared<protocol::Header>(getHub()->getAllocator());
    header->setRequestId(requestId);
    header->setType(protocol::CommunicationType::SUBSCRIBE_REQUEST);

    SharedPtr<protocol::Communication> communication = getHub()->getAllocator().makeShared<protocol::Communication>(getHub()->getAllocator());
    communication->setHeader(header);
    communication->setSubscribeRequest(m_subscribeRequest);

    m_communication = getHub()->getAllocator().makeShared<CommunicationWrapper>(communication);
}

void ChannelSubscribeRequestJob::onTimeout()
{
    String message = getHub()->getAllocator().make<String>("Channel Subscribe Request timed out.");
    EADPSDK_LOGE(getHub(), kChannelSubscribeRequestJobName, message);
    auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::SOCKET_REQUEST_TIMEOUT, message);

    ChannelSubscribeResponse::post(getHub(), kChannelSubscribeRequestJobName, m_callback, error);
}

void ChannelSubscribeRequestJob::onComplete(SharedPtr<CommunicationWrapper> communicationWrapper)
{
    auto socialCommunication = communicationWrapper->getSocialCommunication();
    if (!socialCommunication
        || !socialCommunication->hasSubscribeResponse())
    {
        String message = getHub()->getAllocator().make<String>("Channel Subscribe Response message received is of incorrect type.");
        EADPSDK_LOGE(getHub(), kChannelSubscribeRequestJobName, message);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::INVALID_SERVER_RESPONSE, message);
        ChannelSubscribeResponse::post(getHub(), kChannelSubscribeRequestJobName, m_callback, error);
        return; 
    }

    auto subscribeResponse = socialCommunication->getSubscribeResponse();
    if (subscribeResponse->getSuccess())
    {
        EADPSDK_LOGV(getHub(), kChannelSubscribeRequestJobName, "Channel Subscribe successful");
        ChannelSubscribeResponse::post(getHub(), kChannelSubscribeRequestJobName, m_callback, nullptr);
    }
    else
    {
        String message = getHub()->getAllocator().make<String>("Error received from server in response to Channel Subscribe request.");
        if (subscribeResponse->getErrorCode().length() > 0)
        {
            eadp::foundation::Internal::Utils::appendSprintf(message, "\nErrorCode: %s", subscribeResponse->getErrorCode().c_str());
        }
        if (subscribeResponse->getErrorMessage().length() > 0)
        {
            eadp::foundation::Internal::Utils::appendSprintf(message, "\nReason: %s", subscribeResponse->getErrorMessage().c_str());
        }

        EADPSDK_LOGE(getHub(), kChannelSubscribeRequestJobName, message);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), subscribeResponse->getErrorCode(), message);
        ChannelSubscribeResponse::post(getHub(), kChannelSubscribeRequestJobName, m_callback, error);
    }
}

void ChannelSubscribeRequestJob::onSendError(const eadp::foundation::ErrorPtr& error)
{
    ChannelSubscribeResponse::post(getHub(), kChannelSubscribeRequestJobName, m_callback, error);
}
