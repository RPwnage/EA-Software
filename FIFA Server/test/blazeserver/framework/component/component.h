/*************************************************************************************************/
/*!
\file


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_COMPONENT_H
#define BLAZE_COMPONENT_H

/*** Include files *******************************************************************************/
#include "framework/blazedefines.h"
#include "blazerpcerrors.h"
#include "framework/system/fiber.h"
#include "framework/system/timerqueue.h"
#include "framework/component/rpcproxysender.h"
#include "framework/controller/transaction.h"
#include "framework/grpc/inboundgrpcjobhandlers.h"

#include "EASTL/hash_map.h"
#include "EASTL/vector_set.h"
#include "EASTL/unique_ptr.h"

#include "EATDF/tdfobjectid.h"



/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class Message;
class ComponentStub;
class UserSession;
class ComponentStatus;
class TdfDecoder;
class ComponentManager;
class BlazeRpcComponentDb;
class RawBuffer;



typedef EA::TDF::Tdf* (*TdfFactoryFunc)();



/*! ***********************************************************************/
/*! \class Component
    \brief The base object for all server components.

    This class provides a base interface and functions for activating a component
    and processing messages to/from the component.  Implementation classes add
    RPC and Notification interfaces, as well as message routing and service-specific
    implementation.
***************************************************************************/
class Component
{
    NON_COPYABLE(Component);

public:

    typedef eastl::vector<InstanceId> InstanceIdList;
    typedef eastl::vector_set<InstanceId> InstanceIdSet;
    typedef eastl::hash_map<InstanceId, Fiber::WaitList> WaitListByInstanceIdMap;
    typedef Functor1<InstanceId> SubscribedForNotificationsFromInstanceCb;

    virtual ~Component();

    virtual ComponentStub* asStub() { return nullptr; }
    virtual const ComponentStub* asStub() const { return nullptr; }

    //Reserved notification ids - the high 16 values of the 16-bit notification id are reserved for the 
    //following built in notifications.

    /*! The reserved notification id for a replication map creation. */
    const static NotificationId REPLICATION_MAP_CREATE_NOTIFICATION_ID = 0xFFF0;

    /*! The reserved notification id for a replication map destroy. */
    const static NotificationId REPLICATION_MAP_DESTROY_NOTIFICATION_ID = 0xFFF1;

    /*! The reserved notification id for a replication object update. */
    const static NotificationId REPLICATION_OBJECT_UPDATE_NOTIFICATION_ID = 0xFFF2;

    /*! The reserved notification id for a replication object erasure. */
    const static NotificationId REPLICATION_OBJECT_ERASE_NOTIFICATION_ID = 0xFFF3;

    /*! The reserved synchronization for a component is complete. */
    const static NotificationId REPLICATION_SYNC_COMPLETE_NOTIFICATION_ID = 0xFFF4;

    /*! The reserved synchronization for a component is complete. */
    const static NotificationId REPLICATION_MAP_SYNC_COMPLETE_NOTIFICATION_ID = 0xFFF5;

    /*! The reserved notification id for a replication object insert. */
    const static NotificationId REPLICATION_OBJECT_INSERT_NOTIFICATION_ID = 0xFFF6;

    /*! The reserved notification id for a replication object subscription. */
    const static NotificationId REPLICATION_OBJECT_SUBSCRIBE_NOTIFICATION_ID = 0xFFF7;

    //Reserved command ids - the high 16 values of the 16-bit command id are reserved for the 
    //following built in commands.

    /*! The reserved command id for the notification subscription RPC */
    const static CommandId NOTIFICATION_SUBSCRIBE_CMD = 0xFFF0;

    /*! The reserved command id for the replication subscription RPC */
    const static CommandId REPLICATION_SUBSCRIBE_CMD = 0xFFF1;

    /*! The reserved command id for the replication subscription RPC */
    const static CommandId REPLICATION_UNSUBSCRIBE_CMD = 0xFFF2;

    /*! The reserved command id for the grpc subscription RPC */
    const static CommandId NOTIFICATION_SUBSCRIBE_GRPC_CMD = 0xFFF3;

