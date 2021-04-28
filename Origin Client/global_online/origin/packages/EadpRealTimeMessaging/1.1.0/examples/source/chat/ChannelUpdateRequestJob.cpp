// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#include "ChannelUpdateRequestJob.h"
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/realtimemessaging/RealTimeMessagingError.h>
#include <eadp/foundation/internal/Utils.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope;

namespace
{
constexpr char kChannelUpdateRequestJobName[] = "ChannelUpdateRequestJob";
}

ChannelUpdateRequestJob::ChannelUpdateRequestJob(IHub* hub,
                                                 SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService,
                                                 const eadp::foundation::SharedPtr<rtm::protocol::ChatChannelUpdateV1> channelUpdateRequest,
                                                 ChatService::ErrorCallback errorCallback) 
: RTMRequestJob(hub, EADPSDK_MOVE(rtmService))
, m_callback(errorCallback)
, m_channelUpdateRequest(channelUpdateRequest)
{
}

void ChannelUpdateRequestJob::setupOutgoingCommunication(const eadp::foundation::String& requestId)
{
    SharedPtr<rtm::protocol::CommunicationV1> v1 = getHub()->getAllocator().makeShared<rtm::protocol::CommunicationV1>(getHub()->getAllocator());
    v1->setRequestId(requestId);
    v1->setChatChannelUpdate(m_channelUpdateRequest);

    SharedPtr<rtm::protocol::Communication> communication = getHub()->getAllocator().makeShared<rtm::protocol::Communication>(getHub()->getAllocator());
    communication->setV1(v1);

    m_communication = getHub()->getAllocator().makeShared<CommunicationWrapper>(communication);
}

void ChannelUpdateRequestJob::onTimeout()
{
    String message = getHub()->getAllocator().make<String>("Channel Update Request timed out.");
    EADPSDK_LOGE(getHub(), kChannelUpdateRequestJobName, message);
    auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::SOCKET_REQUEST_TIMEOUT, message);
    ErrorResponse::post(getHub(), kChannelUpdateRequestJobName, m_callback, error);
}

void ChannelUpdateRequestJob::onComplete(SharedPtr<CommunicationWrapper> communicationWrapper)
{
    auto rtmCommunication = communicationWrapper->getRtmCommunication();
    if (!rtmCommunication
        || !rtmCommunication->hasV1()
        // Must return either success or error response
        || (!rtmCommunication->getV1()->hasSuccess() && !rtmCommunication->getV1()->hasError()))
    {
        String message = getHub()->getAllocator().make<String>("Channel Update Response message received is of incorrect type.");
        EADPSDK_LOGE(getHub(), kChannelUpdateRequestJobName, message);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::INVALID_SERVER_RESPONSE, message);
        ErrorResponse::post(getHub(), kChannelUpdateRequestJobName, m_callback, error);
        return;
    }

    auto v1 = rtmCommunication->getV1();
    if (v1->hasSuccess())
    {
        EADPSDK_LOGV(getHub(), kChannelUpdateRequestJobName, "Channel update successful");
        ErrorResponse::post(getHub(), kChannelUpdateRequestJobName, m_callback, nullptr);
    }
    else
    {
        String message = getHub()->getAllocator().make<String>("Error received from server in response to Channel Update request.");
        if (v1->getError()->getErrorCode().length() > 0)
        {
            eadp::foundation::Internal::Utils::appendSprintf(message, "\nErrorCode: %s", v1->getError()->getErrorCode().c_str());
        }
        if (v1->getError()->getErrorMessage().length() > 0)
        {
            eadp::foundation::Internal::Utils::appendSprintf(message, "\nReason: %s", v1->getError()->getErrorMessage().c_str());
        }

        EADPSDK_LOGE(getHub(), kChannelUpdateRequestJobName, message);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), v1->getError()->getErrorCode(), message);
        ErrorResponse::post(getHub(), kChannelUpdateRequestJobName, m_callback, error);
    }
}

void ChannelUpdateRequestJob::onSendError(const eadp::foundation::ErrorPtr& error)
{
    ErrorResponse::post(getHub(), kChannelUpdateRequestJobName, m_callback, error);
}
