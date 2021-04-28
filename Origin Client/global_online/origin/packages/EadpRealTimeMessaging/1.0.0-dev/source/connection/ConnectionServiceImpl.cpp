// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved.

#include "ConnectionServiceImpl.h"
#include "ConnectionJob.h"
#include "rtm/HeartbeatJob.h"
#include "rtm/RTMServiceImpl.h"
#include <eadp/foundation/LoggingService.h>

using namespace eadp::realtimemessaging;
using namespace eadp::foundation;


ConnectionService::DisconnectSource::DisconnectSource(ConnectionService::DisconnectSource::Reason reason)
: m_reason(reason)
, m_sessionMessage()
{
}

ConnectionService::DisconnectSource::DisconnectSource(ConnectionService::DisconnectSource::Reason reason, const eadp::foundation::String& message)
: m_reason(reason)
, m_sessionMessage(message)
{
}

ConnectionService::DisconnectSource::Reason ConnectionService::DisconnectSource::getReason() const
{
    return m_reason;
}

const String& ConnectionService::DisconnectSource::getSessionMessage() const
{
    return m_sessionMessage;
}

SharedPtr<ConnectionService> ConnectionService::createService(SharedPtr<IHub> hub)
{
    if (!hub)
    {
        EA_FAIL_MESSAGE("ConnectionService: hub is NULL, could not create ConnectionService");
        return nullptr;
    }

    EADPSDK_LOGV(hub.get(), "ConnectionService", "createService method invoked");
#if defined(EADP_MESSAGING_VERSION)
    EADPSDK_LOG_VERSION(hub.get(), "ConnectionService", EADP_MESSAGING_VERSION);
#endif
    auto service = hub->getAllocator().makeShared<ConnectionServiceImpl>(EADPSDK_MOVE(hub));
    return service;
}

ConnectionServiceImpl::ConnectionServiceImpl(SharedPtr<IHub> hub) 
: m_hub(EADPSDK_MOVE(hub))
, m_rtmService(nullptr)
{
}

void ConnectionServiceImpl::initialize(eadp::foundation::String productId)
{
    // Create RTM Service
    m_rtmService = m_hub->getAllocator().makeShared<Internal::RTMServiceImpl>(m_hub.get(), EADPSDK_MOVE(productId));

    // Create heartbeat job
    auto heartbeatTimeoutJob = m_hub->getAllocator().makeUnique<eadp::realtimemessaging::HeartbeatJob>(m_rtmService);
    m_hub->addJob(EADPSDK_MOVE(heartbeatTimeoutJob));
}

void ConnectionServiceImpl::connect(String accessToken, ConnectCallback connectCallback, DisconnectCallback disconnectCallback, bool forceDisconnect)
{
    auto connectJob = m_hub->getAllocator().makeUnique<ConnectionJob>(m_hub.get(), shared_from_this(), ConnectionJob::Type::CONNECT, accessToken, connectCallback, disconnectCallback, forceDisconnect);
    m_hub->addJob(EADPSDK_MOVE(connectJob));
}

bool ConnectionServiceImpl::isConnected() const
{
    if (!m_rtmService) return false; 
    return m_rtmService->getState() == eadp::realtimemessaging::Internal::RTMService::ConnectionState::CONNECTED;
}

void ConnectionServiceImpl::reconnect(String accessToken, ConnectCallback connectCallback, DisconnectCallback disconnectCallback)
{
    auto reconnectJob = m_hub->getAllocator().makeUnique<ConnectionJob>(m_hub.get(), shared_from_this(), ConnectionJob::Type::RECONNECT, accessToken, connectCallback, disconnectCallback, true);
    m_hub->addJob(EADPSDK_MOVE(reconnectJob));
}

void ConnectionServiceImpl::disconnect()
{
    auto disconnectJob = m_hub->getAllocator().makeUnique<ConnectionJob>(m_hub.get(), shared_from_this(), ConnectionJob::Type::DISCONNECT, m_hub->getAllocator().emptyString(), nullptr, nullptr, false);
    m_hub->addJob(EADPSDK_MOVE(disconnectJob));
}

SharedPtr<eadp::realtimemessaging::Internal::RTMService> ConnectionServiceImpl::getRTMService()
{
    return m_rtmService;
}
