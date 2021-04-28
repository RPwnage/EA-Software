// Copyright(C) 2019-2020 Electronic Arts, Inc. All rights reserved.

#include "RTMServiceImpl.h"
#include "LoginRequestV2Job.h"
#include "LoginRequestV3Job.h"
#include "LogoutRequestJob.h"
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
constexpr char kRTMRequestCompleteRequestJobName[] = "RTMRequestCompleteRequestJob";
constexpr char kRTMCommunicationRequestJobName[] = "RTMCommunicationRequestJob";

using RTMConnectResponseV2 = ResponseT<eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginV2Success>, const ErrorPtr&>;
constexpr char kRTMConnectRequestJobName[] = "RTMConnectRequestV2Job";

using RTMConnectResponseV3 = ResponseT<SharedPtr<rtm::protocol::LoginV3Response>, SharedPtr<rtm::protocol::LoginErrorV1>, const ErrorPtr&>;
constexpr char kRTMConnectRequestV3JobName[] = "RTMConnectRequestV3Job";

using RTMDisconnectResponse = ResponseT<const eadp::realtimemessaging::ConnectionService::DisconnectSource&, const ErrorPtr&>;
constexpr char kRTMDisconnectRequestJobName[] = "RTMDisconnectRequestJob";

using RTMUserSessionChange = ResponseT<SharedPtr<rtm::protocol::SessionNotificationV1>, const ErrorPtr&>;
constexpr char kRTMUserSessionChange[] = "RTMUserSessionChange";

RTMServiceImpl::RTMServiceImpl(IHub* hub, String productId)
: m_hub(hub)
, m_productId(EADPSDK_MOVE(productId))
, m_accessToken(hub->getAllocator().emptyString())
, m_connectCallback(nullptr)
, m_multiConnectCallback(nullptr)
, m_disconnectCallback(nullptr)
, m_userSessionChangeCallback(nullptr)
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

RTMServiceImpl::~RTMServiceImpl()
{
    EA_ASSERT(m_connectionStateLock.GetLockCount() == 0);
}

/*************************************************************************
 * Connection Service
 *************************************************************************/
void RTMServiceImpl::connect(eadp::foundation::String accessToken, eadp::foundation::String clientVersion, ConnectionService::ConnectCallback connectCb, ConnectionService::DisconnectCallback disconnectCb, bool forceDisconnect)
{
    connectHelper(false, accessToken, clientVersion, connectCb, nullptr, disconnectCb, nullptr, forceDisconnect, false, "");
}
void RTMServiceImpl::reconnect(eadp::foundation::String accessToken, eadp::foundation::String clientVersion, ConnectionService::ConnectCallback connectCb, ConnectionService::DisconnectCallback disconnectCb, bool forceDisconnect)
{
    connectHelper(false, accessToken, clientVersion, connectCb, nullptr, disconnectCb, nullptr, forceDisconnect, false, "");
}

void RTMServiceImpl::connect(String accessToken, String clientVersion, eadp::realtimemessaging::ConnectionService::MultiConnectCallback multiConnectCb, eadp::realtimemessaging::ConnectionService::DisconnectCallback disconnectCb, eadp::realtimemessaging::ConnectionService::UserSessionChangeCallback userSessionChangeCb, bool forceDisconnect, eadp::foundation::String sessionKey)
{
    connectHelper(true, accessToken, clientVersion, nullptr, multiConnectCb, disconnectCb, userSessionChangeCb, forceDisconnect, false, sessionKey);
}

void RTMServiceImpl::reconnect(String accessToken, String clientVersion, eadp::realtimemessaging::ConnectionService::MultiConnectCallback multiConnectCb, eadp::realtimemessaging::ConnectionService::DisconnectCallback disconnectCb, eadp::realtimemessaging::ConnectionService::UserSessionChangeCallback userSessionChangeCb, bool forceDisconnect, eadp::foundation::String sessionKey)
{
    connectHelper(true, accessToken, clientVersion, nullptr, multiConnectCb, disconnectCb, userSessionChangeCb, forceDisconnect, true, sessionKey);
}

void RTMServiceImpl::connectHelper(bool isMultiSession, String accessToken, String clientVersion, ConnectionService::ConnectCallback connectCb, eadp::realtimemessaging::ConnectionService::MultiConnectCallback multiConnectCb, eadp::realtimemessaging::ConnectionService::DisconnectCallback disconnectCb, eadp::realtimemessaging::ConnectionService::UserSessionChangeCallback userSessionChangeCb, bool forceDisconnect, bool isReconnect, String sessionKey)
{
    m_accessToken = accessToken;
    m_clientVersion = clientVersion;
    m_disconnectCallback = disconnectCb;
    m_userSessionChangeCallback = userSessionChangeCb;
    m_connectCallback = connectCb;
    m_multiConnectCallback = multiConnectCb;
    m_forceDisconnect = forceDisconnect;
    m_sessionKey = sessionKey;

    switch (getState())
    {
    case ConnectionState::CONNECTED:
    {
        isMultiSession ? notifyServiceConnectedV3(nullptr, nullptr) : notifyServiceConnectedV2(nullptr, nullptr);
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
        openSocketConnection(isMultiSession);
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
        isMultiSession ? notifyServiceConnectedV3(nullptr, error) : notifyServiceConnectedV2(nullptr, error);
        break;
    }
    }
}

