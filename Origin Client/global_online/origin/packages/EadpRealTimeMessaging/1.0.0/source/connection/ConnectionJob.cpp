// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved.

#include "ConnectionJob.h"
#include "rtm/RTMService.h"
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/realtimemessaging/RealTimeMessagingError.h>

using namespace eadp::realtimemessaging;
using namespace eadp::foundation;

using RTMConnectResponse = ResponseT<eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginV2Success>, const eadp::foundation::ErrorPtr&>;
constexpr char kConnectResponseJobName[] = "ConnectResponseJob";

ConnectionJob::ConnectionJob(eadp::foundation::IHub* hub,
    eadp::foundation::WeakPtr<ConnectionService> connectionService,
    Type jobType,
    String accessToken,
    ConnectionService::ConnectCallback connectCb,
    ConnectionService::DisconnectCallback disconnectCb, 
    bool forceDisconnect)
: m_hub(hub)
, m_connectionServiceWeak(connectionService)
, m_jobType(jobType)
, m_accessToken(EADPSDK_MOVE(accessToken))
, m_connectCallback(connectCb)
, m_disconnectCallback(disconnectCb)
, m_forceDisconnect(forceDisconnect)
{
}

void ConnectionJob::execute()
{
    m_done = true;

    SharedPtr<ConnectionService> connectionService = m_connectionServiceWeak.lock();
    if (!connectionService)
    {
        auto error = RealTimeMessagingError::create(m_hub->getAllocator(), RealTimeMessagingError::Code::CONNECTION_SHUTDOWN, "ConnectionService has been shutdown");
        RTMConnectResponse::post(m_hub, kConnectResponseJobName, m_connectCallback, nullptr, error);
        return;
    }

    SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService = connectionService->getRTMService();
    if (!rtmService)
    {
        auto error = FoundationError::create(m_hub->getAllocator(), FoundationError::Code::NOT_INITIALIZED, "CommunicationService is not initialized");
        RTMConnectResponse::post(m_hub, kConnectResponseJobName, m_connectCallback, nullptr, error);
        return;
    }

    switch (m_jobType)
    {
    case Type::CONNECT:
    {
        rtmService->connect(m_accessToken, m_connectCallback, m_disconnectCallback, m_forceDisconnect);
        break;
    }
    case Type::RECONNECT:
    {
        rtmService->reconnect(m_accessToken, m_connectCallback, m_disconnectCallback, m_forceDisconnect);
        break;
    }
    case Type::DISCONNECT:
    {
        rtmService->disconnect();
        break;
    }
    default:
    {
        break;
    }
    }
}

