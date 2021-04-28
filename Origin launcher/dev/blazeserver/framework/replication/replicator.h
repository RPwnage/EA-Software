/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REPLICATOR_H
#define BLAZE_REPLICATOR_H

/*** Include files *******************************************************************************/

#include "framework/replication/replication.h"
#include "framework/replication/mapfactory.h"
#include "framework/replication/remotemapcollection.h"

#include "EASTL/map.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class Controller;
class CollectionId;
class ReplicationMapCreationUpdate;
class ReplicationMapDeletionUpdate;
class ReplicationUpdate;
class ReplicationDeletionUpdate;
class ReplicationMapSynchronizationComplete;
class ReplicationSynchronizationComplete;
class ReplicatedDataMediator;
class ReplicationUpdateWrapper;
class FiberManager;
class ReplicationConfig;
class MapCollectionBase;


class Replicator
{
public:
    Replicator(Controller& controller);
    virtual ~Replicator();
    
    void enablePreviousSnapshot(ComponentId componentId);
    void registerLocalCollection(MapCollectionBase& collection);

    typedef ReplicatedDataMediator *(*DataMediatorFactoryFunc)();
    ReplicatedDataMediator* registerRemoteComponent(uint16_t componentId, MemoryGroupId memoryId, DataMediatorFactoryFunc mediatorFunc);

    template <class ReplicatedMapType>
    ReplicatedMapType* registerRemoteStaticMap(const CollectionId &collectionId, MemoryGroupId memoryId, SharedCollection* sharedCollection = nullptr);

    template <class ReplicatedMapType>
    void registerRemoteDynamicMapRange(const CollectionIdRange &range, MemoryGroupId memoryId, SharedCollection* sharedCollection = nullptr);

    template <class ReplicatedMapType>
    void subscribeToDynamicMapFactory(const CollectionIdRange &range, typename ReplicatedMapType::MapFactorySubscriber& subscriber);
    template <class ReplicatedMapType>
    void unsubscribeToDynamicMapFactory(const CollectionIdRange &range, typename ReplicatedMapType::MapFactorySubscriber& subscriber);

    BlazeRpcError DEFINE_ASYNC_RET(synchronizeRemoteCollection(Component& component, bool waitForCompletion = true, InstanceId selectedInstanceId = INVALID_INSTANCE_ID));
    BlazeRpcError DEFINE_ASYNC_RET(desynchronizeRemoteCollection(Component& component, InstanceId instanceId));
    BlazeRpcError DEFINE_ASYNC_RET(waitForSyncCompletion(ComponentId component));
    BlazeRpcError DEFINE_ASYNC_RET(waitForMapCreation(const CollectionId &collectionId, RemoteReplicatedMapBase* &map));
    BlazeRpcError DEFINE_ASYNC_RET(cleanupRemoteCollection(Component& component, InstanceId instanceId));

    void shutdown();
    void destroy();
   
    RemoteMapCollection* getRemoteCollection(ComponentId componentId);

    RemoteReplicatedMapBase* getRemoteMap(const CollectionId& collectionId);    
    void getRemoteMaps(const CollectionIdRange &range, RemoteReplicatedMapList &list);
   
    //Replicated data event handlers
    bool handleReplicationNotification(Notification& notification);
    void onDynamicMapCreated(const ReplicationMapCreationUpdate& data);
    void onDynamicMapDeleted(const ReplicationMapDeletionUpdate& data);
    void onReplicatedItemUpdated(ReplicationUpdate& data);
    void onReplicatedItemDeleted(const ReplicationDeletionUpdate& data);
    void onReplicatedItemSubscribed(const ReplicationObjectSubscribe& update);
    void onMapSynchronizationComplete(const ReplicationMapSynchronizationComplete& data); 
    void onSynchronizationComplete(const ReplicationSynchronizationComplete& data);    
    
    EA::TDF::TimeValue getWaitForMapVersionTimeout() const;
    EA::TDF::TimeValue getSyncThrottleInterval() const;
    uint32_t getSyncThrottleItemCount() const;
    bool isShuttingDown() const;

private:
    friend class MapCollection;
    friend class RemoteMapCollection;

    typedef eastl::map<ComponentId, RemoteMapCollection*> RemoteComponentCollections;
    typedef eastl::map<ComponentId, MapCollectionBase*> LocalComponentCollections;
    typedef eastl::vector< eastl::pair<CollectionId, Fiber::EventHandle> > WaitForCreationVector;

