// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#include "PresenceSubscribeJob.h"
#include <eadp/realtimemessaging/RealTimeMessagingError.h>
#include <eadp/foundation/internal/ResponseT.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;
using namespace com::ea::eadp::antelope;


using PresenceResponse = ResponseT<const ErrorPtr&>;

constexpr char kPresenceSubscribeJobName[] = "PresenceSubscribeJob";

PresenceSubscribeJob::PresenceSubscribeJob(IHub* hub, SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService, SharedPtr<rtm::protocol::PresenceSubscribeV1> request, PresenceService::ErrorCallback errorCallback)
: RTMCommunicationJob(hub, EADPSDK_MOVE(rtmService))
, m_errorCallback(EADPSDK_MOVE(errorCallback))
, m_request(EADPSDK_MOVE(request))
{
}

void PresenceSubscribeJob::onSendError(const ErrorPtr& error)
{
    PresenceResponse::post(getHub(), kPresenceSubscribeJobName, m_errorCallback, error);
}

void PresenceSubscribeJob::setupOutgoingCommunication()
{
    SharedPtr<rtm::protocol::CommunicationV1> v1 = getHub()->getAllocator().makeShared<rtm::protocol::CommunicationV1>(getHub()->getAllocator());
    v1->setPresenceSubscribe(m_request);

    SharedPtr<rtm::protocol::Communication> communication = getHub()->getAllocator().makeShared<rtm::protocol::Communication>(getHub()->getAllocator());
    communication->setV1(v1);

    m_communication = getHub()->getAllocator().makeShared<CommunicationWrapper>(communication);
}
