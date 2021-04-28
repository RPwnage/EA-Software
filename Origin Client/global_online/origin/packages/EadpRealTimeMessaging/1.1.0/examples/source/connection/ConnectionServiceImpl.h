// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/realtimemessaging/ConnectionService.h>
#include "rtm/RTMService.h"

namespace eadp
{
namespace realtimemessaging
{
class ConnectionServiceImpl : public ConnectionService, public EADPSDK_ENABLE_SHARED_FROM_THIS<ConnectionServiceImpl>
{
public:
    explicit ConnectionServiceImpl(eadp::foundation::SharedPtr<eadp::foundation::IHub> hub);
    virtual ~ConnectionServiceImpl() = default;

    eadp::foundation::IHub* getHub() const override;
    void initialize(eadp::foundation::String productId) override;
    void connect(eadp::foundation::String accessToken, eadp::foundation::String clientVersion, ConnectCallback connectCallback, DisconnectCallback disconnectCallback, bool forceDisconnect) override;
    void connect(eadp::foundation::UniquePtr<MultiConnectionRequest> options) override;
    bool isConnected() const override;
    void disconnect() override;
    void reconnect(eadp::foundation::String accessToken, eadp::foundation::String clientVersion, ConnectCallback connectCallback, DisconnectCallback disconnectCallback) override;
    void reconnect(eadp::foundation::UniquePtr<MultiConnectionRequest> options) override;
    void getSessions(SessionRequestCallback callback);
    void cleanupSession(const eadp::foundation::String& sessionKey, SessionCleanupCallback callback);
    eadp::foundation::SharedPtr<Internal::RTMService> getRTMService() override;

private:
    eadp::foundation::SharedPtr<eadp::foundation::IHub> m_hub;
    eadp::foundation::SharedPtr<Internal::RTMService> m_rtmService;
};
}
}

