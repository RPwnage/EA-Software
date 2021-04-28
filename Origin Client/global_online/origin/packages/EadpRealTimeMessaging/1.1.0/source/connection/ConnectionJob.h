// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/internal/JobBase.h>
#include <eadp/realtimemessaging/ConnectionService.h>

namespace eadp
{
namespace realtimemessaging
{
class ConnectionJob : public eadp::foundation::Internal::JobBase
{
public:
    enum class Type : uint32_t
    {
        CONNECT,
        RECONNECT,
        MULTICONNECT,    //multi-session supported connect
        MULTIRECONNECT,  //multi-session supported reconnect
        DISCONNECT
    };

    ConnectionJob(eadp::foundation::IHub* hub,
        eadp::foundation::WeakPtr<ConnectionService> connectionService,
        Type jobType,
        eadp::foundation::String accessToken,
        eadp::foundation::String clientVersion,
        ConnectionService::ConnectCallback connectCb,
        ConnectionService::MultiConnectCallback multiConnectCb,
        ConnectionService::DisconnectCallback disconnectCb, 
        ConnectionService::UserSessionChangeCallback userSessionChangeCb,
        bool forceDisconnect,
        eadp::foundation::String forceDisconnectSessionKey,
        eadp::foundation::String reconnectSessionKey);

    // Overload for disconnect case
    ConnectionJob(eadp::foundation::IHub* hub,
        eadp::foundation::WeakPtr<ConnectionService> connectionService);

    virtual ~ConnectionJob() = default;
    void execute() override;

private:
    eadp::foundation::IHub* m_hub;
    eadp::foundation::WeakPtr<ConnectionService> m_connectionServiceWeak;
    Type m_jobType;
    eadp::foundation::String m_accessToken;
    eadp::foundation::String m_clientVersion;
    ConnectionService::ConnectCallback m_connectCallback;
    ConnectionService::MultiConnectCallback m_multiConnectCallback;
    ConnectionService::DisconnectCallback m_disconnectCallback;
    ConnectionService::UserSessionChangeCallback m_userSessionChangeCallback;
    bool m_forceDisconnect;
    eadp::foundation::String m_forceDisconnectSessionKey;
    eadp::foundation::String m_reconnectSessionKey;
};
}
}
