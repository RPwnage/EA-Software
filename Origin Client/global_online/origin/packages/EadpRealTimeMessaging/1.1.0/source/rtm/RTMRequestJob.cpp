// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#include "RTMRequestJob.h"
#include <eadp/foundation/internal/Timestamp.h>

using namespace eadp::realtimemessaging;
using namespace eadp::foundation;

RTMRequestJob::RTMRequestJob(IHub* hub, SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService)
: m_hub(hub)
, m_rtmService(EADPSDK_MOVE(rtmService))
, m_persistObj(hub->getAllocator().makeShared<Callback::Persistence>())
, m_expiration()
, m_requestId(hub->getAllocator().emptyString())
, m_state(State::PRE_SETUP)
, m_timeout(kDefaultTimeout)
, m_skipConnectivityCheck(false)
{
}

void RTMRequestJob::execute()
{
    switch (m_state)
    {
    case State::PRE_SETUP:
    {
        if (!m_rtmService)
        {
            m_done = true; // Mark current job complete

            auto error = FoundationError::create(m_hub->getAllocator(), FoundationError::Code::NOT_INITIALIZED, "RTMService object is empty. Please ensure ConnectionService was initialized");
            onSendError(error);
            return;
        }

        m_requestId = m_rtmService->generateRequestId();

        setupOutgoingCommunication(m_requestId);

        if (m_done)
        {
            return;
        }

        if (!m_communication)
        {
            m_done = true; // Mark current job complete
            auto error = FoundationError::create(m_hub->getAllocator(), FoundationError::Code::NOT_INITIALIZED, "Outgoing Communication is not initialized");
            onSendError(error);
            return;
        }

        // Set expiration time
        m_expiration = eadp::foundation::Internal::getTimestampForNow();
        m_expiration.AddTime(EA::StdC::Parameter::kParameterSecond, m_timeout);

        auto onCompleteCallback = [this](SharedPtr<CommunicationWrapper> responseCommunication)
        {
            m_done = true; // Mark current job complete
            onComplete(responseCommunication);
        };

        auto onSendErrorCallback = [this](const ErrorPtr& error)
        {
            // If there was an error while sending communication mark this job done and call onError()
            if (error)
            {
                m_done = true; // Mark current job complete
                m_rtmService->removeRequest(m_requestId);
                onSendError(error);
            }
        };

        // Send request
        m_rtmService->sendRequest(m_requestId,
            m_communication,
            eadp::realtimemessaging::Internal::RTMService::RTMRequestCompleteCallback(onCompleteCallback, m_persistObj, Callback::kIdleInvokeGroupId),
            eadp::realtimemessaging::Internal::RTMService::RTMErrorCallback(onSendErrorCallback, m_persistObj, Callback::kIdleInvokeGroupId),
            m_skipConnectivityCheck);
        m_state = State::AWAITING_RESPONSE;
        break;
    }
    case State::AWAITING_RESPONSE:
    {
        if (m_rtmService->getState() == eadp::realtimemessaging::Internal::RTMService::ConnectionState::DISCONNECTED)
        {
            // m_rtmService disconnected. Mark this job as done
            m_done = true;
            return;
        }

        Timestamp now = eadp::foundation::Internal::getTimestampForNow();
        if (now.GetSeconds() > m_expiration.GetSeconds())
        {
            // Request timed out
            m_done = true; // Mark current job complete
            m_rtmService->removeRequest(m_requestId);

            onTimeout();
        }
        break;
    }
    default:
    break;
    }
}

String RTMRequestJob::getRequestId()
{
    return m_requestId;
}

Timestamp RTMRequestJob::getExpirationTime()
{
    return m_expiration;
}

SharedPtr<CommunicationWrapper> RTMRequestJob::getRequestCommunication()
{
    return m_communication;
}

IHub* RTMRequestJob::getHub()
{
    return m_hub;
}

void RTMRequestJob::skipConnectivityCheck()
{
    m_skipConnectivityCheck = true;
}
