// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <TestConfig.h>
#include <../source/rtm/RTMService.h>

namespace eadp
{
namespace realtimemessaging
{
namespace Internal
{

class MockRTMService : public RTMService
{
public:
    ~MockRTMService() override = default;
    MOCK_METHOD5(connect, void(eadp::foundation::String accessToken, eadp::foundation::String clientVersion, ConnectionService::ConnectCallback connectCb, ConnectionService::DisconnectCallback disconnectCb, bool forceDisconnect));
    MOCK_METHOD5(reconnect, void(eadp::foundation::String accessToken, eadp::foundation::String clientVersion, ConnectionService::ConnectCallback connectCb, ConnectionService::DisconnectCallback disconnectCb, bool forceDisconnect));
    MOCK_METHOD7(connect, void(eadp::foundation::String accessToken, eadp::foundation::String clientVersion, ConnectionService::MultiConnectCallback connectCb, ConnectionService::DisconnectCallback disconnectCb, ConnectionService::UserSessionChangeCallback userSessionChangeCb, bool forceDisconnect, eadp::foundation::String forceDisconnectSessionKey));
    MOCK_METHOD8(reconnect, void(eadp::foundation::String accessToken, eadp::foundation::String clientVersion, ConnectionService::MultiConnectCallback connectCb, ConnectionService::DisconnectCallback disconnectCb, ConnectionService::UserSessionChangeCallback userSessionChangeCb, bool forceDisconnect, eadp::foundation::String forceDisconnectSessionKey, eadp::foundation::String reconnectSessionKey));
    MOCK_METHOD0(disconnect, void());
    MOCK_METHOD0(refresh, void());
    MOCK_METHOD2(sendCommunication, void(eadp::foundation::SharedPtr<CommunicationWrapper> communication, RTMErrorCallback errorCallback));
    MOCK_METHOD5(sendRequest, void(eadp::foundation::String requestId, eadp::foundation::SharedPtr<CommunicationWrapper> communication, RTMRequestCompleteCallback completeCallback, RTMErrorCallback errorCallback, bool skipConnectivityCheck));
    MOCK_METHOD1(removeRequest, void(eadp::foundation::String requestId));
    MOCK_METHOD1(addRTMCommunicationCallback, void(RTMCommunicationCallback communicationCallback));
    MOCK_METHOD0(isRTMCommunicationReady, bool());
    MOCK_METHOD0(generateRequestId, eadp::foundation::String());
    MOCK_METHOD0(getState, ConnectionState());

}; // class MockRTMService

} // namespace Internal
} // namespace realtimemessaging
} // namespace eadp