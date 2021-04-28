/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_INBOUNDRPCCONNECTION_H
#define BLAZE_INBOUNDRPCCONNECTION_H

/*** Include files *******************************************************************************/

#include "EASTL/hash_set.h"
#include "EASTL/list.h"
#include "EASTL/vector.h"
#include "EASTL/intrusive_list.h"

#include "framework/system/timerqueue.h"
#include "framework/connection/connection.h"
#include "framework/component/message.h"
#include "framework/component/notification.h"
#include "framework/slivers/sliver.h"
#include "framework/connection/session.h"
#include "framework/component/peerinfo.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class ConnectionOwner;
class SocketChannel;
class RawBuffer;
class Endpoint;
class TdfEncoder;
class TdfDecoder;
class Command;
class Component;
struct CommandInfo;
class Message;
class OwnedSliver;
class SlaveSession;
struct RpcContext;
typedef eastl::intrusive_ptr<SlaveSession> SlaveSessionPtr;

namespace Redirector
{
    class RedirectorMasterImpl;
}

class InboundRpcConnection : public Connection
{
    NON_COPYABLE(InboundRpcConnection);

public:

    InboundRpcConnection(ConnectionOwner& owner, ConnectionId ident, 
        SocketChannel& channel, Blaze::Endpoint& endpoint, RatesPerIp *rateLimitCounter);
    ~InboundRpcConnection() override;

    static void initLocalRpcContext(UserSessionId userSessionId, bool superUserPrivilege);

    void close() override; // overrides Connection::close(), but calls Connection::close() internally.

    void sendNotification(Notification& notification);
    
    RatesPerIp* getRateLimitCounter() const { return mRateLimitCounter; }
    void setRateLimitCounter(RatesPerIp* rateLimitCounter);

    bool isServerConnection() const override = 0;
    bool isInboundConnection() const override { return true; }
    bool getIgnoreInactivityTimeout() const override { return mPeerInfo.getIgnoreInactivityTimeout(); }

    bool hasOutstandingCommands() const override { return !mOutstandingCommands.empty(); }

    //+ Connectivity checking related apis
    void setConnectivityCheckSettings(EA::TDF::TimeValue interval, EA::TDF::TimeValue timeout);
    
    void enableConnectivityChecking();
    void disableConnectivityChecking();
    bool isConnectivityCheckingEnabled() const { return (mConnectivityCheckTimer != INVALID_TIMER_ID); }
    
    bool isResponsive() const { return mIsResponsive; }
    BlazeRpcError DEFINE_ASYNC_RET(waitForConnectivityCheck(const EA::TDF::TimeValue& absoluteTimeout));
    //-

    uint64_t getExceededRpcFiberLimitDropped() const { return mExceededRpcFiberLimitDroppedCount; }
    uint64_t getExceededRpcFiberLimitPermitted() const { return mExceededRpcFiberLimitPermittedCount; }

protected:
    void processMessage(RawBufferPtr& buffer) override;

    virtual BlazeRpcError initRpcContext(RpcContext& rpcContext) = 0;

    void processRequest(Component& component, const CommandInfo& cmdInfo, Message& message);
    void processRawRequest(Component& component, const CommandInfo& cmdInfo, Message& message);
    void completeRequestInternal(SlaveSessionId slaveSessionId, UserSessionId userSessionId, const RpcProtocol::Frame &frame, BlazeRpcError err, InstanceId movedTo, const EA::TDF::Tdf *tdf = nullptr, RawBuffer *passthroughData = nullptr);

    // Called to indicate completion of a request and send the response
    virtual void completeRequest(SlaveSessionId slaveSessionId, UserSessionId userSessionId, const RpcProtocol::Frame &frame, BlazeRpcError err, InstanceId movedTo, const EA::TDF::Tdf *tdf = nullptr);    

    virtual SlaveSessionId getOrCreateSlaveSessionId(const RpcProtocol::Frame &inFrame) = 0;
    virtual UserSessionId getOrAllocateUserSessionId(const RpcProtocol::Frame &inFrame) = 0;

    virtual void fillResponseFrame(UserSessionId userSessionId, BlazeRpcError error, InstanceId movedTo, const RpcProtocol::Frame& originalFrame, RpcProtocol::Frame& responseFrame);
    virtual bool processUnhandledMessage(UserSessionId userSessionId, RpcProtocol::Frame& frame, RawBufferPtr& incomingMessageBuffer) { return false; }
    bool getQueueDirectly() const override;
    RpcProtocol &getProtocol() const { return (RpcProtocol &) getProtocolBase(); }
    
    bool sendPing(uint32_t userIndex, const char8_t* token, uint32_t* msgNum = nullptr);
    void signalConnectivityChecks(BlazeRpcError error, uint32_t msgNum);
    virtual void onResponsivenessChanged() {}

protected:
    friend struct RpcContext;
    friend class Component;
    friend class Redirector::RedirectorMasterImpl;
    template <class ComponentStubClass, const CommandInfo& CMD_INFO> friend class NotificationSubscribeCommand;
    template <class ComponentStubClass, const CommandInfo& CMD_INFO> friend class ReplicationSubscribeCommand;
    template <class ComponentStubClass, const CommandInfo& CMD_INFO> friend class ReplicationUnsubscribeCommand;

