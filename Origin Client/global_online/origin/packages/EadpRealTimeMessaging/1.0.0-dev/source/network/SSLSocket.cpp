// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved.

#include "SSLSocket.h"
#include <eadp/foundation/internal/ResponseT.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/foundation/internal/Utils.h>
#include EA_ASSERT_HEADER

using namespace eadp::realtimemessaging;
using namespace eadp::foundation;

using SSLSocketDataResponse = ResponseT<UniquePtr<eadp::foundation::Internal::RawBuffer>>;
constexpr char kSSLSocketDataResponseJobName[] = "SSLSocketDataResponseJob";
constexpr char kSSLSocketConnectResponseJobName[] = "SSLSocketConnectResponseJob";
constexpr char kSSLSocketDisconnectResponseJobName[] = "SSLSocketDisconnectResponseJob";
constexpr char kSSLSocketSendResponseJobName[] = "SSLSocketSendResponseJob";


SSLSocket::SSLSocket(IHub* hub) 
: m_hub(hub)
, m_protosocket(nullptr)
, m_dataCallback(nullptr)
, m_connectCallback(nullptr)
, m_disconnectCallback(nullptr)
, m_state(State::DISCONNECTED)
, m_buffer(hub->getAllocator(), kBufSize)
, m_queuedData(hub->getAllocator().make<Queue<SharedPtr<eadp::foundation::Internal::RawBuffer>>>())
, m_lastUnsentByte(0)
{
}

SSLSocket::~SSLSocket()
{
    if (m_protosocket != nullptr)
    {
        ProtoSSLDestroy(m_protosocket);
        m_protosocket = nullptr;
    }
}

SSLSocket::State SSLSocket::getState() const
{
    return m_state;
}

void SSLSocket::connect(String url,
    const SSLSocketConnectCallback& connectCb,
    const SSLSocketDisconnectCallback& disconnectCb,
    const SSLSocketDataCallback&  dataCb)
{
    if (url.empty())
    {
        EADPSDK_LOGE(m_hub, "SSLSocket", "Invalid parameter. URL Empty.");
        auto error = RealTimeMessagingError::create(m_hub->getAllocator(), RealTimeMessagingError::Code::PROTO_SOCKET_ERROR, "Invalid parameter. URL Empty.");
        ErrorResponseT::post(m_hub, kSSLSocketConnectResponseJobName, connectCb, error);
        return;
    }

    // Remove "https://" prefix, if exists
    String prefix = "https://";
    size_t pos = url.find(prefix);
    if (pos == 0)
    {
        url.erase(0, prefix.length());
    }

    if (m_state == State::CONNECTED)
    {
        // if already connected
        EADPSDK_LOGE(m_hub, "SSLSocket", "Calling connect on already connected socket");
        auto error = RealTimeMessagingError::create(m_hub->getAllocator(),
            RealTimeMessagingError::Code::PROTO_SOCKET_ERROR,
            "Calling connect on already connected socket. Call close() before calling connect() again.");
        ErrorResponseT::post(m_hub, kSSLSocketConnectResponseJobName, connectCb, error);
        return;
    }

    if (m_protosocket == nullptr)
    {
        m_protosocket = ProtoSSLCreate();
    }

    if (m_protosocket == nullptr)
    {
        EADPSDK_LOGE(m_hub, "SSLSocket", "Construction of ProtoSSL socket failed");
        auto error = RealTimeMessagingError::create(m_hub->getAllocator(), RealTimeMessagingError::Code::PROTO_SOCKET_ERROR, "ProtoSSL socket construction failed");
        ErrorResponseT::post(m_hub, kSSLSocketConnectResponseJobName, connectCb, error);
        return;
    }

    // m_state == State::CONNECTING, means we are already connecting, we just update the callbacks and return
    // Handle m_state == State::DISCONNECTED only
    if (m_state == State::DISCONNECTED)
    {
        // if Disconnected, start Connection
        int conStat = ProtoSSLConnect(m_protosocket, 1 /* Secure */, url.data(), 0, 0);

        if (conStat != 0)
        {
            String description = m_hub->getAllocator().emptyString();
            eadp::foundation::Internal::Utils::appendSprintf(description, "Connection of ProtoSSL socket failed with error %d for url %s", conStat, url.c_str());
            EADPSDK_LOGE(m_hub, "SSLSocket", description);
            auto error = RealTimeMessagingError::create(m_hub->getAllocator(), RealTimeMessagingError::Code::PROTO_SOCKET_ERROR, description);
            ErrorResponseT::post(m_hub, kSSLSocketConnectResponseJobName, connectCb, error);
            return;
        }
    }

    m_connectCallback = connectCb;
    m_disconnectCallback = disconnectCb;
    m_dataCallback = dataCb;

    m_state = State::CONNECTING;
    return;
}

