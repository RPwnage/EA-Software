// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved.

#include "ConnectionServiceImpl.h"
#include "ConnectionJob.h"
#include "rtm/HeartbeatJob.h"
#include "rtm/RTMServiceImpl.h"
#include "rtm/SessionRequestJob.h"
#include "rtm/SessionCleanupJob.h"
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

ConnectionService::MultiConnectionRequest::MultiConnectionRequest(const Allocator& allocator)
    : m_accessToken(allocator.emptyString())
    , m_clientVersion(allocator.emptyString())
    , m_multiConnectCallback(nullptr)
    , m_disconnectCallback(nullptr)
    , m_userSessionChangeCallback(nullptr)
    , m_forceDisconnect(false)
    , m_sessionKey(allocator.emptyString())
{
}

ConnectionService::MultiConnectionRequest::MultiConnectionRequest(
    const eadp::foundation::String& accessToken,
    const eadp::foundation::String& clientVersion,
    ConnectionService::MultiConnectCallback multiConnectCallback,
    ConnectionService::DisconnectCallback disconnectCallback,
    ConnectionService::UserSessionChangeCallback userSessionChangeCallback,
    bool forceDisconnect,
    const eadp::foundation::String& sessionKey
)
    : m_accessToken(accessToken)
    , m_clientVersion(clientVersion)
    , m_multiConnectCallback(EADPSDK_MOVE(multiConnectCallback))
    , m_disconnectCallback(EADPSDK_MOVE(disconnectCallback))
    , m_userSessionChangeCallback(EADPSDK_MOVE(userSessionChangeCallback))
    , m_forceDisconnect(forceDisconnect)
    , m_sessionKey(sessionKey)
{
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

IHub* ConnectionServiceImpl::getHub() const
{
    return m_hub.get(); 
}

void ConnectionServiceImpl::initialize(eadp::foundation::String productId)
{
    // Create RTM Service
    m_rtmService = m_hub->getAllocator().makeShared<Internal::RTMServiceImpl>(m_hub.get(), EADPSDK_MOVE(productId));

    // Create heartbeat job
    auto heartbeatTimeoutJob = m_hub->getAllocator().makeUnique<eadp::realtimemessaging::HeartbeatJob>(m_rtmService);
    m_hub->addJob(EADPSDK_MOVE(heartbeatTimeoutJob));
}

void ConnectionServiceImpl::connect(String accessToken, String clientVersion, ConnectCallback connectCallback, DisconnectCallback disconnectCallback, bool forceDisconnect)
{
    auto connectJob = m_hub->getAllocator().makeUnique<ConnectionJob>(m_hub.get(), shared_from_this(), ConnectionJob::Type::CONNECT, accessToken, clientVersion, connectCallback, nullptr, disconnectCallback, nullptr, forceDisconnect, m_hub->getAllocator().emptyString());
    m_hub->addJob(EADPSDK_MOVE(connectJob));
}

void ConnectionServiceImpl::connect(UniquePtr<ConnectionService::MultiConnectionRequest> request)
{
    auto connectJob = m_hub->getAllocator().makeUnique<ConnectionJob>(m_hub.get()
                                                                      , shared_from_this()
                                                                      , ConnectionJob::Type::MULTICONNECT
                                                                      , request->m_accessToken
                                                                      , request->m_clientVersion
                                                                      , nullptr
                                                                      , request->m_multiConnectCallback
                                                                      , request->m_disconnectCallback
                                                                      , request->m_userSessionChangeCallback
                                                                      , request->m_forceDisconnect
                                                                      , request->m_sessionKey);

    m_hub->addJob(EADPSDK_MOVE(connectJob));
}

bool ConnectionServiceImpl::isConnected() const
{
    if (!m_rtmService) return false; 
    return m_rtmService->getState() == eadp::realtimemessaging::Internal::RTMService::ConnectionState::CONNECTED;
}

void ConnectionServiceImpl::reconnect(String accessToken, String clientVersion, ConnectCallback connectCallback, DisconnectCallback disconnectCallback)
{
    auto reconnectJob = m_hub->getAllocator().makeUnique<ConnectionJob>(m_hub.get(), shared_from_this(), ConnectionJob::Type::RECONNECT, accessToken, clientVersion, connectCallback, nullptr, disconnectCallback, nullptr, true, m_hub->getAllocator().emptyString());
    m_hub->addJob(EADPSDK_MOVE(reconnectJob));
}

void ConnectionServiceImpl::reconnect(UniquePtr<ConnectionService::MultiConnectionRequest> request)
{
    auto reconnectJob = m_hub->getAllocator().makeUnique<ConnectionJob>(m_hub.get()
                                                                        , shared_from_this()
                                                                        , ConnectionJob::Type::MULTIRECONNECT
                                                                        , request->m_accessToken
                                                                        , request->m_clientVersion
                                                                        , nullptr
                                                                        , request->m_multiConnectCallback
                                                                        , request->m_disconnectCallback
                                                                        , request->m_userSessionChangeCallback
                                                                        , request->m_forceDisconnect
                                                                        , request->m_sessionKey);

    m_hub->addJob(EADPSDK_MOVE(reconnectJob));
}

void ConnectionServiceImpl::getSessions(SessionRequestCallback callback)
{
    auto sessionRequestJob = m_hub->getAllocator().makeUnique<SessionRequestJob>(m_hub.get(), m_rtmService, callback);
    m_hub->addJob(EADPSDK_MOVE(sessionRequestJob));
}

void ConnectionServiceImpl::disconnect()
{
    auto disconnectJob = m_hub->getAllocator().makeUnique<ConnectionJob>(m_hub.get(), shared_from_this(), ConnectionJob::Type::DISCONNECT, m_hub->getAllocator().emptyString(), m_hub->getAllocator().emptyString(), nullptr, nullptr, nullptr, nullptr, false, m_hub->getAllocator().emptyString());
    m_hub->addJob(EADPSDK_MOVE(disconnectJob));
}

void ConnectionServiceImpl::cleanupSession(const String& sessionKey, SessionCleanupCallback callback)
{
    auto sessionCleanupJob = m_hub->getAllocator().makeUnique<SessionCleanupJob>(m_hub.get(), m_rtmService, sessionKey, callback);
    m_hub->addJob(EADPSDK_MOVE(sessionCleanupJob));
}

SharedPtr<eadp::realtimemessaging::Internal::RTMService> ConnectionServiceImpl::getRTMService()
{
    return m_rtmService;
}
