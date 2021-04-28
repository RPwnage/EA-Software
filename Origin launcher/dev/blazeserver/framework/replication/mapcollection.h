/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_MAPCOLLECTION_H
#define BLAZE_MAPCOLLECTION_H

/*** Include files *******************************************************************************/
#include "framework/replication/replication.h"
#include "framework/connection/session.h"
#include "EASTL/map.h"
#include "EASTL/vector.h"
#include "EASTL/vector_set.h"
#include "EASTL/vector_map.h"
#include "eathread/eathread_rwmutex.h"
#include "eathread/shared_ptr_mt.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class InetAddress;
class Replicator;
class ReplicatedMapBase;
class ReplicatedMapFactoryBase;
class ReplicationSubscribeRequest;
class ReplicationObjectSubscribe;
class Selector;
class SlaveSession;
class DynamicMapSubscriber;

typedef EA::Thread::shared_ptr_mt<EA::TDF::Tdf> SharedTdf;

class MapCollectionBase : public SlaveSessionListener
{
    NON_COPYABLE(MapCollectionBase);
public:
    MapCollectionBase(ComponentId componentId, MemoryGroupId memoryGroupId, bool localSubscription);
    ~MapCollectionBase() override {};
    
    void addLocalSubscriber();
    void addSlaveSubscriber(SlaveSession& session, const ReplicationSubscribeRequest& req);
    void removeSlaveSubscriber(SlaveSession& session) { onSlaveSessionRemoved(session); }
    void onSlaveSessionRemoved(SlaveSession& session) override;
  
    int32_t getLogCategory() const { return mLogCategory; }
    ComponentId getComponentId() const { return mComponentId; }

    void sendSyncCreateNotice(SlaveSession* session, const CollectionId& id) const;
    void sendSyncUpdateNotice(SlaveSession* session, const CollectionId& id, ObjectId ownerId, EA::TDF::Tdf& tdf, ReplicatedMapVersion version) const;

    bool isLocalSubscription() const { return mLocalSubscription; }

    ReplicatedMapVersion getVersion() const { return mVersion; }
    ReplicatedMapVersion getNextVersion() const;
    void updateObjectSubscription(const ReplicationObjectSubscribe& subscribe);

    // Only applicable to MapCollection
    virtual void destroyAllLocalMaps() {};

protected:
    virtual void sync(SlaveSession* session) = 0;
    virtual EA::TDF::Tdf* findItem(const CollectionId& collectionId, ObjectId objectId) = 0;
    bool isSubscribedForPartialReplication(ReplicatedMapType mapType, SlaveSession* session) const;
    uint32_t getInstanceId() const;

public:
    // Notify any slave sessions of a change
    void sendCreateNotification(const CollectionId& collectionId, const EA::TDF::Tdf* context);
    void sendDestroyNotification(const CollectionId& collectionId, const EA::TDF::Tdf* context);
    void sendUpdateNotification(const CollectionId& collectionId, ObjectId objectId, const EA::TDF::Tdf& item, const EA::TDF::Tdf* context, bool incrementalUpdate);
    void sendEraseNotification(const CollectionId& collectionId, ObjectId objectId, const EA::TDF::Tdf* context);

private:
    typedef eastl::vector<InstanceId> InstanceIdList;
    void sendNotificationToSlaves(uint16_t notificationType, const EA::TDF::Tdf* tdf, bool onlySendChanges, InstanceId* instances, size_t instanceCount) const;
    void getSlavesAffectedByObjectUpdate(const CollectionId& collectionId, ObjectId objectId, InstanceIdList& instances) const;
    ReplicatedMapVersion incrVersion();

protected:
    typedef eastl::vector_set<InstanceId> SubscriberSet;
    typedef eastl::vector_map<ReplicatedMapType, SubscriberSet> SubscriberByCollectionTypeMap;
    typedef eastl::hash_map<ObjectId, SubscriberSet> SubscriberByObjectIdMap;
    typedef eastl::map<CollectionId, SubscriberByObjectIdMap> SubscriberByCollectionMap;