void SSLSocket::send(const SharedPtr<eadp::foundation::Internal::RawBuffer> data, SSLSocketErrorCallback callback)
{
    if (m_state != State::CONNECTED)
    {
        String description = m_hub->getAllocator().emptyString();
        eadp::foundation::Internal::Utils::appendSprintf(description, "Send Requested on SSLSocket which is not connected");
        EADPSDK_LOGE(m_hub, "SSLSocket", description);
        auto error = FoundationError::create(m_hub->getAllocator(), FoundationError::Code::INVALID_STATUS, description);
        ErrorResponseT::post(m_hub, kSSLSocketSendResponseJobName, callback, error);
        return;
    }

    if (!data)
    {
        String description = m_hub->getAllocator().emptyString();
        eadp::foundation::Internal::Utils::appendSprintf(description, "Data value is NULL");
        EADPSDK_LOGE(m_hub, "SSLSocket", description);
        auto error = FoundationError::create(m_hub->getAllocator(), FoundationError::Code::INVALID_ARGUMENT, description);
        ErrorResponseT::post(m_hub, kSSLSocketSendResponseJobName, callback, error);
        return;
    }

    EA_ASSERT_MSG((m_protosocket != nullptr), "SSLSocket::send() : protosocket object is null");

    m_queuedData.push_back(data);

    ErrorResponseT::post(m_hub, kSSLSocketSendResponseJobName, callback, nullptr);
}

void SSLSocket::update()
{
    if (m_state == State::DISCONNECTED)
    {
        return;
    }

    if (m_protosocket == nullptr)
    {
        return;
    }

    // Update ProtoSSLSocket
    ProtoSSLUpdate(m_protosocket);

    processState();

    if (m_state == State::CONNECTED)   // if Connected, try to read data from socket
    {
        int readLen = ProtoSSLRecv(m_protosocket, (char *)m_buffer.getBytes(), m_buffer.getSize());

        // readLen == 0, no data to read, wait till next update() invocation to read again
        if (readLen > 0)
        {
            UniquePtr<eadp::foundation::Internal::RawBuffer> buffer = m_hub->getAllocator().makeUnique<eadp::foundation::Internal::RawBuffer>(m_hub->getAllocator(), readLen);
            memcpy(buffer->getBytes(), m_buffer.getBytes(), readLen);
            SSLSocketDataResponse::post(m_hub, kSSLSocketDataResponseJobName, m_dataCallback, EADPSDK_MOVE(buffer));
        }
        else if (readLen < 0)
        {
            // There was an error reading the data. Close the connection
            String description = m_hub->getAllocator().emptyString();
            eadp::foundation::Internal::Utils::appendSprintf(description, "Socket Disconnected due to error while reading data. Reference : %d", readLen);
            EADPSDK_LOGE(m_hub, "SSLSocket", description);
            auto error = RealTimeMessagingError::create(m_hub->getAllocator(), RealTimeMessagingError::Code::PROTO_SOCKET_ERROR, description);
            onClose(error);
        }

        // Try sending queued data. Store the position of the last unsent byte of front element in m_lastUnsentByte
        while (!m_queuedData.empty())
        {
            auto front = m_queuedData.front();
            int32_t bytesSent = ProtoSSLSend(m_protosocket, (char*)(front->getBytes() + m_lastUnsentByte), front->getSize() - m_lastUnsentByte);
            if (bytesSent < 0)
            {
                // Error while sending data. Close the connection
                String disconnectDescription = m_hub->getAllocator().emptyString();
                eadp::foundation::Internal::Utils::appendSprintf(disconnectDescription, "Socket Disconnected due to error while sending data. Reference : %d", bytesSent);
                EADPSDK_LOGE(m_hub, "SSLSocket", disconnectDescription);
                auto disconnectError = RealTimeMessagingError::create(m_hub->getAllocator(), RealTimeMessagingError::Code::PROTO_SOCKET_ERROR, disconnectDescription);
                onClose(disconnectError);
                return;
            }
            else if (bytesSent < (int32_t)(front->getSize() - m_lastUnsentByte))
            {
                // Update m_lastUnsentByte position and break the loop because we cannot send any further data
                m_lastUnsentByte += bytesSent;
                break;
            }
            m_lastUnsentByte = 0;
            m_queuedData.pop_front();     // Remove element from the front because data has been sent
        }
    }
}