    /*! An invalid command id */
    const static CommandId INVALID_COMMAND_ID;
    const static NotificationId INVALID_NOTIFICATION_ID = 0xFFFF;

    /*! An invalid component id */
    static const EA::TDF::ComponentId INVALID_COMPONENT_ID;

    BlazeRpcError DEFINE_ASYNC_RET(sendRequest(const CommandInfo& cmdInfo, const EA::TDF::Tdf* request, EA::TDF::Tdf *responseTdf, EA::TDF::Tdf *errorTdf, const RpcCallOptions& options, Message* origMsg = nullptr, InstanceId* movedTo = nullptr, RawBuffer* responseRaw = nullptr, bool obfuscateResponse = false));
    BlazeRpcError DEFINE_ASYNC_RET(sendRawRequest(const CommandInfo& cmdInfo, RawBufferPtr& requestBuf, RawBufferPtr& responseBuf, const RpcCallOptions& options, InstanceId* movedTo = nullptr));

    static InstanceId getControllerInstanceId();

    class MultiRequestResponse
    {
    public:
        MultiRequestResponse(InstanceId _instanceId, const EA::TDF::TdfPtr& _response, const EA::TDF::TdfPtr& _errorResponse) 
            : instanceId(_instanceId), outboundMessageNum(0), error(ERR_OK), response(_response), errorResponse(_errorResponse) {}

        InstanceId instanceId;
        MsgNum outboundMessageNum;
        BlazeRpcError error;
        EA::TDF::TdfPtr response;
        EA::TDF::TdfPtr errorResponse;
    };

    typedef eastl::vector<eastl::unique_ptr<MultiRequestResponse>> MultiRequestResponseList;

    template <typename ResponseType, typename ErrorResponseType>
    class MultiRequestResponseHelper : public MultiRequestResponseList
    {
    public:
        template<typename InstanceIdListType>
        MultiRequestResponseHelper(const InstanceIdListType& instanceIds, bool includeLocalInstance = true, MemoryGroupId memGroupId = DEFAULT_BLAZE_MEMGROUP, const char8_t* memAllocName = "")
        {
            reserve(instanceIds.size());
            for (typename InstanceIdListType::const_iterator itr = instanceIds.begin(), endItr = instanceIds.end(); itr != endItr; ++itr)
            {
                if (includeLocalInstance || (Component::getControllerInstanceId() != *itr))
                {
                    push_back(eastl::unique_ptr<MultiRequestResponse>(BLAZE_NEW MultiRequestResponse(*itr, EA::TDF::TypeDescriptionSelector<ResponseType>::get().createInstance(*Allocator::getAllocator(memGroupId), memAllocName), 
                       EA::TDF::TypeDescriptionSelector<ErrorResponseType>::get().createInstance(*Allocator::getAllocator(memGroupId), memAllocName))));
                }
            }
        }
    };

    /*! \brief Send RPC request to multiple instances. Returns ERR_OK if and only if:
                   1) There was at least one valid instance in the response list (i.e. the response list included at least one valid instance id), AND
                   2) Requests to all valid instances in the response list succeeded
                If multiple requests fail, the first error will be the return value. However, the error for each individual request will be available in the 'error' field of the corresponding response.
    */
    BlazeRpcError DEFINE_ASYNC_RET(sendMultiRequest(const CommandInfo& cmdInfo, const EA::TDF::Tdf* request, MultiRequestResponseList& responses, const RpcCallOptions& options = RpcCallOptions()));

    /*! \brief Process an incoming notification from a parent component. */
    //void processNotification(const RpcProtocol::Frame &inFrame, RawBufferPtr& buffer, TdfDecoder &decoder);
    void processNotification(Notification& notification);

    const ComponentData& getComponentInfo() const { return mComponentInfo; }

    const CommandInfo* getCommandInfo(CommandId id) const { return mComponentInfo.getCommandInfo(id); } 

    /*! \brief Gets the unique id of this component..  */
    EA::TDF::ComponentId getId() const { return mComponentInfo.id; }

    size_t getIndex() const { return mComponentInfo.index; }

    size_t getTypeIndex() const { return mComponentInfo.baseInfo.index; }