    ComponentId mComponentId;     // The component for which this MapCollectionBase manages ReplicatedMaps
    MemoryGroupId mMemoryId;    //The memory id this collection should use when creating objects
    bool mLocalSubscription;
    SubscriberSet mSubscriberSet;
    SubscriberByCollectionTypeMap mSubscriberByCollectionTypeMap;
    SubscriberByCollectionMap mSubscriberByCollectionMap;

private:
    int32_t mLogCategory;
    ReplicatedMapVersion mVersion;
};

class MapCollection : public MapCollectionBase
{
    NON_COPYABLE(MapCollection);
public:
    MapCollection(ComponentId componentId, MemoryGroupId memoryGroupId) : MapCollectionBase(componentId, memoryGroupId, false), mReplMaps(BlazeStlAllocator("MapCollectionReplicatedMaps", memoryGroupId))
 {}
    ~MapCollection() override;

    bool mapExists(const CollectionId& collectionId) const;
    void addMap(ReplicatedMapBase& map);
    template <class MapType>
    MapType* createDynamicMap(const CollectionIdRange &mapRange, const CollectionId& collectionId, const EA::TDF::Tdf *context = nullptr);
    void destroyDynamicMap(const CollectionId& collectionId, const EA::TDF::Tdf *context = nullptr);
    // Clear all maps and destroy allocated objects
    void destroyAllLocalMaps() override;
    template <class MapType>
    MapType* getDynamicMap(const CollectionIdRange &mapRange, const CollectionId& collectionId);
    template <class MapType>
    const MapType* getDynamicMap(const CollectionIdRange &mapRange, const CollectionId& collectionId) const;
    ReplicatedMapBase* getMap(const CollectionId& collectionId);
    const ReplicatedMapBase* getMap(const CollectionId& collectionId) const;

protected:
    void sync(SlaveSession* session) override;
    EA::TDF::Tdf* findItem(const CollectionId& collectionId, ObjectId objectId) override;

private:
    friend class ReplicatedMapBase;

    typedef eastl::map<CollectionId, ReplicatedMapBase*> ReplicatedMaps;

    ReplicatedMaps mReplMaps;  // A map from CollectionId to ReplicatedMapBase
};

class SharedCollection : public MapCollectionBase
{
    NON_COPYABLE(SharedCollection);
public:
    SharedCollection(ComponentId componentId, MemoryGroupId memoryGroupId) : MapCollectionBase(componentId, memoryGroupId, true) {}
    ~SharedCollection() override {};

    template <class MapType>
    MapType* createDynamicMap(const CollectionIdRange &mapRange, const CollectionId& collectionId, const EA::TDF::Tdf *context);
    void destroyDynamicMap(const CollectionId& collectionId, const EA::TDF::Tdf *context);
    template <class MapType>
    MapType* getDynamicMap(const CollectionIdRange &mapRange, const CollectionId& collectionId);
    template <class MapType>
    const MapType* getDynamicMap(const CollectionIdRange &mapRange, const CollectionId& collectionId) const;
    RemoteReplicatedMapBase* getMap(const CollectionId& collectionId);
    const RemoteReplicatedMapBase* getMap(const CollectionId& collectionId) const;

protected:
    void sync(SlaveSession* session) override;
    EA::TDF::Tdf* findItem(const CollectionId& collectionId, ObjectId objectId) override;
};

template <class MapType>
MapType *MapCollection::createDynamicMap(const CollectionIdRange &mapRange, const CollectionId& collectionId, const EA::TDF::Tdf *context)
{
    MapType *result = nullptr;
    if (mapRange.isInBounds(collectionId))
    {
        if (!mapExists(collectionId))        
        {
            result = BLAZE_NEW_MGID(mMemoryId, "DynamicReplicatedMap") MapType(collectionId, mMemoryId, *this, true);
            addMap(*result);
            sendCreateNotification(collectionId, context);
        }
        else
        {
            BLAZE_ERR_LOG(Log::REPLICATION, "[MapCollection] Dynamic map already created for collection id " << collectionId.getComponentId() 
                         << "," << collectionId.getCollectionNum() << ".");
        }
    }
    else
    {
        BLAZE_ERR_LOG(Log::REPLICATION, "[MapCollection] Map out of range (" <<  mapRange.getComponentId() << ", " << mapRange.getStartId()
                      << " - " << mapRange.getEndId() << ") with collection id " << collectionId.getComponentId() << "," << collectionId.getCollectionNum() << ".");
    }

    return result;
}