void SSLSocket::processState()
{
    int protoSSLState = ProtoSSLStat(m_protosocket, 'stat', NULL, 0);

    // If no state change, return
    if (protoSSLState == (int)m_state)
    {
        return;
    }

    EA_ASSERT_FORMATTED((protoSSLState >= -1 && protoSSLState <= 1), ("SSLSocket::processState() : Unknown: %d", protoSSLState));

    if (m_state == State::CONNECTING)
    {
        // Call m_connectCallback on success or failure
        ErrorPtr error;
        if (protoSSLState == (int)State::DISCONNECTED)
        {
            int reason = ProtoSSLStat(m_protosocket, 'fail', NULL, 0);
            String description = m_hub->getAllocator().emptyString();
            eadp::foundation::Internal::Utils::appendSprintf(description, "SSLSocket connection failed. Disconnect Reason (%d)", reason);
            error = RealTimeMessagingError::create(m_hub->getAllocator(), RealTimeMessagingError::Code::PROTO_SOCKET_ERROR, description);
        }
        ErrorResponseT::post(m_hub, kSSLSocketConnectResponseJobName, m_connectCallback, error);
    }
    else if (m_state == State::CONNECTED)
    {
        // Connect m_disconnectcallback on failure
        if (protoSSLState == (int)State::DISCONNECTED)
        {
            int reason = ProtoSSLStat(m_protosocket, 'fail', NULL, 0);
            String description = m_hub->getAllocator().emptyString();
            eadp::foundation::Internal::Utils::appendSprintf(description, "SSLSocket disconnected. Disconnect Reason (%d)", reason);
            ErrorPtr error = RealTimeMessagingError::create(m_hub->getAllocator(), RealTimeMessagingError::Code::PROTO_SOCKET_ERROR, description);
            onClose(error);
        }
    }

    m_state = (State)protoSSLState;
}

void SSLSocket::close()
{
    closeConnection();
}

void SSLSocket::closeConnection()
{
    if (m_state != State::DISCONNECTED)
    {
        ProtoSSLDisconnect(m_protosocket);
        m_state = State::DISCONNECTED;
    }
}

void SSLSocket::onClose(const eadp::foundation::ErrorPtr& error)
{
    closeConnection();
    notifyDisconnect(error);
}

void SSLSocket::notifyDisconnect(const eadp::foundation::ErrorPtr& error)
{
    ErrorResponseT::post(m_hub, kSSLSocketDisconnectResponseJobName, m_disconnectCallback, error);
}

/***********************************************************
 * SSLSocketConnectJob
 ***********************************************************/

SSLSocketConnectJob::SSLSocketConnectJob(IHub* hub,
    SharedPtr<SSLSocket> socket,
    SharedPtr<eadp::foundation::Internal::Url> url,
    SSLSocket::SSLSocketConnectCallback connectCb,
    SSLSocket::SSLSocketDisconnectCallback disconnectCb,
    SSLSocket::SSLSocketDataCallback dataCb) 
: m_hub(hub)
, m_socket(socket)
, m_url(url)
, m_state(State::INIT)
, m_dataCallback(dataCb)
, m_connectCallback(connectCb)
, m_disconnectCallback(disconnectCb)
, m_directorService(DirectorService::getService(hub))
{
}

SSLSocketConnectJob::~SSLSocketConnectJob()
{
}

void SSLSocketConnectJob::execute()
{
    switch (m_state)
    {
    case State::INIT:
    {
        if (m_url->update(m_directorService))
        {
            if (m_url->getUrlString().empty())
            {
                m_done = true;

                String description = m_hub->getAllocator().emptyString();
                eadp::foundation::Internal::Utils::appendSprintf(description, "Fail to retrieve the base url with configuration key %s.", m_url->getConfigurationKey().c_str());
                EADPSDK_LOGE(m_hub, "SSLSocket", description);
                auto error = FoundationError::create(m_hub->getAllocator(), FoundationError::Code::INVALID_ARGUMENT, description);
                ErrorResponseT::post(m_hub, kSSLSocketConnectResponseJobName, m_connectCallback, error);
                return;
            }
            else
            {
                m_socket->connect(m_url->getUrlString(), m_connectCallback, m_disconnectCallback, m_dataCallback);
                m_state = State::CONNECT_CALLED;
            }
        }
        break;
    }
    case State::CONNECT_CALLED:
    {
        // Keep updating the socket so it can perform its work
        m_socket->update();
        if (m_socket->getState() == SSLSocket::State::DISCONNECTED)
        {
            m_done = true;
        }
        break;
    }
    default:
    {
        break;
    }
    }
}

/***********************************************************
 * SSLSocketSendJob
 ***********************************************************/

SSLSocketSendJob::SSLSocketSendJob(eadp::foundation::IHub* hub,
    eadp::foundation::SharedPtr<SSLSocket> socket,
    const eadp::foundation::SharedPtr<eadp::foundation::Internal::RawBuffer> data,
    SSLSocket::SSLSocketErrorCallback callback) 
: m_hub(hub)
, m_socket(socket)
, m_data(data)
, m_callback(callback)
{
}

SSLSocketSendJob::~SSLSocketSendJob()
{
}

void SSLSocketSendJob::execute()
{
    // Send data mark job as done
    m_done = true;

    m_socket->send(m_data, m_callback);
}


/***********************************************************
 * SSLSocketCloseJob
 ***********************************************************/

SSLSocketCloseJob::SSLSocketCloseJob(eadp::foundation::IHub* hub, eadp::foundation::SharedPtr<SSLSocket> socket) 
: m_hub(hub)
, m_socket(socket)
{
}

SSLSocketCloseJob::~SSLSocketCloseJob()
{
}

void SSLSocketCloseJob::execute()
{
    m_done = true;

    m_socket->close();
}