    size_t getLogCategory() const { return mComponentInfo.baseInfo.index; }
    
    MemoryGroupId getMemGroupId() const { return mComponentInfo.baseInfo.memgroup; }

    Blaze::Allocator& getAllocator() const { return mAllocator; }

    /*! \brief Gets the name of the component. */
    const char8_t* getName() const { return mComponentInfo.name; }

    /*! \brief Gets the base name of the component. For slaves this is the component name, for masters this is the component name minus "_master" */
    const char8_t* getBaseName() const { return mComponentInfo.baseInfo.name; }
    
    /*! \brief Returns true if this component is designated a "Master" component */
    bool isMaster() const { return RpcIsMasterComponent(mComponentInfo.id); }

    /*! \brief Returns true if the given component id is designated a "Master" component.*/
    static bool isMasterComponentId(EA::TDF::ComponentId componentId) { return RpcIsMasterComponent(componentId); }

    /*! \brief Returns true if this component has a master it needs to resolve. */
    bool hasMasterComponent() const { return mComponentInfo.hasMaster; }

    /*! \brief Returns true if this component has a master it needs to resolve before activating. */
    bool requiresMasterComponent() const { return mComponentInfo.requiresMasterComponent; }

    /*! \brief Returns true if the given component can have multiple instances */
    bool hasMultipleInstances() const { return mComponentInfo.hasMultipleInstances; }
    
    /*! \brief Returns whether the component is locally hosted.*/
    virtual bool isLocal() const { return false; }

    /*! \brief Returns whether or not this component exposes any replicated maps. */
    bool hasComponentReplication() const { return mComponentInfo.hasReplication; }
    
    /*! \brief Returns whether or not this component exposes any notifications. */
    bool hasComponentNotifications() const { return mComponentInfo.hasNotifications; }

    /*! \brief Resets all of the global rate table index to (-1). */
    void resetAllCommandRateTableIndexes();

    /*! \brief Sets the global rate table index for the specified command. */
    void setCommandRateTableIndex(CommandId commandId, int32_t index);

    /*! \brief Starts transaction associated with this component */
    virtual BlazeRpcError DEFINE_ASYNC_RET(startTransaction(TransactionContextHandlePtr &transactionPtr, uint64_t customData = 0, EA::TDF::TimeValue timeout = 0));

    /*! \brief Commits transaction associated with this component */
    virtual BlazeRpcError DEFINE_ASYNC_RET(commitTransaction(TransactionContextId id));

    /*! \brief Rollback transaction associated with this component */
    virtual BlazeRpcError DEFINE_ASYNC_RET(rollbackTransaction(TransactionContextId id)); 
    
    /*! \brief Resolves transaction id into transaction pointer */
    TransactionContextPtr getTransactionContext(TransactionContextId id); 

    /*! \brief Resolves transaction id into transaction handle pointer */
    TransactionContextHandlePtr getTransactionContextHandle(TransactionContextId id);
    
    /*! \brief Subscribe the replication and notification if the instance has been added or restarted */
    BlazeRpcError sendReplicaAndNotifForInstance(InstanceId instanceId);

    virtual void setInstanceDraining(InstanceId id, bool draining);

    BlazeRpcError addInstance(InstanceId id);
    bool removeInstance(InstanceId id);

    int64_t getInstanceRestartTimeoutUsec() const;
    bool hasInstanceId(InstanceId instanceId) const { return mInstanceIds.find(instanceId) != mInstanceIds.end(); }
    BlazeRpcError waitForInstanceId(InstanceId instanceId, const char8_t* context, EA::TDF::TimeValue absoluteTimeout = EA::TDF::TimeValue());
    bool isInstanceDraining(InstanceId instanceId) const { return mNonDrainingInstanceIds.find(instanceId) == mInstanceIds.end(); }
    bool isValidKey(uint64_t key) const { return hasInstanceId(GetInstanceIdFromInstanceKey64(key)); }
    InstanceId validateKey(uint64_t key) const { return hasInstanceId(GetInstanceIdFromInstanceKey64(key)) ? GetInstanceIdFromInstanceKey64(key) : INVALID_INSTANCE_ID; }
    InstanceId getLowestInstanceId(bool selectFromDrainingInstances = false) const { return !mInstanceIds.empty() ? (selectFromDrainingInstances ? *mInstanceIds.begin() : *mNonDrainingInstanceIds.begin()) : INVALID_INSTANCE_ID;}
    InstanceId getAnyInstanceId() const;
    InstanceId selectInstanceId() const;
    SliverId getSliverIdByLoad() const;
    const InetAddress& getExternalInetAddress() const { return mExternalAddress; }

