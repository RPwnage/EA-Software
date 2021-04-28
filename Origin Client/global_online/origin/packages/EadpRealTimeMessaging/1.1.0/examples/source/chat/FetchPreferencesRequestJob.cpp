// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#include "FetchPreferencesRequestJob.h"
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/realtimemessaging/RealTimeMessagingError.h>
#include <eadp/foundation/internal/Utils.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope;

namespace
{
using FetchPreferencesResponse = ResponseT<const SharedPtr<rtm::protocol::GetPreferenceResponseV1>, const ErrorPtr&>;
constexpr char kFetchPreferencesRequestJobName[] = "FetchPreferencesRequestJob";
}

FetchPreferencesRequestJob::FetchPreferencesRequestJob(IHub* hub,
                                                       SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService,
                                                       ChatService::PreferenceCallback callback) 
: RTMRequestJob(hub, EADPSDK_MOVE(rtmService))
, m_callback(callback)
{
}

void FetchPreferencesRequestJob::setupOutgoingCommunication(const eadp::foundation::String& requestId)
{
    if (m_callback.isEmpty())
    {
        m_done = true;      // Mark current job done

        return;
    }

    SharedPtr<rtm::protocol::GetPreferenceRequestV1> getPreferenceRequestV1 = getHub()->getAllocator().makeShared<rtm::protocol::GetPreferenceRequestV1>(getHub()->getAllocator());

    SharedPtr<rtm::protocol::CommunicationV1> v1 = getHub()->getAllocator().makeShared<rtm::protocol::CommunicationV1>(getHub()->getAllocator());
    v1->setRequestId(requestId);
    v1->setGetPreferenceRequest(getPreferenceRequestV1);

    SharedPtr<rtm::protocol::Communication> communication = getHub()->getAllocator().makeShared<rtm::protocol::Communication>(getHub()->getAllocator());
    communication->setV1(v1);

    m_communication = getHub()->getAllocator().makeShared<CommunicationWrapper>(communication);
}

void FetchPreferencesRequestJob::onTimeout()
{
    String message = getHub()->getAllocator().make<String>("Fetch Preferences Request timed out.");
    EADPSDK_LOGE(getHub(), kFetchPreferencesRequestJobName, message);
    auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::SOCKET_REQUEST_TIMEOUT, message);

    FetchPreferencesResponse::post(getHub(), kFetchPreferencesRequestJobName, m_callback, nullptr, error);
}

void FetchPreferencesRequestJob::onComplete(SharedPtr<CommunicationWrapper> communicationWrapper)
{
    auto rtmCommunication = communicationWrapper->getRtmCommunication();
    if (!rtmCommunication
        || !rtmCommunication->hasV1()
        // Must return either preferences or error response
        || (!rtmCommunication->getV1()->hasGetPreferenceResponse() && !rtmCommunication->getV1()->hasError()))
    {
        String message = getHub()->getAllocator().make<String>("Fetch Preferences Response message received is of incorrect type.");
        EADPSDK_LOGE(getHub(), kFetchPreferencesRequestJobName, message);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::INVALID_SERVER_RESPONSE, message);
        FetchPreferencesResponse::post(getHub(), kFetchPreferencesRequestJobName, m_callback, nullptr, error);
        return;
    }

    auto v1 = rtmCommunication->getV1();
    if (v1->hasGetPreferenceResponse())
    {
        EADPSDK_LOGV(getHub(), kFetchPreferencesRequestJobName, "Fetch Preferences successful");
        FetchPreferencesResponse::post(getHub(), kFetchPreferencesRequestJobName, m_callback, v1->getGetPreferenceResponse(), nullptr);
    }
    else
    {
        String message = getHub()->getAllocator().make<String>("Error received from server in response to Fetch Preferences request.");
        if (v1->getError()->getErrorCode().length() > 0)
        {
            eadp::foundation::Internal::Utils::appendSprintf(message, "\nErrorCode: %s", v1->getError()->getErrorCode().c_str());
        }
        if (v1->getError()->getErrorMessage().length() > 0)
        {
            eadp::foundation::Internal::Utils::appendSprintf(message, "\nReason: %s", v1->getError()->getErrorMessage().c_str());
        }

        EADPSDK_LOGE(getHub(), kFetchPreferencesRequestJobName, message);
        auto error = RealTimeMessagingError::create(getHub()->getAllocator(), v1->getError()->getErrorCode(), message);
        FetchPreferencesResponse::post(getHub(), kFetchPreferencesRequestJobName, m_callback, nullptr, error);
    }
}

void FetchPreferencesRequestJob::onSendError(const eadp::foundation::ErrorPtr& error)
{
    FetchPreferencesResponse::post(getHub(), kFetchPreferencesRequestJobName, m_callback, nullptr, error);
}
