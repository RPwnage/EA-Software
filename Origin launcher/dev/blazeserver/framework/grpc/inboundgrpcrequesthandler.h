/*************************************************************************************************/
/*!
    \file   inboundgrpcrequesthandler.h

    \attention
        (c) Electronic Arts Inc. 2017
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class InboundGrpcRequestHandler

*/
/*************************************************************************************************/
#ifndef BLAZE_INBOUND_GRPC_REQUESTHANDLER_H
#define BLAZE_INBOUND_GRPC_REQUESTHANDLER_H

#include "framework/blazedefines.h"
#include "framework/protocol/rpcprotocol.h"
#include "framework/component/peerinfo.h"
#include "framework/grpc/grpcutils.h"
#include "framework/system/fiberjobqueue.h"

#include "EASTL/intrusive_ptr.h"

namespace google
{
namespace protobuf
{
class Message;
}
}

namespace Blaze
{
struct RatesPerIp;

namespace Grpc
{

class GrpcEndpoint;
class InboundRpcBase;
using InboundRpcBasePtr = eastl::intrusive_ptr<InboundRpcBase>;

// This class is equivalent of InboundRpcConnection for grpc.
class InboundGrpcRequestHandler : private Blaze::PeerInfoListener   
{
    friend class GrpcEndpoint;
    friend class InboundRpcBase;
public:
    ~InboundGrpcRequestHandler();

    // The choice of the processMessage/processRequest names is a bit odd but they are to keep harmony with InboundRpcConnection.

    // Process the incoming message (do any error checking etc.)
    void processMessage(InboundRpcBase& rpc, const google::protobuf::Message* protobufMessage);
    void processLegacyMessage(InboundRpcBasePtr rpcPtr, const google::protobuf::Message* protobufMessage);

    // Process the incoming request/rpc on the fiber. 
    void processRequest(InboundRpcBasePtr rpcPtr, const google::protobuf::Message* protobufMessage);
    
    void completeRequest(InboundRpcBase& rpc, BlazeRpcError err, InstanceId movedTo, const EA::TDF::Tdf* tdf = nullptr);
    void completeNotificationSubscription(InboundRpcBase& rpc, BlazeRpcError err, InstanceId movedTo);

    bool isShutdown() const { return mShutdown; }
    void shutdown();

    void addNotificationHandler(InboundRpcBase& handler);
    void removeNotificationHandlers();
    void sendNotification(Notification& notification, bool overrideImmediately);
    // Unlike legacy Blaze framework, a user has to explicitly subscribe to notifications in Grpc by calling an rpc.
    // This creates a small time window between the time user session was created and it subscribed to it's notifications. 
    // The user session calls below to check if a notification handler is available. If not, it caches the notification. 
    bool hasNotificationHandler(EA::TDF::ComponentId componentId) const;

    uint32_t getNextMsgNum() { return ++mNextMsgNum; }
    PeerInfo& getPeerInfo() { return mPeerInfo; }
    bool hasOutstandingRpcs() { return !mInboundRpcBasePtrByIdMap.empty(); }
    Fiber::FiberGroupId getProcessMessageFiberGroupId() const { return mProcessMessageFiberGroupId; }

    // A request handler has a temp session key until it gets attached to a user session at which point
    // it has the key that is same as user session key.
    const char8_t* getSessionKey() { return mSessionKey.c_str(); }
    void setSessionKey(const char8_t* sessionKey) { mSessionKey.assign(sessionKey); }

    void detachFromUserSession() { mIsAttachedToUserSession = false; }
    bool isAttachedToUserSession() { return mIsAttachedToUserSession; }

    RatesPerIp* getRateLimitCounter() const { return mRateLimitCounter; }
    void setRateLimitCounter(RatesPerIp* rateLimitCounter) { mRateLimitCounter = rateLimitCounter; }

private:
    InboundGrpcRequestHandler(GrpcEndpoint& endpoint, FixedString64& tempSessionKey);

    void handleSystemError(InboundRpcBase& rpc, BlazeRpcError err, InstanceId movedTo);

    // RpcListener Interface
    void onInboundRpcDone(InboundRpcBase& rpc, bool rpcCancelled);

    // PeerInfoListener Interface
    void onAllUsersRemoved() override;

    void attachAndInitializeRpc(InboundRpcBase& rpc);
    void detachRpc(InboundRpcBase& rpc);

    GrpcEndpoint& mOwner;
    GrpcPeerInfo mPeerInfo;
    uint32_t mNextMsgNum; 
    // The fiber id to use for each rpc belonging to this request handler. This is useful when we have to wait for all the fibers to finish when exiting.
    Fiber::FiberGroupId mProcessMessageFiberGroupId; 
    bool mIsAttachedToUserSession;

    // Each component has a single server streaming rpc which sends out the notification
    eastl::hash_map<EA::TDF::ComponentId, Blaze::Grpc::InboundRpcBase*> mNotificationHandlersMap;

    void sendPendingNotifications(EA::TDF::ComponentId componentId);
    void sendQueuedNotifications();

    using InboundRpcBasePtrByIdMap = eastl::map<int64_t, InboundRpcBasePtr>;
    InboundRpcBasePtrByIdMap mInboundRpcBasePtrByIdMap;

    FixedString64 mSessionKey;

    bool mShutdown;
    Blaze::RatesPerIp* mRateLimitCounter;

    FiberJobQueue mLegacyRpcQueue;
    typedef eastl::list<NotificationPtr> AsyncNotificationsList;
    AsyncNotificationsList mQueuedNotificationList;
    size_t mOutstandingUnaryRpcs;
};

} //namespace Grpc
} //namespace Blaze

#endif // BLAZE_INBOUND_GRPC_REQUESTHANDLER_H
