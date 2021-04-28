// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved

#include "RTMServiceImpl.h"
#include "LoginRequestJob.h"
#include <EAStdC/EAString.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/foundation/internal/HttpRequest.h>
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/foundation/internal/Timestamp.h>
#include <eadp/foundation/internal/Utils.h>
#include <com/ea/eadp/antelope/rtm/protocol/SessionNotificationV1.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging::Internal;
using namespace com::ea::eadp::antelope;

#define DIRECTOR_KEY_RTM_SERVICE   "eadp.realtimemessaging"
#define REQUEST_ID_COUNTER_START 1

using RTMErrorResponse = ResponseT<const ErrorPtr&>;
constexpr char kRTMErrorResponseJobName[] = "RTMErrorResponseJob";

using RTMCommunicationResponse = ResponseT<SharedPtr<eadp::realtimemessaging::CommunicationWrapper>>;
constexpr char kRTMRequestCompleteResponseJobname[] = "RTMRequestCompleteResponseJob";
constexpr char kRTMCommunicationResponseJobname[] = "RTMCommunicationResponseJob";

using RTMConnectResponse = ResponseT<SharedPtr<rtm::protocol::LoginV2Success>, const ErrorPtr&>;
constexpr char kRTMConnectResponseJobName[] = "RTMconnectResponseJob";

using RTMDisconnectResponse = ResponseT<const eadp::realtimemessaging::ConnectionService::DisconnectSource&, const ErrorPtr&>;
constexpr char kRTMDisconnectResponseJobName[] = "RTMDisconnectResponseJob";

RTMServiceImpl::RTMServiceImpl(IHub* hub, String productId)
: m_hub(hub)
, m_productId(EADPSDK_MOVE(productId))
, m_accessToken(hub->getAllocator().emptyString())
, m_connectCallback(nullptr)
, m_disconnectCallback(nullptr)
, m_persistObj(hub->getAllocator().makeShared<Callback::Persistence>())
, m_connectionState(ConnectionState::DISCONNECTED)
, m_serializer(hub)
, m_socket(hub->getAllocator().makeShared<eadp::realtimemessaging::SSLSocket>(hub))
, m_requestIdCounter(REQUEST_ID_COUNTER_START)
, m_requestManager(hub->getAllocator().make<HashStringMap<RTMRequestCompleteCallback>>())
, m_data(hub->getAllocator().emptyString())
, m_queueReconnectionData(hub->getAllocator().make<Vector<QueuedReconnectionData>>())
, m_rtmCallbackList(hub->getAllocator().makeShared<eadp::foundation::Internal::CallbackList<RTMCommunicationCallback>>(hub))
, m_heartbeatCommunication(nullptr)
, m_connectionStateLock()
, m_forceDisconnect(false)
{
    // Create heartbeat communication wrapper
    auto allocator = m_hub->getAllocator();
    auto heartbeat = allocator.makeShared<rtm::protocol::HeartbeatV1>(allocator);
    auto communicationV1 = allocator.makeShared<rtm::protocol::CommunicationV1>(allocator);
    communicationV1->setHeartbeat(EADPSDK_MOVE(heartbeat));
    auto rtmCommunication = allocator.makeShared<rtm::protocol::Communication>(allocator);
    rtmCommunication->setV1(EADPSDK_MOVE(communicationV1));
    m_heartbeatCommunication = allocator.makeShared<eadp::realtimemessaging::CommunicationWrapper>(EADPSDK_MOVE(rtmCommunication));
}

/*************************************************************************
 * Connection Service
 *************************************************************************/
void RTMServiceImpl::connect(String accessToken, eadp::realtimemessaging::ConnectionService::ConnectCallback connectCb, eadp::realtimemessaging::ConnectionService::DisconnectCallback disconnectCb, bool forceDisconnect)
{
    connectHelper(accessToken, connectCb, disconnectCb, forceDisconnect, false);
}

void RTMServiceImpl::reconnect(String accessToken, eadp::realtimemessaging::ConnectionService::ConnectCallback connectCb, eadp::realtimemessaging::ConnectionService::DisconnectCallback disconnectCb, bool forceDisconnect)
{
    connectHelper(accessToken, connectCb, disconnectCb, forceDisconnect, true);
}

