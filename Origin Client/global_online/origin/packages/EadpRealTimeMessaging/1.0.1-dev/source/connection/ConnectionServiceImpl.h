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

    void initialize(eadp::foundation::String productId) override;
    void connect(eadp::foundation::String accessToken, ConnectCallback connectCallback, DisconnectCallback disconnectCallback, bool forceDisconnect) override;
    bool isConnected() const override;
    void disconnect() override;
    void reconnect(eadp::foundation::String accessToken, ConnectCallback connectCallback, DisconnectCallback disconnectCallback) override;
    eadp::foundation::SharedPtr<Internal::RTMService> getRTMService() override;

private:
    eadp::foundation::SharedPtr<eadp::foundation::IHub> m_hub;
    eadp::foundation::SharedPtr<Internal::RTMService> m_rtmService;
};
}
}

