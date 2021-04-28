// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#include "ChannelUnsubscribeRequestJob.h"
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/realtimemessaging/RealTimeMessagingError.h>
#include <eadp/foundation/internal/Utils.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope;

namespace
{
using ChannelUnsubscribeResponse = ResponseT<const ErrorPtr&>;
constexpr char kChannelUnsubscribeRequestJobName[] = "ChannelUnsubscribeRequestJob";
}

ChannelUnsubscribeRequestJob::ChannelUnsubscribeRequestJob(IHub* hub,
                                                           SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService,
                                                           const eadp::foundation::SharedPtr<protocol::UnsubscribeRequest> unsubscribeRequest,
                                                           ChatService::ErrorCallback errorCallback) 
: RTMRequestJob(hub, EADPSDK_MOVE(rtmService))
, m_callback(errorCallback)
, m_unsubscribeRequest(unsubscribeRequest)
{
}

void ChannelUnsubscribeRequestJob::setupOutgoingCommunication(const eadp::foundation::String& requestId)
{
    if (!m_unsubscribeRequest)
    {
        m_done = true;      // Mark current job done

        String message = getHub()->getAllocator().make<String>("Request not set or empty");
        EADPSDK_LOGE(getHub(), kChannelUnsubscribeRequestJobName, message);
        auto error = FoundationError::create(getHub()->getAllocator(), FoundationError::Code::INVALID_ARGUMENT, message);

        ChannelUnsubscribeResponse::post(getHub(), kChannelUnsubscribeRequestJobName, m_callback, error);
        return;
    }

    if (m_unsubscribeRequest->getChannelId().empty())
    {
        m_done = true;      // Mark current job done

        String message = getHub()->getAllocator().make<String>("ChannelId cannot be empty");
        EADPSDK_LOGE(getHub(), kChannelUnsubscribeRequestJobName, message);
        auto error = FoundationError::create(getHub()->getAllocator(), FoundationError::Code::INVALID_ARGUMENT, message);

        ChannelUnsubscribeResponse::post(getHub(), kChannelUnsubscribeRequestJobName, m_callback, error);
        return;
    }

    SharedPtr<protocol::Header> header = getHub()->getAllocator().makeShared<protocol::Header>(getHub()->getAllocator());
    header->setRequestId(requestId);
    header->setType(protocol::CommunicationType::UNSUBSCRIBE_REQUEST);

    SharedPtr<protocol::Communication> communication = getHub()->getAllocator().makeShared<protocol::Communication>(getHub()->getAllocator());
    communication->setHeader(header);
    communication->setUnsubscribeRequest(m_unsubscribeRequest);

    m_communication = getHub()->getAllocator().makeShared<CommunicationWrapper>(communication);
}

void ChannelUnsubscribeRequestJob::onTimeout()
{
    String message = getHub()->getAllocator().make<String>("Channel Unsubscribe Request timed out.");
    EADPSDK_LOGE(getHub(), kChannelUnsubscribeRequestJobName, message);
    auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::SOCKET_REQUEST_TIMEOUT, message);

    ChannelUnsubscribeResponse::post(getHub(), kChannelUnsubscribeRequestJobName, m_callback, error);
}

void ChannelUnsubscribeRequestJob::onComplete(SharedPtr<CommunicationWrapper> communicationWrapper)
{
    auto socialCommunication = communicationWrapper->getSocialCommunication();
    if (!socialCommunication
        || !socialCommunication->hasUnsubscribeResponse())
    {
        String message = getHub()->getAllocator().make<String>("Channel Unsubscribe Response message received is of incorrect type.");
        EADPSDK_LOGE(getHub(), kChannelUnsubscribeRequestJobName, message);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::INVALID_SERVER_RESPONSE, message);
        ChannelUnsubscribeResponse::post(getHub(), kChannelUnsubscribeRequestJobName, m_callback, error);
        return;
    }

    auto unsubscribeResponse = socialCommunication->getUnsubscribeResponse();
    if (unsubscribeResponse->getSuccess())
    {
        EADPSDK_LOGV(getHub(), kChannelUnsubscribeRequestJobName, "Channel Unsubscribe successful");
        ChannelUnsubscribeResponse::post(getHub(), kChannelUnsubscribeRequestJobName, m_callback, nullptr);
    }
    else
    {
        String message = getHub()->getAllocator().make<String>("Error received from server in response to Channel Unsubscribe request.");
        if (unsubscribeResponse->getErrorCode().length() > 0)
        {
            eadp::foundation::Internal::Utils::appendSprintf(message, "\nErrorCode: %s", unsubscribeResponse->getErrorCode().c_str());
        }
        if (unsubscribeResponse->getErrorMessage().length() > 0)
        {
            eadp::foundation::Internal::Utils::appendSprintf(message, "\nReason: %s", unsubscribeResponse->getErrorMessage().c_str());
        }

        EADPSDK_LOGE(getHub(), kChannelUnsubscribeRequestJobName, message);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), unsubscribeResponse->getErrorCode(), message);
        ChannelUnsubscribeResponse::post(getHub(), kChannelUnsubscribeRequestJobName, m_callback, error);
    }
}

void ChannelUnsubscribeRequestJob::onSendError(const eadp::foundation::ErrorPtr& error)
{
    ChannelUnsubscribeResponse::post(getHub(), kChannelUnsubscribeRequestJobName, m_callback, error);
}
