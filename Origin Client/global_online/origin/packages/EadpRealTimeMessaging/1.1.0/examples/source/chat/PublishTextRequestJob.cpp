// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#include "PublishTextRequestJob.h"
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/realtimemessaging/RealTimeMessagingError.h>
#include <eadp/foundation/internal/Utils.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope;

namespace
{
using PublishTextResponse = ResponseT<const ErrorPtr&>;
constexpr char kPublishTextRequestJobName[] = "PublishTextRequestJob";
}

PublishTextRequestJob::PublishTextRequestJob(IHub* hub,
                                             SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService,
                                             const eadp::foundation::SharedPtr<protocol::PublishTextRequest> publishTextRequest,
                                             ChatService::ErrorCallback errorCallback) 
: RTMRequestJob(hub, EADPSDK_MOVE(rtmService))
, m_callback(errorCallback)
, m_publishTextRequest(publishTextRequest)
{
}

void PublishTextRequestJob::setupOutgoingCommunication(const eadp::foundation::String& requestId)
{
    if (!m_publishTextRequest)
    {
        m_done = true;      // Mark current job done

        String message = getHub()->getAllocator().make<String>("Request not set or empty");
        EADPSDK_LOGE(getHub(), kPublishTextRequestJobName, message);
        auto error = FoundationError::create(getHub()->getAllocator(), FoundationError::Code::INVALID_ARGUMENT, message);

        PublishTextResponse::post(getHub(), kPublishTextRequestJobName, m_callback, error);
        return;
    }

    if (m_publishTextRequest->getChannelId().empty())
    {
        m_done = true;      // Mark current job done

        String message = getHub()->getAllocator().make<String>("ChannelId cannot be empty");
        EADPSDK_LOGE(getHub(), kPublishTextRequestJobName, message);
        auto error = FoundationError::create(getHub()->getAllocator(), FoundationError::Code::INVALID_ARGUMENT, message);

        PublishTextResponse::post(getHub(), kPublishTextRequestJobName, m_callback, error);
        return;
    }

    SharedPtr<protocol::Header> header = getHub()->getAllocator().makeShared<protocol::Header>(getHub()->getAllocator());
    header->setRequestId(requestId);
    header->setType(protocol::CommunicationType::PUBLISH_TEXT_REQUEST);

    SharedPtr<protocol::Communication> communication = getHub()->getAllocator().makeShared<protocol::Communication>(getHub()->getAllocator());
    communication->setHeader(header);
    communication->setPublishTextRequest(m_publishTextRequest);

    m_communication = getHub()->getAllocator().makeShared<CommunicationWrapper>(communication);
}

void PublishTextRequestJob::onTimeout()
{
    String message = getHub()->getAllocator().make<String>("Publish Text Request timed out.");
    EADPSDK_LOGE(getHub(), kPublishTextRequestJobName, message);
    auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::SOCKET_REQUEST_TIMEOUT, message);

    PublishTextResponse::post(getHub(), kPublishTextRequestJobName, m_callback, error);
}

void PublishTextRequestJob::onComplete(SharedPtr<CommunicationWrapper> communicationWrapper)
{
    auto socialCommunication = communicationWrapper->getSocialCommunication();
    if (!socialCommunication
        || !socialCommunication->hasPublishResponse())
    {
        String message = getHub()->getAllocator().make<String>("Publish Text Response message received is of incorrect type.");
        EADPSDK_LOGE(getHub(), kPublishTextRequestJobName, message);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::INVALID_SERVER_RESPONSE, message);
        PublishTextResponse::post(getHub(), kPublishTextRequestJobName, m_callback, error);
        return;
    }

    auto publishResponse = socialCommunication->getPublishResponse();
    if (publishResponse->getSuccess())
    {
        EADPSDK_LOGV(getHub(), kPublishTextRequestJobName, "Publish Text successful");
        PublishTextResponse::post(getHub(), kPublishTextRequestJobName, m_callback, nullptr);
    }
    else
    {
        String message = getHub()->getAllocator().make<String>("Error received from server in response to Publish Text request.");
        if (publishResponse->getErrorCode().length() > 0)
        {
            eadp::foundation::Internal::Utils::appendSprintf(message, "\nErrorCode: %s", publishResponse->getErrorCode().c_str());
        }
        if (publishResponse->getErrorMessage().length() > 0)
        {
            eadp::foundation::Internal::Utils::appendSprintf(message, "\nReason: %s", publishResponse->getErrorMessage().c_str());
        }

        EADPSDK_LOGE(getHub(), kPublishTextRequestJobName, message);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), publishResponse->getErrorCode(), message);
        PublishTextResponse::post(getHub(), kPublishTextRequestJobName, m_callback, error);
    }
}

void PublishTextRequestJob::onSendError(const eadp::foundation::ErrorPtr& error)
{
    PublishTextResponse::post(getHub(), kPublishTextRequestJobName, m_callback, error);
}