void RTMServiceImpl::connectHelper(String accessToken, eadp::realtimemessaging::ConnectionService::ConnectCallback connectCb, eadp::realtimemessaging::ConnectionService::DisconnectCallback disconnectCb, bool forceDisconnect, bool isReconnect)
{
    m_accessToken = accessToken;
    m_disconnectCallback = disconnectCb;
    m_connectCallback = connectCb;
    m_forceDisconnect = forceDisconnect;

    switch (getState())
    {
    case ConnectionState::CONNECTED:
    {
        notifyServiceConnected(nullptr, nullptr);
        break;
    }
    case ConnectionState::DISCONNECTED:
    {
        if (isReconnect)
        {
            setState(ConnectionState::RECONNECTING);
        }
        else
        {
            reinitialize();
            setState(ConnectionState::CONNECTING);
        }
        openSocketConnection();
        break;
    }
    case ConnectionState::RECONNECTING:
    case ConnectionState::CONNECTING:
        break;
    default:
    {
        String message = m_hub->getAllocator().make<String>("Unknown Connection State");
        EADPSDK_LOGE(m_hub, "RTMService", message);
        auto error = FoundationError::create(m_hub->getAllocator(), FoundationError::Code::SYSTEM_UNEXPECTED, message);
        notifyServiceConnected(nullptr, error);
        break;
    }
    }
}

void RTMServiceImpl::disconnect()
{
    m_disconnectCallback = nullptr;
    m_connectCallback = nullptr;
    m_forceDisconnect = true;

    closeSocketConnection(true);
    setState(ConnectionState::DISCONNECTED);

    reinitialize();
}

void RTMServiceImpl::refresh()
{
    sendCommunication(m_heartbeatCommunication, nullptr);
}

/*************************************************************************
 * RTM Service
 *************************************************************************/

void RTMServiceImpl::addRTMCommunicationCallback(RTMCommunicationCallback communicationCallback)
{
    m_rtmCallbackList->add(communicationCallback);
}

void RTMServiceImpl::sendCommunication(SharedPtr<eadp::realtimemessaging::CommunicationWrapper> communication, RTMErrorCallback errorCallback)
{
    if (getState() != ConnectionState::CONNECTED && getState() != ConnectionState::RECONNECTING)
    {
        String message = m_hub->getAllocator().make<String>("ConnectionService must be connected before sending RTM request");
        auto error = RealTimeMessagingError::create(m_hub->getAllocator(), RealTimeMessagingError::Code::CONNECTION_SERVICE_NOT_CONNECTED, message);

        RTMErrorResponse::post(m_hub, kRTMErrorResponseJobName, errorCallback, error);
        return;
    }

    if (getState() == ConnectionState::RECONNECTING)
    {
        queueReconnectionData(communication, errorCallback);
    }
    else
    {
        sendCommunicationOnSocket(communication, errorCallback);
    }
}

void RTMServiceImpl::sendRequest(String requestId,
    SharedPtr<eadp::realtimemessaging::CommunicationWrapper> communication,
    RTMRequestCompleteCallback completeCallback,
    RTMErrorCallback errorCallback,
    bool skipConnectivityCheck)
{
    if (!skipConnectivityCheck)
    {
        if (getState() != ConnectionState::CONNECTED && getState() != ConnectionState::RECONNECTING)
        {
            String message = m_hub->getAllocator().make<String>("ConnectionService must be connected before sending RTM request");
            auto error = RealTimeMessagingError::create(m_hub->getAllocator(), RealTimeMessagingError::Code::CONNECTION_SERVICE_NOT_CONNECTED, message);

            RTMErrorResponse::post(m_hub, kRTMErrorResponseJobName, errorCallback, error);
            return;
        }
    }

    m_requestManager.emplace(requestId, completeCallback);

    if (getState() == ConnectionState::RECONNECTING && !skipConnectivityCheck)
    {
        queueReconnectionData(communication, errorCallback);
    }
    else
    {
        sendCommunicationOnSocket(communication, errorCallback);
    }
}