template <class MapType>
MapType *MapCollection::getDynamicMap(const CollectionIdRange &mapRange, const CollectionId& collectionId)
{
    MapType *result = nullptr;
    if (mapRange.isInBounds(collectionId))
    {
        result = (MapType *) getMap(collectionId);
    }
    else
    {
        BLAZE_ERR_LOG(Log::REPLICATION, "[MapCollection] Map out of range (" << mapRange.getComponentId() << ", " << mapRange.getStartId() << " - "
                      << mapRange.getEndId() << ") with collection id " << collectionId.getComponentId() <<"," << collectionId.getCollectionNum() << ".");
    }

    return result;
}

template <class MapType>
const MapType *MapCollection::getDynamicMap(const CollectionIdRange &mapRange, const CollectionId& collectionId) const
{
    MapType *result = nullptr;
    if (mapRange.isInBounds(collectionId))
    {
        result = (MapType *) getMap(collectionId);
    }
    else
    {
        BLAZE_ERR_LOG(Log::REPLICATION, "[MapCollection] Map out of range (" << mapRange.getComponentId() << ", " << mapRange.getStartId() << " - "
            << mapRange.getEndId() << ") with collection id " << collectionId.getComponentId() <<"," << collectionId.getCollectionNum() << ".");
    }

    return result;
}

template <class MapType>
MapType *SharedCollection::createDynamicMap(const CollectionIdRange &mapRange, const CollectionId& collectionId, const EA::TDF::Tdf *context)
{
    EA_ASSERT(collectionId.getCollectionNum() <= INSTANCE_KEY_MAX_KEY_BASE_64);
    MapType *result = nullptr;
    if (mapRange.isInBounds(collectionId))
    {
        CollectionId shardedId(collectionId.getComponentId(),
                               BuildInstanceKey64(getInstanceId(), collectionId.getCollectionNum()),
                               collectionId.getCollectionType());
        if (getMap(shardedId) == nullptr)
        {
            sendCreateNotification(shardedId, context);
            result = (MapType *) getMap(shardedId);
        }
        else
        {
            BLAZE_ERR_LOG(Log::REPLICATION, "[SharedCollection] Dynamic map already created for collection id " << collectionId.getComponentId() 
                          << "," << collectionId.getCollectionNum() << ".");
        }
    }
    else
    {
        BLAZE_ERR_LOG(Log::REPLICATION, "[SharedCollection] Map out of range (" <<  mapRange.getComponentId() << ", " << mapRange.getStartId()
                      << " - " << mapRange.getEndId() << ") with collection id " << collectionId.getComponentId() << "," << collectionId.getCollectionNum() << ".");
    }

    return result;
}

template <class MapType>
MapType *SharedCollection::getDynamicMap(const CollectionIdRange &mapRange, const CollectionId& collectionId)
{
    MapType *result = nullptr;
    if (mapRange.isInBounds(collectionId))
    {
        result = (MapType *) getMap(collectionId);
    }
    else
    {
        BLAZE_ERR_LOG(Log::REPLICATION, "[SharedCollection] Map out of range (" << mapRange.getComponentId() << ", " << mapRange.getStartId() << " - "
                      << mapRange.getEndId() << ") with collection id " << collectionId.getComponentId() <<"," << collectionId.getCollectionNum() << ".");
    }

    return result;
}

template <class MapType>
const MapType *SharedCollection::getDynamicMap(const CollectionIdRange &mapRange, const CollectionId& collectionId) const
{
    MapType *result = nullptr;
    if (mapRange.isInBounds(collectionId))
    {
        result = (MapType *) getMap(collectionId);
    }
    else
    {
        BLAZE_ERR_LOG(Log::REPLICATION, "[SharedCollection] Map out of range (" << mapRange.getComponentId() << ", " << mapRange.getStartId() << " - "
            << mapRange.getEndId() << ") with collection id " << collectionId.getComponentId() <<"," << collectionId.getCollectionNum() << ".");
    }

    return result;
}

} // Blaze

#endif // BLAZE_COLLECTION_H