    RemoteComponentCollections mRemoteComponentCollections;
    LocalComponentCollections mLocalComponentCollections;
    WaitForCreationVector mWaitForCreationVector;

    bool mShutdown;    
};


/************************************************************************/
/* Create Map Functions                                                 */
/************************************************************************/

/*! ***************************************************************************/
/*! \brief Registers a remote static map with the given collection id.

    This function creates a static map for use with the replicator.  Before 
    calling this function ensure the component collection has been registered
    with "registerRemoteComponent". 

    \param collectionId The id of the map to register.
    \return  The map, or nullptr if there was an error.
*******************************************************************************/
template <class ReplicatedMapType>
ReplicatedMapType* Replicator::registerRemoteStaticMap(const CollectionId& collectionId, MemoryGroupId memoryId, SharedCollection* sharedCollection)
{
    ReplicatedMapType* result = nullptr;
    RemoteMapCollection* collection = getRemoteCollection(collectionId.getComponentId());   

    if (collection != nullptr)
    {
        result = BLAZE_NEW_MGID(memoryId, "StaticReplicatedMap") ReplicatedMapType(collectionId, *collection, memoryId, false, collection->getMediator(), collection->getReplicatedItemPoolCollection(), sharedCollection);
        collection->addMap(*result);
    }
    else if (!mShutdown)
    {
        BLAZE_ERR_LOG(Log::REPLICATION, "[Replicator].registerStaticMap: Component collection " << collectionId.getComponentId() << " not found, cannot register map.");
    }

    return result;
}

/*! ***************************************************************************/
/*! \brief Registers a range of ids for a specific remote map type.

    This function registers the ids in the given range such that when a dynamic
    map is created it will be created with that type.  Before calling this function 
    ensure the component collection has been registered with "registerXXXXXComponent". 

    \param range  The range to register.
    \param subscriber  An optional factory subscriber to listen for events on this range.
*******************************************************************************/
template <class ReplicatedMapType>
void Replicator::registerRemoteDynamicMapRange(const CollectionIdRange& range, MemoryGroupId memoryId, SharedCollection* sharedCollection)
{
    RemoteMapCollection* collection = getRemoteCollection(range.getComponentId());  

    if (collection != nullptr)
    {
        collection->addFactory(*(BLAZE_NEW_MGID(memoryId, "ReplicatedMapFactory") typename ReplicatedMapType::FactoryType(range, memoryId, collection->getMediator(), collection->getReplicatedItemPoolCollection(), sharedCollection)));
    }
    else if (!mShutdown)
    {
        BLAZE_ERR_LOG(Log::REPLICATION, "[Replicator].registerDynamicMapRange: Component collection " << range.getComponentId() << " not found, cannot register dynamic range.");
    }
}


/*! ***************************************************************************/
/*! \brief Registers a subscriber to receive create/delete map notifications for a specific range.

    \param range    Range of ids to recieve notifications for.
    \param subscriber The subscriber to add.
*******************************************************************************/
template <class ReplicatedMapType>
void Replicator::subscribeToDynamicMapFactory(const CollectionIdRange &range, 
                                              typename ReplicatedMapType::MapFactorySubscriber& subscriber)
{
    RemoteMapCollection* collection = getRemoteCollection(range.getComponentId());  

    if (collection != nullptr)
    {
        collection->addMapFactorySubscriber(range, &subscriber);
    }
    else if (!mShutdown)
    {
        BLAZE_ERR_LOG(Log::REPLICATION, "[Replicator].subscribeDynamicMapFactory: Component collection " << range.getComponentId() << " not found.");
    }
}


/*! ***************************************************************************/
/*! \brief Unsubscribers a create/delete map notification listener from a range of map ids.

    \param range Range to unsubscribe from.
    \param subscriber Subscriber to remove.
*******************************************************************************/
template <class ReplicatedMapType>
void Replicator::unsubscribeToDynamicMapFactory(const CollectionIdRange& range, 
                                              typename ReplicatedMapType::MapFactorySubscriber& subscriber)
{
    RemoteMapCollection* collection = getRemoteCollection(range.getComponentId());  

    if (collection != nullptr)
    {
        collection->removeMapFactorySubscriber(range, &subscriber);
    }
    else if (!mShutdown)
    {
        BLAZE_ERR_LOG(Log::REPLICATION, "[Replicator].unsubscribeDynamicMapFactory: Component collection " << range.getComponentId() << " not found.");
    }
}

extern EA_THREAD_LOCAL Replicator* gReplicator;

} // Blaze

#endif // BLAZE_REPLICATOR_H