    void getComponentInstances(InstanceIdList& instances, bool randomized = true, bool includeDrainingInstances = true) const;
    size_t getComponentInstanceCount(bool includeDrainingInstances = true) const { return includeDrainingInstances ? mInstanceIds.size() : mNonDrainingInstanceIds.size(); }

    virtual BlazeRpcError replicationSubscribe(InstanceId selectedInstanceId = INVALID_INSTANCE_ID);
    virtual BlazeRpcError replicationUnsubscribe(InstanceId selectedInstanceId = INVALID_INSTANCE_ID);
    const InstanceIdSet& getReplicatedInstances() const { return mHasSentReplicationSubscribe; }
    
    BlazeRpcError notificationSubscribe();
    bool hasSentNotificationSubscribeToInstance(InstanceId instanceId);
    void registerNotificationsSubscribedCallback(SubscribedForNotificationsFromInstanceCb cb);
    void deregisterNotificationsSubscribedCallback(SubscribedForNotificationsFromInstanceCb cb);

    virtual InstanceId getLocalInstanceId() const;

    template <class CompClass>
    static Component* createFunc(RpcProxyResolver& resolver)
    {
        return BLAZE_NEW_MGID(CompClass::COMPONENT_MEMORY_GROUP, "Remote Component") CompClass(resolver);
    }
    template <class CompClass>
    static Component* createFunc(RpcProxyResolver& resolver, const InetAddress& addy)
    {
        return BLAZE_NEW_MGID(CompClass::COMPONENT_MEMORY_GROUP, "Remote Component") CompClass(resolver, addy);
    }

    template <class ComponentClass, class ListenerClass, class NotificationType, void (ListenerClass::*FuncAddr)(const NotificationType&,UserSession*)>
    void dispatchNotification(const EA::TDF::Tdf* payload, UserSession* associatedSession) const
    {
        //Worth calling out the use of the template keyword below to disambiguate the member template below.
        //See http://stackoverflow.com/questions/610245/where-and-why-do-i-have-to-put-the-template-and-typename-keywords for a good overview
        ((const ComponentClass*) this)->getDispatcher().template dispatch<const NotificationType&, UserSession*>(FuncAddr, (const NotificationType&) *payload, associatedSession); 
    }

    template <class ComponentClass, class ListenerClass, void (ListenerClass::*FuncAddr)(UserSession*)>
    void dispatchNotificationNoPayload(const EA::TDF::Tdf*, UserSession* associatedSession) const
    {
        ((const ComponentClass*) this)->getDispatcher().template dispatch<UserSession*>(FuncAddr, associatedSession); 
    }

    template<typename NotificationMsg>
    static void sendNotificationGrpc(Blaze::Grpc::InboundRpcBase& rpc, Blaze::Notification& notification)
    {
        NotificationMsg notificationMsg;
        
        NotificationId notificationId = notification.getNotificationId();
        if (notificationId == 0)
            notificationId = 20002; // See TdfGrammar.g for the explanation
        
        using NotificationType = typename NotificationMsg::NotificationId;
        notificationMsg.setNotificationId(static_cast<NotificationType>(notificationId));

        if (notification.getPayload() != nullptr)
        {
            notificationMsg.getNotificationMsg().set(notification.getPayload());
        }
        else
        {
            EA::TDF::TdfPtr tdfptr;
            if (notification.extractPayloadTdf(tdfptr, false) == ERR_OK)
            {
                notificationMsg.getNotificationMsg().set(notification.getPayload());
            }
        }
        
        rpc.sendResponse(&notificationMsg);
    }

