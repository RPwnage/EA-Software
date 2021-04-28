// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#pragma once

#include <eadp/realtimemessaging/ConnectionService.h>
#include "rtm/CommunicationWrapper.h"

namespace eadp
{
namespace realtimemessaging
{
namespace Internal
{ 

class RTMService
{
public:
    enum class ConnectionState : uint32_t
    {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        RECONNECTING
    };

    using RTMCommunicationCallback = eadp::foundation::CallbackT<void(eadp::foundation::SharedPtr<CommunicationWrapper> communication)>;
    using RTMRequestCompleteCallback = eadp::foundation::CallbackT<void(eadp::foundation::SharedPtr<CommunicationWrapper> responseCommunication)>;
    using RTMErrorCallback = eadp::foundation::CallbackT<void(const eadp::foundation::ErrorPtr& error)>;

    virtual void connect(eadp::foundation::String accessToken, ConnectionService::ConnectCallback connectCb, ConnectionService::DisconnectCallback disconnectCb, bool forceDisconnect) = 0;
    virtual void reconnect(eadp::foundation::String accessToken, ConnectionService::ConnectCallback connectCb, ConnectionService::DisconnectCallback disconnectCb, bool forceDisconnect) = 0;
    virtual void disconnect() = 0;
    virtual void refresh() = 0; 
    virtual void sendCommunication(eadp::foundation::SharedPtr<CommunicationWrapper> communication, RTMErrorCallback errorCallback) = 0;
    virtual void sendRequest(eadp::foundation::String requestId, eadp::foundation::SharedPtr<CommunicationWrapper> communication, RTMRequestCompleteCallback completeCallback, RTMErrorCallback errorCallback, bool skipConnectivityCheck = false) = 0;
    virtual void removeRequest(eadp::foundation::String requestId) = 0;
    virtual void addRTMCommunicationCallback(RTMCommunicationCallback communicationCallback) = 0;
    virtual bool isRTMCommunicationReady() = 0;
    virtual eadp::foundation::String generateRequestId() = 0;
    virtual ConnectionState getState() = 0;
};

}
}
}

