// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#include "TypingIndicatorCommunicationJob.h"
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/realtimemessaging/RealTimeMessagingError.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope;

namespace
{
constexpr char kTypingIndicatorJobName[] = "TypingIndicatorCommunicationJob";
}

TypingIndicatorCommunicationJob::TypingIndicatorCommunicationJob(IHub* hub,
                                                                 SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService,
                                                                 const SharedPtr<rtm::protocol::ChatTypingEventRequestV1> request,
                                                                 ChatService::ErrorCallback errorCallback)
: RTMCommunicationJob(hub, EADPSDK_MOVE(rtmService))
, m_callback(EADPSDK_MOVE(errorCallback))
, m_request(EADPSDK_MOVE(request))
{
}

void TypingIndicatorCommunicationJob::setupOutgoingCommunication()
{
    Allocator& allocator = getHub()->getAllocator();

    auto v1 = allocator.makeShared<rtm::protocol::CommunicationV1>(allocator);
    v1->setRequestId(m_rtmService->generateRequestId());
    v1->setChatTypingEventRequest(m_request);

    SharedPtr<rtm::protocol::Communication> communication = allocator.makeShared<rtm::protocol::Communication>(allocator);
    communication->setV1(v1);

    m_communication = allocator.makeShared<CommunicationWrapper>(communication);
}

void TypingIndicatorCommunicationJob::onSendError(const ErrorPtr& error)
{
    ErrorResponse::post(getHub(), kTypingIndicatorJobName, m_callback, error);
}