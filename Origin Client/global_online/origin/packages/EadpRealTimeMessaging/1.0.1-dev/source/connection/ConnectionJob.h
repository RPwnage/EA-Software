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
        DISCONNECT
    };

    ConnectionJob(eadp::foundation::IHub* hub,
        eadp::foundation::WeakPtr<ConnectionService> connectionService,
        Type jobType,
        eadp::foundation::String accessToken,
        ConnectionService::ConnectCallback connectCb,
        ConnectionService::DisconnectCallback disconnectCb, 
        bool forceDisconnect);
    virtual ~ConnectionJob() = default;
    void execute() override;

private:
    eadp::foundation::IHub* m_hub;
    eadp::foundation::WeakPtr<ConnectionService> m_connectionServiceWeak;
    Type m_jobType;
    eadp::foundation::String m_accessToken;
    ConnectionService::ConnectCallback m_connectCallback;
    ConnectionService::DisconnectCallback m_disconnectCallback;
    bool m_forceDisconnect;
};
}
}