    //This is the current slave session running a remote instance command.  
    static FiberLocalWrappedPtr<SlaveSessionPtr, SlaveSession> smCurrentSlaveSession;

private:
    void scheduleNextConnectivityCheck();
    void doConnectivityCheck();
    void handleConnectivityCheckTimeout();

    void queueNotificationData(ComponentId componentId, NotificationId notificationId, const RawBufferPtr payloadBuffer, int32_t logCategory, const NotificationSendOpts& opts);
    bool isRpcAuthorized(const CommandInfo& cmdInfo, Component& component, RpcProtocol::Frame& inFrame, RawBufferPtr& incomingMessageBuffer, UserSessionId userSessionId);
    bool isOverloadingFibers(const CommandInfo& cmdInfo, Component& component, RpcProtocol::Frame& inFrame);
    bool isWrongEndpoint(const CommandInfo& cmdInfo, Component& component);
private:

    typedef eastl::hash_set<uint32_t> MessageSeqSet;
    MessageSeqSet mOutstandingCommands;

    
    

    RatesPerIp *mRateLimitCounter;

    struct EventHandleNode : eastl::intrusive_list_node
    {
        EventHandleNode(const Fiber::EventHandle& _eventHandle) : eventHandle(_eventHandle) {}

        Fiber::EventHandle eventHandle;
        operator const Fiber::EventHandle& () const { return eventHandle; }
    };

    typedef eastl::intrusive_list<EventHandleNode> EventHandleList;
    EventHandleList mConnectivityCheckWaiters;
    EA::TDF::TimeValue mLastFiberLimitWarnLog;
    // mConnectivityCheckTimer is used for the periodic calls to sendPing(), *and* for the response timeout.
    TimerId mConnectivityCheckTimer;
    EA::TDF::TimeValue mConnectivityCheckInterval;
    EA::TDF::TimeValue mConnectivityCheckTimeout;
    EA::TDF::TimeValue mNextConnectivityCheckTime;
    uint32_t mLastPingMsgNumForPeriodicChecks;
    uint32_t mLastPingMsgNumForManualChecks;
    uint64_t mExceededRpcFiberLimitDroppedCount;
    uint64_t mExceededRpcFiberLimitPermittedCount;
    bool mIsResponsive;
    bool mAwaitingFirstPingReply;

protected:
    LegacyPeerInfo mPeerInfo;
};

class ClientInboundRpcConnection : public InboundRpcConnection, private PeerInfoListener
{
public:
    ClientInboundRpcConnection(ConnectionOwner& owner, ConnectionId ident, SocketChannel& channel, Blaze::Endpoint& endpoint, RatesPerIp *rateLimitCounter);
    ~ClientInboundRpcConnection() override;

    bool isServerConnection() const override { return false; }

    void close() override;

    void scheduleGracefulClose();

protected:
    BlazeRpcError initRpcContext(RpcContext& rpcContext) override;

    SlaveSessionId getOrCreateSlaveSessionId(const RpcProtocol::Frame &inFrame) override;
    UserSessionId getOrAllocateUserSessionId(const RpcProtocol::Frame &inFrame) override;

    bool isKeepAliveRequired() const override;
    void fillResponseFrame(UserSessionId userSessionId, BlazeRpcError error, InstanceId movedTo, const RpcProtocol::Frame& originalFrame, RpcProtocol::Frame& responseFrame) override;

    void onResponsivenessChanged() override;
    void onResponsivenessChangedInternal();

    void initiateGracefulClose();

private:
    void onAllUsersRemoved() override;
    friend class UserSessionsMasterImpl;

    bool mPerformGracefulClose;
    TimerId mCloseTimer;

};

class ServerInboundRpcConnection : public InboundRpcConnection
{
public:
    ServerInboundRpcConnection(ConnectionOwner& owner, ConnectionId ident, SocketChannel& channel, Blaze::Endpoint& endpoint, RatesPerIp* rateLimitCounter);
    
    bool isServerConnection() const override { return true; }
    void close() override;

protected:
    BlazeRpcError initRpcContext(RpcContext& rpcContext) override;

    SlaveSessionId getOrCreateSlaveSessionId(const RpcProtocol::Frame &inFrame) override;
    UserSessionId getOrAllocateUserSessionId(const RpcProtocol::Frame &inFrame) override;

    bool isKeepAliveRequired() const override { return true; }

private:
    SlaveSession *mSlaveSession;

    SlaveSessionId makeSlaveSessionIdFromContext(uint64_t context) const;
};

} // Blaze

#endif // BLAZE_INBOUNDRPCCONNECTION_H