void RTMServiceImpl::disconnect()
{
    m_disconnectCallback = nullptr;
    m_connectCallback = nullptr;
    m_forceDisconnect = true;

    auto logoutCallback = [this]()
    {
        // Close the socket regardless of an error occurring
        closeSocketConnection();
        setState(ConnectionState::DISCONNECTED);
        reinitialize();
    };

    auto logoutJob = m_hub->getAllocator().makeUnique<LogoutRequestJob>(m_hub, 
                                                                        shared_from_this(),
                                                                        LogoutRequestJob::Callback(logoutCallback, m_persistObj, Callback::kIdleInvokeGroupId));
    m_hub->addJob(EADPSDK_MOVE(logoutJob)); 
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
                    closeSocketConnection();
                    ConnectionService::DisconnectSource source{ ConnectionService::DisconnectSource::Reason::RECONNECT_REQUEST };
                    notifyServiceDisconnected(source, nullptr);
                }
                else if (rtmCommunication->getV1()->hasSessionNotification()
                    && rtmCommunication->getV1()->getSessionNotification()->getType() == rtm::protocol::SessionNotificationTypeV1::FORCE_DISCONNECT)
                {
                    setState(ConnectionState::DISCONNECTED);
                    closeSocketConnection();
                    ConnectionService::DisconnectSource source
                    {
                        ConnectionService::DisconnectSource::Reason::FORCE_DISCONNECT,
                        rtmCommunication->getV1()->getSessionNotification()->getMessage()
                    };
                    notifyServiceDisconnected(source, nullptr);
                }
                else if (rtmCommunication->getV1()->hasSessionNotification()
                    && (rtmCommunication->getV1()->getSessionNotification()->getType() == rtm::protocol::SessionNotificationTypeV1::USER_LOGGED_IN
                        || rtmCommunication->getV1()->getSessionNotification()->getType() == rtm::protocol::SessionNotificationTypeV1::USER_LOGGED_OUT))
                {
                    notifyUserSessionChange(rtmCommunication->getV1()->getSessionNotification(), nullptr);
                }
                else
                {
                    // notify RTM Communication Handler
                    m_rtmCallbackList->respond<RTMCommunicationResponse>(kRTMCommunicationRequestJobName, commWrapper);
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
                m_rtmCallbackList->respond<RTMCommunicationResponse>(kRTMCommunicationRequestJobName, commWrapper);
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
        RTMCommunicationResponse::post(m_hub, kRTMRequestCompleteRequestJobName, iterator->second, communication);
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

void RTMServiceImpl::openSocketConnection(bool isMultiSession)
{
    auto onConnectCallback = [isMultiSession, this](const ErrorPtr& error)
    {
        onSocketConnect(isMultiSession, error);
    };

    auto onDisconnectCallback = [isMultiSession, this](const ErrorPtr& error)
    {
        onSocketDisconnect(isMultiSession, error);
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

void RTMServiceImpl::closeSocketConnection()
{
    if (m_socket)
    {
        auto closeJob = m_hub->getAllocator().makeUnique<eadp::realtimemessaging::SSLSocketCloseJob>(m_hub, m_socket);
        m_hub->addJob(EADPSDK_MOVE(closeJob));
    }
}

void RTMServiceImpl::onSocketConnect(bool isMultiSession, const ErrorPtr& error)
{
    if (error)
    {
        setState(ConnectionState::DISCONNECTED);
        closeSocketConnection();
        isMultiSession ? notifyServiceConnectedV3(nullptr, error) : notifyServiceConnectedV2(nullptr, error);
        m_queueReconnectionData.clear();
    }
    else
    {
        // Socket Connect send login request
        isMultiSession ? sendLoginRequestV3() : sendLoginRequestV2(); 
    }
}

void RTMServiceImpl::onSocketDisconnect(bool isMultiSession, const ErrorPtr& error)
{
    switch (getState())
    {
    case ConnectionState::CONNECTED:
    {
        setState(ConnectionState::DISCONNECTED);
        closeSocketConnection();
        m_queueReconnectionData.clear();
        ConnectionService::DisconnectSource source{ ConnectionService::DisconnectSource::Reason::CONNECTION_ERROR };
        notifyServiceDisconnected(source, error);
        break;
    }
    case ConnectionState::RECONNECTING:
    {
        setState(ConnectionState::DISCONNECTED);
        isMultiSession ? notifyServiceConnectedV3(nullptr, error) : notifyServiceConnectedV2(nullptr, error);
        break;
    }
    case ConnectionState::CONNECTING:
    {
        setState(ConnectionState::DISCONNECTED);
        isMultiSession ? notifyServiceConnectedV3(nullptr, error) : notifyServiceConnectedV2(nullptr, error);
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
 * Login Requests
 *************************************************************************/

void RTMServiceImpl::sendLoginRequestV2()
{
    bool isReconnect = getState() == ConnectionState::RECONNECTING;
    auto loginResponseCallback = [this, isReconnect](SharedPtr<rtm::protocol::LoginV2Success> loginResponse, const ErrorPtr& error)
    {
        onLoginResponseV2(EADPSDK_MOVE(loginResponse), error, isReconnect);
    };

    auto loginRequestJob = m_hub->getAllocator().makeUnique<eadp::realtimemessaging::LoginRequestV2Job>(m_hub,
        shared_from_this(),
        m_productId,
        m_accessToken,
        isReconnect,
        m_forceDisconnect,
        eadp::realtimemessaging::LoginRequestV2Job::Callback(loginResponseCallback, m_persistObj, Callback::kIdleInvokeGroupId));

    m_hub->addJob(EADPSDK_MOVE(loginRequestJob));
}

void RTMServiceImpl::sendLoginRequestV3()
{
    bool isReconnect = getState() == ConnectionState::RECONNECTING;
    auto loginResponseCallback = [this, isReconnect](SharedPtr<rtm::protocol::LoginV3Response> loginResponse, SharedPtr<rtm::protocol::LoginErrorV1> loginError, const ErrorPtr& error)
    {
        onLoginResponseV3(EADPSDK_MOVE(loginResponse), error, isReconnect);
    };
    
    auto loginRequestJob = m_hub->getAllocator().makeUnique<eadp::realtimemessaging::LoginRequestV3Job>(m_hub,
        shared_from_this(),
        m_productId,
        m_clientVersion,
        m_accessToken,
        isReconnect,
        m_forceDisconnect,
        m_sessionKey,
        eadp::realtimemessaging::LoginRequestV3Job::Callback(loginResponseCallback, m_persistObj, Callback::kIdleInvokeGroupId));

    m_hub->addJob(EADPSDK_MOVE(loginRequestJob));
}

void RTMServiceImpl::onLoginResponseV2(SharedPtr<rtm::protocol::LoginV2Success> loginResponse, const ErrorPtr& error, bool isReconnect)
{
    if (error)
    {
        closeSocketConnection();
        setState(ConnectionState::DISCONNECTED);
        notifyServiceConnectedV2(nullptr, error);
        m_queueReconnectionData.clear();
    }
    else
    {
        setState(ConnectionState::CONNECTED);
        notifyServiceConnectedV2(loginResponse, nullptr);
        if (isReconnect)
        {
            completeReconnection();
        }
    }
}

void RTMServiceImpl::onLoginResponseV3(SharedPtr<rtm::protocol::LoginV3Response> loginResponse, const ErrorPtr& error, bool isReconnect)
{
    if (error)
    {
        closeSocketConnection();
        setState(ConnectionState::DISCONNECTED);
        notifyServiceConnectedV3(nullptr, error);
        m_queueReconnectionData.clear();
    }
    else
    {
        setState(ConnectionState::CONNECTED);
        notifyServiceConnectedV3(loginResponse, nullptr);
        m_sessionKey = loginResponse->getSessionKey();
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

void RTMServiceImpl::notifyServiceConnectedV2(SharedPtr<rtm::protocol::LoginV2Success> loginResponse, const ErrorPtr& error)
{
    RTMConnectResponseV2::post(m_hub, kRTMConnectRequestJobName, m_connectCallback, loginResponse, error);
    m_connectCallback = nullptr;
}

void RTMServiceImpl::notifyServiceConnectedV3(SharedPtr<rtm::protocol::LoginV3Response> loginResponse, const ErrorPtr& error)
{
    RTMConnectResponseV3::post(m_hub, kRTMConnectRequestJobName, m_multiConnectCallback, loginResponse, nullptr, error);
    m_multiConnectCallback = nullptr;
}

void RTMServiceImpl::notifyServiceDisconnected(eadp::realtimemessaging::ConnectionService::DisconnectSource source, const ErrorPtr& error)
{
    RTMDisconnectResponse::post(m_hub, kRTMDisconnectRequestJobName, m_disconnectCallback, source, error);
}

void RTMServiceImpl::notifyUserSessionChange(SharedPtr<rtm::protocol::SessionNotificationV1> notification, const ErrorPtr& error)
{
    RTMUserSessionChange::post(m_hub, kRTMUserSessionChange, m_userSessionChangeCallback, notification, error);    
}

