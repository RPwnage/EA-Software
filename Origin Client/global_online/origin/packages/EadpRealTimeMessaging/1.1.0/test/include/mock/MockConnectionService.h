// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <TestConfig.h>
#include <eadp/realtimemessaging/ConnectionService.h>

namespace eadp
{
namespace realtimemessaging
{

class MockConnectionService : public ConnectionService
{
public:
    MockConnectionService(eadp::foundation::IHub* hub) : m_hub(hub) {}
    ~MockConnectionService() override = default; 
    eadp::foundation::IHub* getHub() const override { return m_hub;  }
    MOCK_METHOD1(initialize, void(eadp::foundation::String productId));
    MOCK_METHOD5(connect, void(const eadp::foundation::String& accessToken, const eadp::foundation::String& clientVersion, ConnectCallback connectCallback, DisconnectCallback disconnectCallback, bool forceDisconnect));
    MOCK_METHOD1(connect, void(eadp::foundation::UniquePtr<MultiConnectionRequest> options));
    MOCK_CONST_METHOD0(isConnected, bool());
    MOCK_METHOD0(disconnect, void()); 
    MOCK_METHOD4(reconnect, void(const eadp::foundation::String& accessToken, const eadp::foundation::String& clientVersion, ConnectCallback connectCallback, DisconnectCallback disconnectCallback));
    MOCK_METHOD1(reconnect, void(eadp::foundation::UniquePtr<MultiConnectionRequest> options));
    MOCK_METHOD1(getSessions, void(GetSessionsCallback callback));
    MOCK_METHOD2(cleanupSession, void(const eadp::foundation::String& sessionKey, CleanupSessionCallback callback));
    MOCK_METHOD0(getRTMService, eadp::foundation::SharedPtr<Internal::RTMService>());

private:
    eadp::foundation::IHub* m_hub; 

}; // class MockConnectionService

} // namespace realtimemessaging
} // namespace eadp