// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Job.h>
#include <eadp/foundation/Hub.h>
#include <DirtySDK/proto/protossl.h>
#include <eadp/realtimemessaging/RealTimeMessagingError.h>
#include <eadp/foundation/internal/JobBase.h>
#include <eadp/foundation/internal/Serialization.h>
#include <eadp/foundation/internal/HttpRequest.h>
#include <eadp/foundation/DirectorService.h>

namespace eadp
{
namespace realtimemessaging
{
class SSLSocket
{
public:
    static const uint32_t kBufSize = 8 * 1024;

    using SSLSocketDataCallback = eadp::foundation::CallbackT<void(eadp::foundation::UniquePtr<eadp::foundation::Internal::RawBuffer> data)>;
    using SSLSocketConnectCallback = eadp::foundation::CallbackT<void(const eadp::foundation::ErrorPtr& error)>;
    using SSLSocketDisconnectCallback = eadp::foundation::CallbackT<void(const eadp::foundation::ErrorPtr& error)>;
    using SSLSocketErrorCallback = eadp::foundation::CallbackT<void(const eadp::foundation::ErrorPtr&)>;

    explicit SSLSocket(eadp::foundation::IHub* hub);
    ~SSLSocket();


    enum class State
    {
        DISCONNECTED = -1,
        CONNECTING = 0,
        CONNECTED = 1,
    };

    // The following functions are for internal use only. Please use appropriate jobs to call these functionalities.
    void connect(eadp::foundation::String url,
        const SSLSocketConnectCallback& connectCb,
        const SSLSocketDisconnectCallback& disconnectCb,
        const SSLSocketDataCallback& dataCb);
    void close();
    void send(const eadp::foundation::SharedPtr<eadp::foundation::Internal::RawBuffer> data, SSLSocketErrorCallback callback);
    void update();
    State getState() const;

private:
    void processState();
    void onClose(const eadp::foundation::ErrorPtr& error);
    void closeConnection();
    void notifyDisconnect(const eadp::foundation::ErrorPtr& error);

    eadp::foundation::IHub* m_hub;
    ProtoSSLRefT* m_protosocket;
    SSLSocketDataCallback m_dataCallback;
    SSLSocketConnectCallback m_connectCallback;
    SSLSocketDisconnectCallback m_disconnectCallback;
    State m_state;
    eadp::foundation::Internal::RawBuffer m_buffer;
    eadp::foundation::Queue<eadp::foundation::SharedPtr<eadp::foundation::Internal::RawBuffer>> m_queuedData;
    uint32_t m_lastUnsentByte;
};

class SSLSocketConnectJob : public eadp::foundation::Internal::JobBase
{
public:
    using Callback = eadp::foundation::CallbackT<void(const eadp::foundation::ErrorPtr&)>;

    /*
     * Callback will be trigger with error if Connection request failed.
     */
    SSLSocketConnectJob(eadp::foundation::IHub* hub,
        eadp::foundation::SharedPtr<SSLSocket> socket,
        eadp::foundation::SharedPtr<eadp::foundation::Internal::Url> url,
        SSLSocket::SSLSocketConnectCallback connectCb,
        SSLSocket::SSLSocketDisconnectCallback disconnectCb,
        SSLSocket::SSLSocketDataCallback dataCb);

    ~SSLSocketConnectJob();

    void execute() override;

private:
    enum class State
    {
        INIT,
        CONNECT_CALLED
    };
    eadp::foundation::IHub* m_hub;
    eadp::foundation::SharedPtr<SSLSocket> m_socket;
    eadp::foundation::SharedPtr<eadp::foundation::Internal::Url> m_url;
    State m_state;
    SSLSocket::SSLSocketDataCallback m_dataCallback;
    SSLSocket::SSLSocketConnectCallback m_connectCallback;
    SSLSocket::SSLSocketDisconnectCallback m_disconnectCallback;
    eadp::foundation::IDirectorService* m_directorService;
};

class SSLSocketSendJob : public eadp::foundation::Internal::JobBase
{
public:
    SSLSocketSendJob(eadp::foundation::IHub* hub,
        eadp::foundation::SharedPtr<SSLSocket> socket,
        const eadp::foundation::SharedPtr<eadp::foundation::Internal::RawBuffer> data,
        SSLSocket::SSLSocketErrorCallback callback);

    ~SSLSocketSendJob();

    void execute() override;
private:
    eadp::foundation::IHub* m_hub;
    eadp::foundation::SharedPtr<SSLSocket> m_socket;
    eadp::foundation::SharedPtr<eadp::foundation::Internal::RawBuffer> m_data;
    SSLSocket::SSLSocketErrorCallback m_callback;
};

class SSLSocketCloseJob : public eadp::foundation::Internal::JobBase
{
public:
    using Callback = eadp::foundation::CallbackT<void(const eadp::foundation::ErrorPtr&)>;

    SSLSocketCloseJob(eadp::foundation::IHub* hub, eadp::foundation::SharedPtr<SSLSocket> socket);
    ~SSLSocketCloseJob();

    void execute() override;
private:
    eadp::foundation::IHub* m_hub;
    eadp::foundation::SharedPtr<SSLSocket> m_socket;
};
}
}

