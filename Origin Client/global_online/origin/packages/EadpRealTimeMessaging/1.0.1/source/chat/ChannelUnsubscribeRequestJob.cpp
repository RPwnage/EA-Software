// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#include "ChannelUnsubscribeRequestJob.h"
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/realtimemessaging/RealTimeMessagingError.h>
#include <eadp/foundation/internal/Utils.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope;

using ChannelUnsubscribeResponse = ResponseT<const ErrorPtr&>;
constexpr char kChannelUnsubscribeResponseJobName[] = "ChannelUnsubscribeResponseJob";

ChannelUnsubscribeRequestJob::ChannelUnsubscribeRequestJob(IHub* hub,
    SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService,
    const eadp::foundation::SharedPtr<protocol::UnsubscribeRequest> unsubscribeRequest,
    ChatService::ErrorCallback errorCallback) 
: RTMRequestJob(hub, rtmService)
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
        EADPSDK_LOGE(getHub(), "ChannelUnsubscribeRequest", message);
        auto error = FoundationError::create(getHub()->getAllocator(), FoundationError::Code::NOT_INITIALIZED, message);

        ChannelUnsubscribeResponse::post(getHub(), kChannelUnsubscribeResponseJobName, m_callback, error);
        return;
    }

    if (m_unsubscribeRequest->getChannelId().empty())
    {
        m_done = true;      // Mark current job done

        String message = getHub()->getAllocator().make<String>("ChannelId cannot be empty");
        EADPSDK_LOGE(getHub(), "ChannelUnsubscribeRequest", message);
        auto error = FoundationError::create(getHub()->getAllocator(), FoundationError::Code::INVALID_ARGUMENT, message);

        ChannelUnsubscribeResponse::post(getHub(), kChannelUnsubscribeResponseJobName, m_callback, error);
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
    EADPSDK_LOGE(getHub(), "ChannelUnsubscribeRequest", message);
    auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::SOCKET_REQUEST_TIMEOUT, message);

    ChannelUnsubscribeResponse::post(getHub(), kChannelUnsubscribeResponseJobName, m_callback, error);
}

void ChannelUnsubscribeRequestJob::onComplete(SharedPtr<CommunicationWrapper> communicationWrapper)
{
    if (communicationWrapper->hasSocialCommunication())
    {
        SharedPtr<protocol::Communication> communication = communicationWrapper->getSocialCommunication();
        if (communication->hasUnsubscribeResponse())
        {
            if (communication->getUnsubscribeResponse()->getSuccess())
            {
                EADPSDK_LOGV(getHub(), "ChannelUnsubscribeRequest", "Channel Unsubscribe successful");
                ChannelUnsubscribeResponse::post(getHub(), kChannelUnsubscribeResponseJobName, m_callback, nullptr);
            }
            else
            {
                String message = getHub()->getAllocator().make<String>("Error received from server in response to Channel Unsubscribe request.");
                if (communication->getUnsubscribeResponse()->getErrorCode().length() > 0)
                {
                    eadp::foundation::Internal::Utils::appendSprintf(message, "\nErrorCode: %s", communication->getUnsubscribeResponse()->getErrorCode().c_str());
                }
                if (communication->getUnsubscribeResponse()->getErrorMessage().length() > 0)
                {
                    eadp::foundation::Internal::Utils::appendSprintf(message, "\nReason: %s", communication->getUnsubscribeResponse()->getErrorMessage().c_str());
                }

                EADPSDK_LOGE(getHub(), "ChannelUnsubscribeRequest", message);

                auto error = RealTimeMessagingError::create(getHub()->getAllocator(), communication->getUnsubscribeResponse()->getErrorCode(), message);

                ChannelUnsubscribeResponse::post(getHub(), kChannelUnsubscribeResponseJobName, m_callback, error);
            }
            return;
        }
    }

    String message = getHub()->getAllocator().make<String>("Channel Unsubscribe Response message received is of incorrect type.");
    EADPSDK_LOGE(getHub(), "ChannelUnsubscribeRequest", message);
    auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::INVALID_SERVER_RESPONSE, message);

    ChannelUnsubscribeResponse::post(getHub(), kChannelUnsubscribeResponseJobName, m_callback, error);

}

void ChannelUnsubscribeRequestJob::onSendError(const eadp::foundation::ErrorPtr& error)
{
    ChannelUnsubscribeResponse::post(getHub(), kChannelUnsubscribeResponseJobName, m_callback, error);
}