void RTMServiceImpl::sendCommunicationOnSocket(const SharedPtr<eadp::realtimemessaging::CommunicationWrapper>& commWrap, RTMErrorCallback errorCallback)
{

    SharedPtr<eadp::foundation::Internal::RawBuffer> buffer = m_hub->getAllocator().makeShared<eadp::foundation::Internal::RawBuffer>(m_hub->getAllocator());

    if (commWrap->hasRtmCommunication())
    {
        EADPSDK_LOGV(m_hub, "RTMService", "Sending RTM Proto : %s", commWrap->getRtmCommunication()->toString("").c_str());
    }
    else
    {
        EADPSDK_LOGV(m_hub, "RTMService", "Sending Social Proto : %s", commWrap->getSocialCommunication()->toString("").c_str());
    }

    auto error = m_serializer.serializeWithDelimiter(commWrap, *buffer);
    if (error)
    {
        RTMErrorResponse::post(m_hub, kRTMErrorResponseJobName, errorCallback, error);
        return;
    }

    auto sendJob = m_hub->getAllocator().makeUnique<eadp::realtimemessaging::SSLSocketSendJob>(m_hub, m_socket, buffer, errorCallback);
    m_hub->addJob(EADPSDK_MOVE(sendJob));
}

void RTMServiceImpl::queueReconnectionData(const SharedPtr<eadp::realtimemessaging::CommunicationWrapper>& commWrap, RTMErrorCallback errorCallback)
{
    m_queueReconnectionData.emplace_back(commWrap, errorCallback);
}

void RTMServiceImpl::processCommunication(SharedPtr<eadp::realtimemessaging::CommunicationWrapper> commWrapper)
{
    if (commWrapper->hasRtmCommunication())
    {
        // Process RTM Communication
        auto rtmCommunication = commWrapper->getRtmCommunication();
        EADPSDK_LOGV(m_hub, "RTMService", "Received RTM Proto : %s", rtmCommunication->toString("").c_str());
        if (rtmCommunication->hasV1())
        {
            String requestId = rtmCommunication->getV1()->getRequestId();
            if (requestId.empty())
            {
                if (rtmCommunication->getV1()->hasHeartbeat())
                {
                    // Ignore heartbeat responses
                }
                else if (rtmCommunication->getV1()->hasReconnectRequest())
                {
                    // Reconnect
                    setState(ConnectionState::DISCONNECTED);
                    closeSocketConnection(false);
                    ConnectionService::DisconnectSource source{ ConnectionService::DisconnectSource::Reason::RECONNECT_REQUEST };
                    notifyServiceDisconnected(source, nullptr);
                }
                else if (rtmCommunication->getV1()->hasSessionNotification()
                    && rtmCommunication->getV1()->getSessionNotification()->getType() == rtm::protocol::SessionNotificationTypeV1::FORCE_DISCONNECT)
                {
                    setState(ConnectionState::DISCONNECTED);
                    closeSocketConnection(false);
                    ConnectionService::DisconnectSource source
                    {
                        ConnectionService::DisconnectSource::Reason::FORCE_DISCONNECT,
                        rtmCommunication->getV1()->getSessionNotification()->getMessage()
                    };
                    notifyServiceDisconnected(source, nullptr);
                }
                else
                {
                    // notify RTM Communication Handler
                    m_rtmCallbackList->respond<RTMCommunicationResponse>(kRTMCommunicationResponseJobname, commWrapper);
                }
            }
            else
            {
                delegateResponse(requestId, commWrapper);
            }
        }
    }
    else
    {
        // Process Social Communication
        auto socialCommunication = commWrapper->getSocialCommunication();
        EADPSDK_LOGV(m_hub, "RTMService", "Received Social Proto : %s", socialCommunication->toString("").c_str());

        if (socialCommunication->hasHeader())
        {
            String requestId = socialCommunication->getHeader()->getRequestId();
            if (requestId.empty())
            {
                // notify RTM Communication Handler
                m_rtmCallbackList->respond<RTMCommunicationResponse>(kRTMCommunicationResponseJobname, commWrapper);
            }
            else
            {
                delegateResponse(requestId, commWrapper);
            }
        }
    }
}

void RTMServiceImpl::delegateResponse(String requestId, SharedPtr<eadp::realtimemessaging::CommunicationWrapper> communication)
{
    auto iterator = m_requestManager.find(requestId);
    if (iterator == m_requestManager.end())
    {
        EADPSDK_LOGE(m_hub, "RTMService", "No request found with RequestId (%s). Dropping Response", requestId.c_str());
    }
    else
    {
        RTMCommunicationResponse::post(m_hub, kRTMRequestCompleteResponseJobname, iterator->second, communication);
        m_requestManager.erase(iterator);
    }
}

void RTMServiceImpl::removeRequest(String requestId)
{
    m_requestManager.erase(requestId);
}

String RTMServiceImpl::generateRequestId()
{
    String requestId = m_hub->getAllocator().emptyString();
    eadp::foundation::Internal::Utils::appendSprintf(requestId, "%d", m_requestIdCounter);

    m_requestIdCounter++;   // Increment counter for next time

    return requestId;
}

