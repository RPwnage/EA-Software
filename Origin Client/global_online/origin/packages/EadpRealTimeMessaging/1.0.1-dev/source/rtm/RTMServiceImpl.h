// Copyright(C) 2019-2020 Electronic Arts, Inc. All rights reserved.

#pragma once

#include "RTMService.h"
#include "RTMSerializer.h"
#include "network/SSLSocket.h"
#include <eadp/foundation/Config.h>
#include <eadp/foundation/internal/CallbackList.h>
#include <eadp/realtimemessaging/ConnectionService.h>
#include <eathread/eathread_futex.h>


namespace eadp
{
namespace realtimemessaging
{
namespace Internal
{

class RTMServiceImpl : public RTMService, public EADPSDK_ENABLE_SHARED_FROM_THIS<RTMServiceImpl>
{
public:
    RTMServiceImpl(eadp::foundation::IHub* hub, eadp::foundation::String productId);

    void connect(eadp::foundation::String accessToken, ConnectionService::ConnectCallback connectCb, ConnectionService::DisconnectCallback disconnectCb, bool forceDisconnect) override;
    void disconnect() override;
    void refresh() override; 
    void reconnect(eadp::foundation::String accessToken, ConnectionService::ConnectCallback connectCb, ConnectionService::DisconnectCallback disconnectCb, bool forceDisconnect) override;
    void sendCommunication(eadp::foundation::SharedPtr<CommunicationWrapper> communication, RTMErrorCallback errorCallback) override;
    void sendRequest(eadp::foundation::String requestId,
        eadp::foundation::SharedPtr<CommunicationWrapper> communication,
        RTMRequestCompleteCallback completeCallback,
        RTMErrorCallback errorCallback,
        bool skipConnectivityCheck) override;
    void removeRequest(eadp::foundation::String requestId) override;
    eadp::foundation::String generateRequestId() override;
    void addRTMCommunicationCallback(RTMCommunicationCallback communicationCallback) override;
    bool isRTMCommunicationReady() override;
    virtual ~RTMServiceImpl() {};
    ConnectionState getState() override;

    EA_NON_COPYABLE(RTMServiceImpl);

private:

    struct QueuedReconnectionData
    {
        QueuedReconnectionData(eadp::foundation::SharedPtr<CommunicationWrapper> comm, RTMErrorCallback cb) :
            communication(comm),
            callback(cb) {}

        eadp::foundation::SharedPtr<CommunicationWrapper> communication;
        RTMErrorCallback callback;
    };

    void openSocketConnection();
    void closeSocketConnection();
    void onSocketConnect(const eadp::foundation::ErrorPtr& error);
    void onSocketDisconnect(const eadp::foundation::ErrorPtr& error);
    void onSocketData(eadp::foundation::UniquePtr<eadp::foundation::Internal::RawBuffer> data);
    void connectHelper(eadp::foundation::String accessToken, ConnectionService::ConnectCallback connectCb, ConnectionService::DisconnectCallback disconnectCb, bool forceDisconnect, bool isReconnect);
    void setState(ConnectionState state);
    void notifyServiceConnected(eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginV2Success> loginSuccess, const eadp::foundation::ErrorPtr& error);
    void notifyServiceDisconnected(ConnectionService::DisconnectSource source, const eadp::foundation::ErrorPtr& error);
    void sendLoginRequest();
    void onLoginResponse(eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginV2Success> loginSuccess, const eadp::foundation::ErrorPtr& error, bool isReconnect);
    void completeReconnection();
    void sendCommunicationOnSocket(const eadp::foundation::SharedPtr<CommunicationWrapper>& commWrap, RTMErrorCallback errorCallback);
    void queueReconnectionData(const eadp::foundation::SharedPtr<CommunicationWrapper>& commWrap, RTMErrorCallback errorCallback);
    void processCommunication(eadp::foundation::SharedPtr<CommunicationWrapper> commWrapper);
    void resetRequestIdCounter();
    void delegateResponse(eadp::foundation::String requestId, eadp::foundation::SharedPtr<CommunicationWrapper> communiction);
    void reinitialize();

    eadp::foundation::IHub* m_hub;
    eadp::foundation::String m_productId;
    eadp::foundation::String m_accessToken;
    ConnectionService::ConnectCallback m_connectCallback;
    ConnectionService::DisconnectCallback m_disconnectCallback;
    eadp::foundation::Callback::PersistencePtr m_persistObj;
    ConnectionState m_connectionState;
    RTMSerializer m_serializer;
    eadp::foundation::SharedPtr<SSLSocket> m_socket;
    uint32_t m_requestIdCounter;
    eadp::foundation::HashStringMap<RTMRequestCompleteCallback> m_requestManager;
    eadp::foundation::String m_data;
    eadp::foundation::Vector<QueuedReconnectionData> m_queueReconnectionData;
    eadp::foundation::SharedPtr<eadp::foundation::Internal::CallbackList<RTMCommunicationCallback>> m_rtmCallbackList;
    eadp::foundation::SharedPtr<eadp::realtimemessaging::CommunicationWrapper> m_heartbeatCommunication;
    EA::Thread::Futex m_connectionStateLock;
    bool m_forceDisconnect;
};

}
}
}
