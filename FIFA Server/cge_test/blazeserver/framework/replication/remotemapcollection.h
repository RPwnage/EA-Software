/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REMOTEMAPCOLLECTION_H
#define BLAZE_REMOTEMAPCOLLECTION_H

/*** Include files *******************************************************************************/
#include "framework/replication/replication.h"
#include "framework/replication/replicateditempool.h"
#include "framework/system/fibermanager.h"
#include "blazerpcerrors.h"
#include "framework/connection/session.h"
#include "EASTL/map.h"
#include "EASTL/hash_map.h"
#include "EASTL/vector.h"
#include "EASTL/hash_set.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class Component;
class InetAddress;
class Replicator;
class RemoteReplicatedMapBase;
class ReplicatedMapFactoryBase;
class ReplicationObjectSubscribe;
class SlaveSession;
class DynamicMapSubscriber;
class ReplicatedDataMediator;
class Fiber;

class RemoteMapCollection
{
    NON_COPYABLE(RemoteMapCollection);
public:
    RemoteMapCollection(Replicator& replicator, ComponentId componentId, MemoryGroupId memoryId, ReplicatedDataMediator& mediator);
    virtual ~RemoteMapCollection();

    bool isSynced() const;
        
    BlazeRpcError DEFINE_ASYNC_RET(startSynchronization(Component& componentToSync, InstanceId selectedIndex));
    BlazeRpcError DEFINE_ASYNC_RET(waitForSyncCompletion());
    void handleSynchronizationComplete(BlazeRpcError err, InstanceId instanceId);

    void shutdown();
    void destroy();

    void addMapFactorySubscriber(const CollectionIdRange &range, void* subscriber);
    void removeMapFactorySubscriber(const CollectionIdRange &range, void* subscriber);
    
    void createRemoteDynamicMap(const CollectionId& collectionId, const EA::TDF::Tdf* context);
    void destroyRemoteDynamicMap(const CollectionId& collectionId, const EA::TDF::Tdf* context);

    void addMap(RemoteReplicatedMapBase& map);  
    void addFactory(ReplicatedMapFactoryBase& factory);

    RemoteReplicatedMapBase* getMap(const CollectionId& collectionId);
    void getMaps(const CollectionIdRange &range, RemoteReplicatedMapList &list);

    ReplicatedMapFactoryBase* getFactory(const CollectionId& collectionId);
    ReplicatedMapFactoryBase* getFactory(const CollectionIdRange& range);

    ReplicatedDataMediator& getMediator() { return mMediator; }

    int32_t getLogCategory() const { return mLogCategory; }
    ReplicatedItemPoolCollection& getReplicatedItemPoolCollection() { return mItemPoolCollection; }
    
    BlazeRpcError cleanupRemoteInstanceData(InstanceId instanceId);
    static bool getThrottleTimeExceeded(EA::TDF::TimeValue startTime);

    bool hasObjectsOwnedByInstance(InstanceId instanceId) const;
    BlazeRpcError DEFINE_ASYNC_RET(waitForCollectionVersion(ReplicatedMapVersion version));
    ReplicatedMapVersion getRemoteVersion(InstanceId instanceId) const;
    void setRemoteVersion(ReplicatedMapVersion version);
    void signalVersionWaiters(InstanceId instanceId, BlazeRpcError rc);
    bool subscribeToObject(const ReplicationObjectSubscribe& update) const;
    void enablePreviousSnapshot() { mEnablePreviousSnapshot = true; }
    bool getEnablePreviousSnapshot() const { return mEnablePreviousSnapshot; }

private:
    static ReplicatedMapVersion getInvalidVersion() { return INVALID_MAP_VERSION; }

    void processCleanupRemoteInstanceData();
    bool cleanupDataFromInstance(InstanceId instanceId);
    
    friend class RemoteReplicatedMapBase;
    friend class SharedCollection;
        
    typedef eastl::vector<Fiber::EventHandle > SubscriptionEventList;
    typedef eastl::map<CollectionId, RemoteReplicatedMapBase*> RemoteReplicatedMaps;
    typedef eastl::hash_map<InstanceId, RemoteReplicatedMaps> RemoteReplicatedMapsByInstanceId;
    typedef eastl::hash_map<InstanceId, Fiber::EventHandle> CleanupEventHandlesByInstanceId;
    typedef eastl::vector<ReplicatedMapFactoryBase*> MapFactories;
    typedef eastl::hash_set<InstanceId> InstanceIdSet;

    ComponentId mComponentId;     // The component for which this MapCollection manages ReplicatedMaps
    MemoryGroupId mMemoryId;      // The memory group id we create memory with
    RemoteReplicatedMaps mReplMaps;  // A map from CollectionId to RemoteReplicatedMapBase
    RemoteReplicatedMapsByInstanceId mDynamicReplMapsByInstanceId;
    RemoteReplicatedMaps mStaticReplMaps;
    MapFactories mFactories;
    
    Replicator& mReplicator; // Replicator that owns this instance
    SubscriptionEventList mSyncWaitList;
    InstanceIdSet mSyncComplete;
    ReplicatedDataMediator& mMediator;
    ReplicatedItemPoolCollection mItemPoolCollection;
    CleanupEventHandlesByInstanceId mCleanupEventHandlesByInstanceId;
    Fiber::FiberId mInstanceCleanupFiberId;
    int32_t mLogCategory;
    bool mShutdown;
    bool mEnablePreviousSnapshot;

    typedef eastl::hash_map<uint32_t, ReplicatedMapVersion> InstanceIdToVersionMap;
    InstanceIdToVersionMap mRemoteVersions;
    //Map of fibers waiting for a specific version of the map.
    typedef eastl::multimap<ReplicatedMapVersion, Fiber::EventHandle> WaitForVersionMap;
    WaitForVersionMap mWaitForVersionMap;
};

} // Blaze

#endif // BLAZE_REMOTEMAPCOLLECTION_H