void RTMServiceImpl::resetRequestIdCounter()
{
    m_requestIdCounter = REQUEST_ID_COUNTER_START;
    m_requestManager.clear();
}

bool RTMServiceImpl::isRTMCommunicationReady()
{
    EA::Thread::AutoFutex lock(m_connectionStateLock);
    return m_connectionState == ConnectionState::CONNECTED;
}

void RTMServiceImpl::reinitialize()
{
    m_persistObj.reset();
    m_persistObj = m_hub->getAllocator().makeShared<Callback::Persistence>();

    m_connectionState = ConnectionState::DISCONNECTED;

    m_queueReconnectionData.clear();

    resetRequestIdCounter();

    m_socket.reset();
    m_socket = m_hub->getAllocator().makeShared<eadp::realtimemessaging::SSLSocket>(m_hub);

    m_data = m_hub->getAllocator().emptyString();
}

/*************************************************************************
 * Web Socket
 *************************************************************************/

void RTMServiceImpl::openSocketConnection()
{
    auto onConnectCallback = [this](const ErrorPtr& error)
    {
        onSocketConnect(error);
    };

    auto onDisconnectCallback = [this](const ErrorPtr& error)
    {
        onSocketDisconnect(error);
    };

    auto onDataCallback = [this](UniquePtr<eadp::foundation::Internal::RawBuffer> data)
    {
        onSocketData(EADPSDK_MOVE(data));
    };

    auto connectJob = m_hub->getAllocator().makeUnique<SSLSocketConnectJob>(m_hub,
        m_socket,
        m_hub->getAllocator().makeShared<eadp::foundation::Internal::Url>(m_hub->getAllocator(), DIRECTOR_KEY_RTM_SERVICE, ""),
        eadp::realtimemessaging::SSLSocket::SSLSocketConnectCallback(onConnectCallback, m_persistObj, Callback::kIdleInvokeGroupId),
        eadp::realtimemessaging::SSLSocket::SSLSocketDisconnectCallback(onDisconnectCallback, m_persistObj, Callback::kIdleInvokeGroupId),
        eadp::realtimemessaging::SSLSocket::SSLSocketDataCallback(onDataCallback, m_persistObj, Callback::kIdleInvokeGroupId));

    m_hub->addJob(EADPSDK_MOVE(connectJob));
}

void RTMServiceImpl::closeSocketConnection(bool sendLogout)
{
    if (sendLogout && getState() == ConnectionState::CONNECTED)
    {
        sendLogoutRequest();
    }

    if (m_socket)
    {
        auto closeJob = m_hub->getAllocator().makeUnique<eadp::realtimemessaging::SSLSocketCloseJob>(m_hub, m_socket);
        m_hub->addJob(EADPSDK_MOVE(closeJob));
    }
}

void RTMServiceImpl::onSocketConnect(const ErrorPtr& error)
{
    if (error)
    {
        setState(ConnectionState::DISCONNECTED);
        closeSocketConnection(false);
        notifyServiceConnected(nullptr, error);
        m_queueReconnectionData.clear();
    }
    else
    {
        // Socket Connect send login request
        sendLoginRequest();
    }
}

void RTMServiceImpl::onSocketDisconnect(const ErrorPtr& error)
{
    switch (getState())
    {
    case ConnectionState::CONNECTED:
    {
        setState(ConnectionState::DISCONNECTED);
        closeSocketConnection(false);
        m_queueReconnectionData.clear();
        ConnectionService::DisconnectSource source{ ConnectionService::DisconnectSource::Reason::CONNECTION_ERROR };
        notifyServiceDisconnected(source, error);
        break;
    }
    case ConnectionState::RECONNECTING:
    {
        setState(ConnectionState::DISCONNECTED);
        notifyServiceConnected(nullptr, error);
        break;
    }
    case ConnectionState::CONNECTING:
    {
        setState(ConnectionState::DISCONNECTED);
        notifyServiceConnected(nullptr, error);
        break;
    }
    case ConnectionState::DISCONNECTED:
    {
        break;
    }
    default:
        break;
    }
}

