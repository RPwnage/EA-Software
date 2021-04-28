/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef STRESSCONNECTION_H
#define STRESSCONNECTION_H

/*** Include files *******************************************************************************/

#include "EASTL/hash_map.h"
#include "framework/connection/channelhandler.h"
#include "framework/connection/socketchannel.h"
#include "framework/component/message.h"
#include "framework/component/rpcproxysender.h"
#include "framework/component/rpcproxyresolver.h"
#include "framework/system/fibersemaphore.h"

#include "EATDF/tdfobjectid.h"

#include "rpcperformance.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class RawBuffer;
class SocketChannel;
class Channel;
class InetAddress;
class TdfEncoder;
class TdfDecoder;
class Protocol;

namespace Stress
{

class RpcPerformance;
#define DEFAULT_RESUME_SESSION_DELAY_IN_MS 3000

class InstanceResolver;

class AsyncHandlerBase
{
public:
    virtual void onAsyncBase(uint16_t component, uint16_t type, RawBuffer* payload) = 0;

    virtual ~AsyncHandlerBase() { }
};

class AsyncHandler
{
public:
    virtual void onAsync(uint16_t component, uint16_t type, RawBuffer* payload) = 0;

    virtual ~AsyncHandler() { }
};

class StressConnection
    : public RpcProxySender,
      public RpcProxyResolver,
      private ChannelHandler
{
    NON_COPYABLE(StressConnection);

public:
    class ConnectionHandler
    {
    public:
        virtual void onDisconnected(StressConnection* conn, bool resumeSession) = 0;
        virtual void onSessionResumed(StressConnection* conn, bool success) = 0;
        virtual void onUserSessionMigrated(StressConnection* conn) = 0;
        virtual ~ConnectionHandler() { }
    };

    StressConnection(int32_t id, bool secure, InstanceResolver& resolver);
    ~StressConnection();

    bool initialize(const char8_t* protocolType,
            const char8_t* encoderType, const char8_t* decoderType, const uint32_t maxFrameSize);

    bool connect(bool quiet = false);

    void reconnect();

    void disconnect(bool resumeSession = false);
    void setConnectionHandler(ConnectionHandler* handler);

    bool connected() { return mConnected; }

    const InetAddress& getAddress() const { return mAddress; }
    const char8_t* getAddressString(char8_t* buf, size_t len) { return (mAddress.isResolved() ? mAddress.toString(buf, len) : "unresolved address"); }

    void addAsyncHandler(uint16_t component, uint16_t type, AsyncHandler* handler);
    void removeAsyncHandler(uint16_t component, uint16_t type, AsyncHandler* handler);

    void addBaseAsyncHandler(uint16_t component, uint16_t type, AsyncHandlerBase* handler);
    void removeBaseAsyncHandler(uint16_t component, uint16_t type, AsyncHandlerBase* handler);

    int32_t getIdent() { return mIdent; }

    void resetSessionKey() { *mSessionKey = '\0'; }

    // RpcProxySender interface
    BlazeRpcError DEFINE_ASYNC_RET(sendRequest(EA::TDF::ComponentId component, CommandId command, const EA::TDF::Tdf* request, 
        EA::TDF::Tdf *responseTdf, EA::TDF::Tdf *errorTdf, InstanceId& movedTo, int32_t logCategory = Blaze::Log::CONNECTION,
        const RpcCallOptions &options = RpcCallOptions(), RawBuffer* responseRaw = NULL) override);

    // RpcProxyResolver interface
    RpcProxySender* getProxySender(const InetAddress&) override { return this; }
    RpcProxySender* getProxySender(InstanceId) override { return this; }

    int64_t getLastRpcTimestamp() const { return mLastRpcTimestamp; }
    int64_t getLastReadTimestamp() const { return mLastReadTimestamp; }
    int64_t getLastConnectionAttemptTimestamp() const { return mLastConnectionAttemptTimestamp; }
    void setClientType(ClientType clientType) { mClientType = clientType; }
    void setResumeSession(bool resumeSession) { mResumeSession = resumeSession; }
    void setResumeSessionDelay(uint32_t delay) { mResumeSessionDelay = delay; }

private:
    typedef struct
    {
        Blaze::SemaphoreId semaphoreId;
        Blaze::BlazeRpcError *error;
        EA::TDF::Tdf *response;
        EA::TDF::Tdf *errorResponse;
		uint32_t	 *payloadSize;
		uint64_t	 *payloadEncodingTimeUs;
    } ProxyInfo;

    typedef eastl::list<AsyncHandler*> AsyncHandlerList;
    typedef eastl::hash_map<uint32_t,AsyncHandlerList> AsyncHandlerMap;
    typedef eastl::hash_map<uint32_t, AsyncHandlerBase*> AsyncHandlerBaseMap;
    typedef eastl::hash_map<uint32_t,ProxyInfo> ProxyInfoBySeqno;
    typedef eastl::deque<RawBufferPtr> OutputQueue;
    typedef eastl::hash_map<uint32_t, RawBufferPtr> SentRequestBySeqno;

    bool mSecure;
    InstanceResolver& mResolver;
    SocketChannel* mChannel;
    InetAddress mAddress;
    bool mConnected;
    AsyncHandlerMap mAsyncHandlers;
    AsyncHandlerBaseMap mBaseAsyncHandlers;
    ProxyInfoBySeqno mProxyInfo;
    uint32_t mSeqno;
    RawBuffer* mRecvBuf;
    RpcProtocol* mProtocol;
    TdfEncoder* mEncoder;
    TdfDecoder* mDecoder;
    int32_t mIdent;
    ConnectionHandler* mConnectionHandler;
    OutputQueue mSendQueue;
    SentRequestBySeqno mSentRequestBySeqno;
    int64_t mLastRpcTimestamp;
    int64_t mLastReadTimestamp;
    int64_t mLastConnectionAttemptTimestamp;
    ClientType mClientType;
    bool mResumeSession;
    uint32_t mResumeSessionDelay;
    bool mIsResumingSession;
    char8_t mSessionKey[MAX_SESSION_KEY_LENGTH+1];
    TimerId mResumeSessionTimer;

    // ConnectionHandler interface
    virtual void onRead(Channel& channel) override;
    virtual void onWrite(Channel& channel) override;
    virtual void onError(Channel& channel) override;
    virtual void onClose(Channel& channel) override;

    void onConnected(bool success, SocketChannel* channel);

    void processIncomingMessage();
    void dispatchOnAsyncFiber(uint16_t component, uint16_t command, RawBuffer* payload);
    bool writeFrame(RawBufferPtr frame);
    bool flushSendQueue();
    BlazeRpcError sendPingReply(uint16_t component, uint16_t command, uint32_t msgId);

    void resumeSession();
    bool mMigrating;
    bool mSendingPingToNewInstance;
    bool mResolvingInstanceByRdir;
    int32_t mRetryCount;
};

} // namespace Stress
} // namespace Blaze

#endif // STRESSCONNECTION_H

