// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved.

#include "ConnectionJob.h"
#include "rtm/RTMService.h"
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/realtimemessaging/RealTimeMessagingError.h>

using namespace eadp::realtimemessaging;
using namespace eadp::foundation;

using RTMConnectResponse = ResponseT<eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginV2Success>, const eadp::foundation::ErrorPtr&>;
using RTMMultiConnectResponse = ResponseT<eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginV3Response>, eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginErrorV1>, const eadp::foundation::ErrorPtr&>;
constexpr char kConnectRequestJobName[] = "ConnectRequestJob";

ConnectionJob::ConnectionJob(IHub* hub,
                             WeakPtr<ConnectionService> connectionService,
                             Type jobType,
                             String accessToken,
                             String clientVersion,
                             ConnectionService::ConnectCallback connectCb,
                             ConnectionService::MultiConnectCallback multiConnectCb,
                             ConnectionService::DisconnectCallback disconnectCb, 
                             ConnectionService::UserSessionChangeCallback userSessionChangeCb,
                             bool forceDisconnect,
                             String forceDisconnectSessionKey,
                             String reconnectSessionKey)
: m_hub(hub)
, m_connectionServiceWeak(EADPSDK_MOVE(connectionService))
, m_jobType(jobType)
, m_accessToken(EADPSDK_MOVE(accessToken))
, m_clientVersion(EADPSDK_MOVE(clientVersion))
, m_connectCallback(connectCb)
, m_multiConnectCallback(multiConnectCb)
, m_disconnectCallback(disconnectCb)
, m_userSessionChangeCallback(userSessionChangeCb)
, m_forceDisconnect(forceDisconnect)
, m_forceDisconnectSessionKey(EADPSDK_MOVE(forceDisconnectSessionKey))
, m_reconnectSessionKey(EADPSDK_MOVE(reconnectSessionKey))
{
}

ConnectionJob::ConnectionJob(IHub* hub,
    WeakPtr<ConnectionService> connectionService)
    : m_hub(hub)
    , m_connectionServiceWeak(EADPSDK_MOVE(connectionService))
    , m_jobType(Type::DISCONNECT)
    , m_accessToken(hub->getAllocator().emptyString())
    , m_clientVersion(hub->getAllocator().emptyString())
    , m_connectCallback(nullptr)
    , m_multiConnectCallback(nullptr)
    , m_disconnectCallback(nullptr)
    , m_userSessionChangeCallback(nullptr)
    , m_forceDisconnect(false)
    , m_forceDisconnectSessionKey(hub->getAllocator().emptyString())
    , m_reconnectSessionKey(hub->getAllocator().emptyString())
{
}

void ConnectionJob::execute()
{
    m_done = true;

    SharedPtr<ConnectionService> connectionService = m_connectionServiceWeak.lock();
    if (!connectionService)
    {
        auto error = RealTimeMessagingError::create(m_hub->getAllocator(), RealTimeMessagingError::Code::CONNECTION_SHUTDOWN, "ConnectionService has been shutdown");
        if (m_connectCallback.isValid())
        {
            RTMConnectResponse::post(m_hub, kConnectRequestJobName, m_connectCallback, nullptr, error);
        }

        if (m_multiConnectCallback.isValid())
        {
            RTMMultiConnectResponse::post(m_hub, kConnectRequestJobName, m_multiConnectCallback, nullptr, nullptr, error);
        }
        return;
    }

    SharedPtr<eadp::realtimemessaging::Internal::RTMService> rtmService = connectionService->getRTMService();
    if (!rtmService)
    {
        auto error = FoundationError::create(m_hub->getAllocator(), FoundationError::Code::NOT_INITIALIZED, "CommunicationService is not initialized");
        if (m_connectCallback.isValid())
        {
            RTMConnectResponse::post(m_hub, kConnectRequestJobName, m_connectCallback, nullptr, error);
        }

        if (m_multiConnectCallback.isValid())
        {
            RTMMultiConnectResponse::post(m_hub, kConnectRequestJobName, m_multiConnectCallback, nullptr, nullptr, error);
        }
        return;
    }

    switch (m_jobType)
    {
    case Type::CONNECT:
    {
        rtmService->connect(m_accessToken, m_clientVersion, m_connectCallback, m_disconnectCallback, m_forceDisconnect);
        break;
    }
    case Type::RECONNECT:
    {
        rtmService->reconnect(m_accessToken, m_clientVersion, m_connectCallback, m_disconnectCallback, m_forceDisconnect);
        break;
    }
    case Type::MULTICONNECT:
    {
        rtmService->connect(m_accessToken, m_clientVersion, m_multiConnectCallback, m_disconnectCallback, m_userSessionChangeCallback, m_forceDisconnect, m_forceDisconnectSessionKey);
        break;
    }
    case Type::MULTIRECONNECT:
    {
        rtmService->reconnect(m_accessToken, m_clientVersion, m_multiConnectCallback, m_disconnectCallback, m_userSessionChangeCallback, m_forceDisconnect, m_forceDisconnectSessionKey, m_reconnectSessionKey);
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