    //void dropIncomingNotificationsForInstance(InstanceId instanceId);

    virtual bool isShardedBySlivers() const { return false; }

protected:
    friend class ComponentStub;

    Component(const ComponentData& componentData, RpcProxyResolver& resolver, const InetAddress& externalAddress);

     // transaction context implementation
    BlazeRpcError DEFINE_ASYNC_RET(forwardStartTransaction(uint64_t customData, EA::TDF::TimeValue timeout, TransactionContextHandlePtr &transactionPtr));
    BlazeRpcError DEFINE_ASYNC_RET(forwardCompleteTransaction(TransactionContextId id, bool commit));
    size_t getPendingTransactionsCount() const { return mTransactionIdToContextMap.size(); }


    virtual bool hasNotificationListener() const { return false; }
protected:
 
    // transaction mechanism
    virtual BlazeRpcError DEFINE_ASYNC_RET(createTransactionContext(uint64_t customData, TransactionContext*& transaction)) 
        { transaction = nullptr; return Blaze::ERR_SYSTEM; } // not implemented by default    
    
    virtual bool getShardKeyFromRequest(CommandId commandId, const EA::TDF::Tdf& request, ObjectId& shardKey) const { return false; } //Overridden by components

protected:
    InstanceIdSet mInstanceIds;
    InstanceIdSet mNonDrainingInstanceIds;
    InstanceIdSet mHasSentNotificationSubscribe;
    InstanceIdSet mHasSentReplicationSubscribe;

    RpcProxyResolver& mResolver;
    InetAddress mExternalAddress;

private:
    typedef eastl::vector<Fiber::EventHandle> EventList;
    typedef eastl::vector<SubscribedForNotificationsFromInstanceCb> SubscribedForNotficationsCbList;

    struct RestartEntry
    {
        RestartEntry() : restartingInstanceTimerId(INVALID_TIMER_ID) {}
        TimerId restartingInstanceTimerId;
        EventList waitingRpcList;
    };
    typedef eastl::hash_map<InstanceId, RestartEntry> RestartInstanceToEntryMap;

    void eraseInstanceData(InstanceId id);
    void onRemoteInstanceRestartTimeout(InstanceId id, EA::TDF::TimeValue restartingTimeout);

    BlazeRpcError sendReplicationSubscribeForInstance(InstanceId id); 
    BlazeRpcError sendReplicationUnsubscribeForInstance(InstanceId id);
    BlazeRpcError sendNotificationSubscribeForInstance(InstanceId id);

    const ComponentData& mComponentInfo;
    Blaze::Allocator& mAllocator;
    RestartInstanceToEntryMap mRestartingInstanceMap;

    bool mAutoRegInstancesForNotifications;
    bool mAutoRegInstancesForReplication;

    // Transaction mechanism
    typedef eastl::map<TransactionContextId, TransactionContextPtr> TransactionIdToContextMap;    
    TransactionIdToContextMap mTransactionIdToContextMap; 

    friend class Controller;
    friend class ComponentManager;

    // transaction methods
    BlazeRpcError DEFINE_ASYNC_RET(completeLocalTransaction(TransactionContextId id, bool commit));
    BlazeRpcError DEFINE_ASYNC_RET(expireTransactionContexts(EA::TDF::TimeValue defaultTimeout));

    BlazeRpcError waitForInstanceRestart(InstanceId instanceId);

    // check if the instance is valid.  If the instance is not int the set, it means the instance has shutdown and passed the restarting time period.
    bool checkInstanceIsValid(InstanceId instanceId);

    BlazeRpcError executeRemoteCommand(const RpcCallOptions &options, RpcProxySender* sender, const CommandInfo &cmdInfo, const EA::TDF::Tdf* request, EA::TDF::Tdf* responseTdf,
        RawBuffer* responseRaw, EA::TDF::Tdf* errorTdf, InstanceId& movedTo);

    uint64_t mLastContextId;

    WaitListByInstanceIdMap mWaitListByInstanceIdMap;
    SubscribedForNotficationsCbList mSubscribedForNotificationsCbList;
};

} // Blaze

#endif // BLAZE_COMPONENT_H

