// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#include "RTMCommunicationJob.h"

using namespace eadp::realtimemessaging;
using namespace eadp::foundation;

RTMCommunicationJob::RTMCommunicationJob(IHub* hub, SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService)
: m_rtmService(EADPSDK_MOVE(rtmService))
, m_hub(hub)
, m_persistObj(hub->getAllocator().makeShared<Callback::Persistence>())
, m_executed(false)
{
}

void RTMCommunicationJob::execute()
{
    if (m_executed || m_done) return; 

    // RTM Service is empty
    if (!m_rtmService)
    {
        m_done = true;
        onSendError(FoundationError::create(m_hub->getAllocator(), FoundationError::Code::NOT_INITIALIZED, "RTMService object is empty. Please ensure ConnectionService was initialized"));
        return;
    }

    // Setup Request
    setupOutgoingCommunication();

    // Did the request setup mark the job as done?
    if (m_done)
    {
        return;
    }

    // Communication not setup
    if (!m_communication)
    {
        m_done = true;
        onSendError(FoundationError::create(m_hub->getAllocator(), FoundationError::Code::NOT_INITIALIZED, "Outgoing Communication is not initialized"));
        return;
    }

    auto onSendErrorCallback = [this](const ErrorPtr& error)
    {
        m_done = true;
        onSendError(error);
    };

    // Send Message without an expected response
    m_rtmService->sendCommunication(m_communication, eadp::realtimemessaging::Internal::RTMService::RTMErrorCallback(onSendErrorCallback, m_persistObj, Callback::kIdleInvokeGroupId));
    m_executed = true;
}

SharedPtr<CommunicationWrapper> RTMCommunicationJob::getRequestCommunication()
{
    return m_communication;
}

IHub* RTMCommunicationJob::getHub()
{
    return m_hub;
}
