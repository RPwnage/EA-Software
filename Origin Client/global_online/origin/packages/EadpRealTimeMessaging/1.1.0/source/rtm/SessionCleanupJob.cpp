// Copyright(C) 2019-2020 Electronic Arts, Inc. All rights reserved.

#include "SessionCleanupJob.h"
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/realtimemessaging/RealTimeMessagingError.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope;

using SessionCleanupResponse = ResponseT<const ErrorPtr&>;
constexpr char kSessionCleanupJobName[] = "SessionCleanupJob";

SessionCleanupJob::SessionCleanupJob(IHub* hub,
                                   SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService,
                                   const String& sessionKey,
                                   Callback callback)
: RTMCommunicationJob(hub, EADPSDK_MOVE(rtmService))
, m_sessionKey(sessionKey)
, m_callback(EADPSDK_MOVE(callback))
{
}

void SessionCleanupJob::setupOutgoingCommunication()
{
    auto allocator = getHub()->getAllocator();
    auto sessionCleanup = allocator.makeShared<rtm::protocol::SessionCleanupV1>(allocator);
    sessionCleanup->setSessionKey(m_sessionKey);

    SharedPtr<rtm::protocol::CommunicationV1> v1 = getHub()->getAllocator().makeShared<rtm::protocol::CommunicationV1>(getHub()->getAllocator());
    v1->setSessionCleanupV1(sessionCleanup);

    SharedPtr<rtm::protocol::Communication> communication = getHub()->getAllocator().makeShared<rtm::protocol::Communication>(getHub()->getAllocator());
    communication->setV1(v1);

    m_communication = getHub()->getAllocator().makeShared<CommunicationWrapper>(communication);
}

void SessionCleanupJob::onSendError(const eadp::foundation::ErrorPtr& error)
{
    SessionCleanupResponse::post(getHub(), kSessionCleanupJobName, m_callback, error);
}