void RTMServiceImpl::onSocketData(UniquePtr<eadp::foundation::Internal::RawBuffer> data)
{
    m_data.append((char8_t*)(data->getBytes()), data->getSize());

    int32_t bytesRead = 0, totalBytesRead = 0;
    do
    {
        SharedPtr<eadp::realtimemessaging::CommunicationWrapper> commWrapper;
        bytesRead = 0;
        auto error = m_serializer.parseDataIntoCommunication((uint8_t*)m_data.data() + totalBytesRead, (uint32_t)m_data.length() - totalBytesRead, commWrapper, bytesRead);
        if (!error && bytesRead > 0)
        {
            totalBytesRead += bytesRead;
            processCommunication(commWrapper);
        }
    } while (bytesRead > 0);

    m_data.erase(0, totalBytesRead);
    return;
}


/*************************************************************************
 * Login Request
 *************************************************************************/

void RTMServiceImpl::sendLoginRequest()
{
    bool isReconnect = getState() == ConnectionState::RECONNECTING;
    auto loginResponseCallback = [this, isReconnect](SharedPtr<rtm::protocol::LoginV2Success> loginSuccess, const ErrorPtr& error)
    {
        onLoginResponse(EADPSDK_MOVE(loginSuccess), error, isReconnect);
    };

    auto loginRequestJob = m_hub->getAllocator().makeUnique<eadp::realtimemessaging::LoginRequestJob>(m_hub,
        shared_from_this(),
        m_productId,
        m_accessToken,
        isReconnect,
        m_forceDisconnect,
        eadp::realtimemessaging::LoginRequestJob::Callback(loginResponseCallback, m_persistObj, Callback::kIdleInvokeGroupId));

    m_hub->addJob(EADPSDK_MOVE(loginRequestJob));
}

void RTMServiceImpl::onLoginResponse(SharedPtr<rtm::protocol::LoginV2Success> loginSuccess, const ErrorPtr& error, bool isReconnect)
{
    if (error)
    {
        closeSocketConnection(false);
        setState(ConnectionState::DISCONNECTED);
        notifyServiceConnected(nullptr, error);
        m_queueReconnectionData.clear();
    }
    else
    {
        setState(ConnectionState::CONNECTED);
        notifyServiceConnected(loginSuccess, nullptr);

        if (isReconnect)
        {
            completeReconnection();
        }
    }
}

void RTMServiceImpl::completeReconnection()
{
    EADPSDK_LOGV(m_hub, "RTMService", "Sending queued Reconnection data");

    for (QueuedReconnectionData reconnectionData : m_queueReconnectionData)
    {
        sendCommunicationOnSocket(reconnectionData.communication, reconnectionData.callback);
    }

    m_queueReconnectionData.clear();
}

/*************************************************************************
 * Logout Request
 *************************************************************************/

void RTMServiceImpl::sendLogoutRequest()
{
    // Create loginRequestV2 Protobuf
    auto logoutRequest = m_hub->getAllocator().makeShared<protocol::LogoutRequest>(m_hub->getAllocator());

    auto header = m_hub->getAllocator().makeShared<protocol::Header>(m_hub->getAllocator());
    header->setType(protocol::CommunicationType::LOGOUT_REQUEST);

    auto communication = m_hub->getAllocator().makeShared<protocol::Communication>(m_hub->getAllocator());
    communication->setHeader(header);
    communication->setLogoutRequest(logoutRequest);

    auto commWrapper = m_hub->getAllocator().makeShared<eadp::realtimemessaging::CommunicationWrapper>(communication);

    sendCommunicationOnSocket(commWrapper, nullptr);
}

/*************************************************************************
 * Connection State
 *************************************************************************/

RTMServiceImpl::ConnectionState RTMServiceImpl::getState()
{
    EA::Thread::AutoFutex lock(m_connectionStateLock);
    return m_connectionState;
}

void RTMServiceImpl::setState(ConnectionState state)
{
    EA::Thread::AutoFutex lock(m_connectionStateLock);
    m_connectionState = state;
}

void RTMServiceImpl::notifyServiceConnected(SharedPtr<rtm::protocol::LoginV2Success> loginSuccess, const ErrorPtr& error)
{
    RTMConnectResponse::post(m_hub, kRTMConnectResponseJobName, m_connectCallback, loginSuccess, error);
    m_connectCallback = nullptr;
}

void RTMServiceImpl::notifyServiceDisconnected(eadp::realtimemessaging::ConnectionService::DisconnectSource source, const ErrorPtr& error)
{
    RTMDisconnectResponse::post(m_hub, kRTMDisconnectResponseJobName, m_disconnectCallback, source, error);
}

