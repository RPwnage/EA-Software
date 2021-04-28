// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#include "UpdateTranslationPreferenceRequestJob.h"
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/realtimemessaging/RealTimeMessagingError.h>
#include <eadp/foundation/internal/Utils.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope;

using UpdateTranslationPreferenceResponse = ResponseT<const ErrorPtr&>;
constexpr char kUpdateTranslationPreferenceResponseJobName[] = "UpdateTranslationPreferenceResponseJob";

UpdateTranslationPreferenceRequestJob::UpdateTranslationPreferenceRequestJob(IHub* hub,
    SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService,
    SharedPtr<rtm::protocol::TranslationPreferenceV1> translationPreference,
    ChatService::ErrorCallback callback) 
: RTMRequestJob(hub, rtmService)
, m_translationPreference(translationPreference)
, m_callback(callback)
{
}

void UpdateTranslationPreferenceRequestJob::setupOutgoingCommunication(const eadp::foundation::String& requestId)
{
    if (!m_translationPreference)
    {
        m_done = true;      // Mark current job done

        String message = getHub()->getAllocator().make<String>("Request not set or empty");
        EADPSDK_LOGE(getHub(), "UpdateTranslationPreferenceRequest", message);
        auto error = FoundationError::create(getHub()->getAllocator(), FoundationError::Code::NOT_INITIALIZED, message);

        UpdateTranslationPreferenceResponse::post(getHub(), kUpdateTranslationPreferenceResponseJobName, m_callback, error);
        return;
    }

    SharedPtr<rtm::protocol::UpdatePreferenceRequestV1> updatePreferenceRequestV1 = getHub()->getAllocator().makeShared<rtm::protocol::UpdatePreferenceRequestV1>(getHub()->getAllocator());
    updatePreferenceRequestV1->setTranslation(m_translationPreference);

    SharedPtr<rtm::protocol::CommunicationV1> v1 = getHub()->getAllocator().makeShared<rtm::protocol::CommunicationV1>(getHub()->getAllocator());
    v1->setRequestId(requestId);
    v1->setPreferenceRequest(updatePreferenceRequestV1);

    SharedPtr<rtm::protocol::Communication> communication = getHub()->getAllocator().makeShared<rtm::protocol::Communication>(getHub()->getAllocator());
    communication->setV1(v1);

    m_communication = getHub()->getAllocator().makeShared<CommunicationWrapper>(communication);
}


void UpdateTranslationPreferenceRequestJob::onTimeout()
{
    String message = getHub()->getAllocator().make<String>("Update Translation Preference Request timed out.");
    EADPSDK_LOGE(getHub(), "UpdateTranslationPreferenceRequest", message);
    auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::SOCKET_REQUEST_TIMEOUT, message);

    UpdateTranslationPreferenceResponse::post(getHub(), kUpdateTranslationPreferenceResponseJobName, m_callback, error);
}

void UpdateTranslationPreferenceRequestJob::onComplete(SharedPtr<CommunicationWrapper> communicationWrapper)
{
    if (communicationWrapper->hasRtmCommunication())
    {
        SharedPtr<rtm::protocol::Communication> communication = communicationWrapper->getRtmCommunication();
        if (communication->hasV1())
        {
            if (communication->getV1()->hasSuccess())
            {
                EADPSDK_LOGV(getHub(), "UpdateTranslationPreferenceRequest", "Update Translation Preference successful");
                UpdateTranslationPreferenceResponse::post(getHub(), kUpdateTranslationPreferenceResponseJobName, m_callback, nullptr);
            }
            else if (communication->getV1()->hasError())
            {
                String message = getHub()->getAllocator().make<String>("Error received from server in response to Update Translation Preference request.");
                if (communication->getV1()->getError()->getErrorCode().length() > 0)
                {
                    eadp::foundation::Internal::Utils::appendSprintf(message, "\nErrorCode: %s", communication->getV1()->getError()->getErrorCode().c_str());
                }
                if (communication->getV1()->getError()->getErrorMessage().length() > 0)
                {
                    eadp::foundation::Internal::Utils::appendSprintf(message, "\nReason: %s", communication->getV1()->getError()->getErrorMessage().c_str());
                }

                EADPSDK_LOGE(getHub(), "UpdateTranslationPreferenceRequest", message);

                auto error = RealTimeMessagingError::create(getHub()->getAllocator(), communication->getV1()->getError()->getErrorCode(), message);

                UpdateTranslationPreferenceResponse::post(getHub(), kUpdateTranslationPreferenceResponseJobName, m_callback, error);
            }
            return;
        }
    }


    String message = getHub()->getAllocator().make<String>("Update Translation Preference Response message received is of incorrect type.");
    EADPSDK_LOGE(getHub(), "UpdateTranslationPreferenceRequest", message);
    auto error = RealTimeMessagingError::create(getHub()->getAllocator(), RealTimeMessagingError::Code::INVALID_SERVER_RESPONSE, message);

    UpdateTranslationPreferenceResponse::post(getHub(), kUpdateTranslationPreferenceResponseJobName, m_callback, error);
}

void UpdateTranslationPreferenceRequestJob::onSendError(const eadp::foundation::ErrorPtr& error)
{
    UpdateTranslationPreferenceResponse::post(getHub(), kUpdateTranslationPreferenceResponseJobName, m_callback, error);
}
