/*! ************************************************************************************************/
/*!
    \file remotereplicatedmap.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_READREPLICATEDMAP_H
#define BLAZE_READREPLICATEDMAP_H

/*** Include files *******************************************************************************/

#include "framework/replication/replicator.h"
#include "framework/replication/mapcollection.h"
#include "framework/replication/remotemapcollection.h"
#include "framework/replication/replicationcallback.h"
#include "framework/replication/replicateditem.h"
#include "framework/tdf/replicationtypes_server.h"
#include "framework/connection/selector.h"
#include "framework/component/blazerpc.h"
#include "PPMalloc/EAFixedAllocator.h"
#include "EASTL/type_traits.h"
#include "EASTL/algorithm.h"
#include "EASTL/hash_map.h"
#include "EASTL/hash_set.h"
#include "EATDF/tdfobject.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class MapCollection;
class ReplicatedItemPoolCollection;
class SharedCollection;
class Fiber;

template <class ReplicatedMapType>
class ReplicatedMapFactory;

template <class RemoteReplicatedMapSpec>
class SharedReplicatedMap;

class RemoteReplicatedMapBase : public CommonReplicatedMapBase
{
public:    
    static const uint32_t TIMEOUT_NONE = 0xFFFFFFFF;
    ~RemoteReplicatedMapBase() override;

    const CollectionId& getCollectionId() const { return mCollectionId; }

    BlazeRpcError DEFINE_ASYNC_RET(waitForMapVersion(ReplicatedMapVersion version));
    BlazeRpcError DEFINE_ASYNC_RET(waitForSubscriptionResponse(ObjectId objectId, ReplicationSubscriptionId subscriptionId, const EA::TDF::TimeValue& absTimeout));
    BlazeRpcError DEFINE_ASYNC_RET(waitForObject(ObjectId objectId, bool& found, const EA::TDF::TimeValue& absTimeout));
    ReplicationSubscriptionId subscribeToObject(ObjectId objectId);
    ReplicationSubscriptionId unsubscribeFromObject(ObjectId objectId);
    void shutdown();

    bool hasObject(ObjectId key) const { return (findObject(key) != nullptr); }
    virtual bool cleanupDataFromInstance(InstanceId instanceId, const EA::TDF::TimeValue& startTime) = 0;
    virtual size_t size() const = 0;
    /*! ***************************************************************************/
    /*! \brief Returns whether this map is a dynamic or static map.

    \return True if this is a dynamic map, false if static.
    *******************************************************************************/
    bool isDynamic() const { return mDynamic; }

    virtual const char *getInsertContextStr() const = 0;
    virtual const char *getUpdateContextStr() const = 0;
    virtual const char *getEraseContextStr() const = 0;
    virtual const char *getCreateContextStr() const = 0;
    virtual const char *getDestroyContextStr() const = 0;

    ReplicatedMapVersion getRemoteVersion(InstanceId instanceId) const;

protected:
    friend class Replicator;
    friend class MapCollection;
    friend class SharedCollection;

    RemoteReplicatedMapBase(const CollectionId& collectionId, RemoteMapCollection& owner, const MemoryGroupId& memoryId, bool dynamic, ReplicatedItemPoolCollection& poolCollection, size_t poolItemSize, SharedCollection* sharedCollection);

    ReplicatedMapVersion getInvalidVersion() const { return INVALID_MAP_VERSION; }

    void setRemoteVersion(ReplicatedMapVersion version);
    void cancelWaiters(InstanceId instanceId, BlazeRpcError rc);
    void signalSubscribeWaiters(ObjectId objectId, ReplicationSubscriptionId subscriptionId, BlazeRpcError rc);

    template <class TdfType>
    static EA::TDF::Tdf* createTdf(MemoryGroupId memoryId) { return BLAZE_NEW_MGID(memoryId, "TdfItem") TdfType(memoryId); }

protected:
    MemoryGroupId mMemoryId;
    EA::Allocator::FixedAllocatorBase& mItemPool;

    SharedCollection* mSharedCollection;

    typedef eastl::hash_set<ObjectId> ObjectIdSet;
    typedef eastl::hash_map<InstanceId, ObjectIdSet> ObjectIdSetByInstanceIdMap;

    typedef uint64_t RefCount;
    struct RefCountEntry
    {
        RefCountEntry(RefCount _count, ReplicationSubscriptionId _subscriptionId) : 
            count(_count), subscriptionId(_subscriptionId) {}
        RefCount count;
        ReplicationSubscriptionId subscriptionId;
    };
    typedef eastl::hash_map<ObjectId, RefCountEntry> ObjectToRefCountMap;
    ObjectToRefCountMap mRefCountMap;

    typedef eastl::hash_map<InstanceId, ReplicationSubscriptionId> InstanceIdToReplicationSubscriptionIdMap;
    InstanceIdToReplicationSubscriptionIdMap mLastReceivedSubscriptionIds;

    bool hasObjectsOwnedByInstance(InstanceId instanceId) const;
    InstanceId getInstanceId() const;
    uint32_t getThrottleItemCount() const;

private:
    virtual const EA::TDF::Tdf* findObject(ObjectId key) const = 0;
    virtual EA::TDF::Tdf* findObject(ObjectId key) = 0;
    virtual bool synchronizeMap(SlaveSession* session) = 0;

    //Our remote calls
    virtual bool remoteUpdate(const RemoteMapCollection& collection, ReplicationUpdate& update, bool isBlockingFiber) = 0;
    virtual void remoteErase(ObjectId key, ReplicatedMapVersion version, const EA::TDF::Tdf* context, ReplicationSubscriptionId subscriptionId) = 0;

protected:
    typedef eastl::hash_multimap<ObjectId, Fiber::EventHandle> WaitForObjectMap;
    typedef eastl::hash_multimap<ReplicationSubscriptionId, Fiber::EventHandle> WaitForSubscriptionMap;
    RemoteMapCollection& mOwner;
    CollectionId mCollectionId;
    WaitForSubscriptionMap mWaitForSubscriptionResponseMap;
    WaitForObjectMap mWaitForObjectMap;
    ReplicationSubscriptionId mNextSubcriptionId;


    bool mDynamic;
    bool mShutdown;

    NON_COPYABLE(RemoteReplicatedMapBase);
};

template <>
EA::TDF::Tdf* RemoteReplicatedMapBase::createTdf<void>(MemoryGroupId memoryId);



/*! ***********************************************************************/
/*! \class RemoteReplicatedMap
    \brief A map representative of a replicated map on a master component.

    This class is the access point for replicated data from another component.
    The map is underwritten by a hash map, and provides access operators.  There
    are no insertion operators because the map is read-only - only the proxy
    Replicator object can insert, remote and update remote items.
***************************************************************************/
template <class RemoteReplicatedMapSpec>
class RemoteReplicatedMap : public RemoteReplicatedMapBase
{   
public:
    typedef typename RemoteReplicatedMapSpec::ItemClassType ItemType;
    typedef typename RemoteReplicatedMapSpec::BaseTdfClassType BaseTdfType;
    typedef typename RemoteReplicatedMapSpec::ContextClassType ContextType;
    typedef typename RemoteReplicatedMapSpec::MediatorClassType MediatorType;
    
    typedef eastl::hash_map<ObjectId, EA::TDF::tdf_ptr<ItemType> > InnerMapType;  

    typedef ReplicatedSafePtr<ItemType> SafePtr;
    typedef RemoteReplicatedMap<RemoteReplicatedMapSpec> this_type;
    typedef SharedReplicatedMap<RemoteReplicatedMapSpec> SharedType;
    typedef ReplicatedMapFactory<this_type> FactoryType;

    /*! ***********************************************************************/
    /*! \class inner_iterator
        \brief Custom iterator for the replicated map.

        Because the underlying collection is a hashmap, the stock iterator 
        dereference is a pair.  We want to hide that from this so we override
        the dereference and arrow operators to return our object type.
    ***************************************************************************/
    template <bool isConst>
    class inner_iterator : public eastl::type_select<isConst, typename InnerMapType::const_iterator, typename InnerMapType::iterator>::type 
    {
        typedef typename eastl::type_select<isConst, typename InnerMapType::const_iterator, typename InnerMapType::iterator>::type base_type;        
    public:
        typedef typename eastl::type_select<isConst, const ItemType*, ItemType*>::type ItemPointer;

        inner_iterator() {}

        inner_iterator(const typename base_type::this_type_non_const& x) : base_type(x) {}

        inner_iterator(const inner_iterator &x) : base_type(x) {}

        ItemPointer operator*() const
        { return base_type::mpNode->mValue.second.get(); }

        ItemPointer operator->() const
        { return base_type::mpNode->mValue.second.get(); }

        /*! ***************************************************************************/
        /*! \brief Returns the object id of the value the iterator is pointing to.
        *******************************************************************************/
        ObjectId getObjectId() const
        { return base_type::mpNode->mValue.first; }
    };

    typedef inner_iterator<false> iterator;
    typedef inner_iterator<true> const_iterator;


    //This template magic allows us to refer to one of the subscriber types in the base class.
    //This does a selection on the subscriber type based on whether or not "contextType" was passed
    //as a "void" or not. 
    typedef typename eastl::type_select<eastl::is_void<ContextType>::value, 
        ReplicatedMapSubscriber<this_type>, ReplicatedMapContextSubscriber<this_type> >::type Subscriber;

    //Same thing but for the dynamic map factory subscriber.
    typedef typename eastl::type_select<eastl::is_void<ContextType>::value, 
        DynamicMapFactorySubscriber<this_type>, DynamicMapFactoryContextSubscriber<this_type, ContextType> >::type MapFactorySubscriber;

    static RemoteReplicatedMapSpec sSpec;

    ~RemoteReplicatedMap() override;

    size_t size() const override { return mMap.size(); }
    bool empty() const { return mMap.empty(); }

    iterator begin() { return mMap.begin(); }
    iterator end() { return mMap.end(); }

    const_iterator begin() const { return mMap.begin(); }
    const_iterator end() const { return mMap.end(); }
   
    bool exists(ObjectId key) const { return (mMap.count(key) != 0); }

    ItemType* findItem(ObjectId key);
    const ItemType* findItem(ObjectId key) const;

    bool cloneItem(ObjectId key, BaseTdfType &copy);

    void addSubscriber(Subscriber& subscriber);
    void removeSubscriber(Subscriber& subscriber);

    bool cleanupDataFromInstance(InstanceId instanceId, const EA::TDF::TimeValue& startTime) override;

    const char *getInsertContextStr() const override { return sSpec.InsertContextStr; }
    const char *getUpdateContextStr() const override { return sSpec.UpdateContextStr; }
    const char *getEraseContextStr() const override { return sSpec.EraseContextStr; }
    const char *getCreateContextStr() const override { return sSpec.CreateMapContextStr; }
    const char *getDestroyContextStr() const override { return sSpec.DestroyMapContextStr; } 

    const InnerMapType& getInnerMap() const { return mMap; }

protected:
    friend class Replicator;
    friend class MapCollection;
    friend class ReplicatedMapFactory<this_type>;

    RemoteReplicatedMap(CollectionId collectionId, RemoteMapCollection& owner, MemoryGroupId memoryId, bool dynamic, 
        ReplicatedDataMediator& mediator, ReplicatedItemPoolCollection& poolCollection, SharedCollection* sharedCollection = nullptr) : 
        RemoteReplicatedMapBase(collectionId, owner, memoryId, dynamic, poolCollection, sizeof(ItemType), sharedCollection),
        mSubscriberList(BlazeStlAllocator("ReplicatedMapSubscriberList", memoryId)),
        mMap(BlazeStlAllocator("ReplicatedMapInnerMap", memoryId)),
        mObjectIdSetByInstanceIdMap(BlazeStlAllocator("ObjectIdSetByInstanceIdMap", memoryId)),
        mMediator(static_cast<MediatorType&>(mediator)) 
    {
    }

    typedef eastl::vector<Subscriber*> SubscriberList;
    typedef typename SubscriberList::iterator SubscriberListIterator;
    SubscriberList mSubscriberList;

private:
    //Private virtuals from ReplicateMapBase
    EA::TDF::Tdf* findObject(ObjectId id) override;
    const EA::TDF::Tdf* findObject(ObjectId id) const override;
    bool synchronizeMap(SlaveSession* session) override;

    bool remoteUpdate(const RemoteMapCollection& collection, ReplicationUpdate& update, bool isBlockingFiber) override;
    void remoteErase(ObjectId key, ReplicatedMapVersion version, const EA::TDF::Tdf* context, ReplicationSubscriptionId subscriptionId) override;

protected:
    //The actual map
    InnerMapType mMap;
    ObjectIdSetByInstanceIdMap mObjectIdSetByInstanceIdMap; // used for non-blocking cleanup
    MediatorType& mMediator;
};


template<class RemoteReplicatedMapSpec>
RemoteReplicatedMap<RemoteReplicatedMapSpec>::~RemoteReplicatedMap()
{
    iterator i = mMap.begin();
    iterator e = mMap.end();
    for(; i != e; ++i)
        (*i)->decRef();
}

template<class RemoteReplicatedMapSpec>
const typename RemoteReplicatedMap<RemoteReplicatedMapSpec>::ItemType *RemoteReplicatedMap<RemoteReplicatedMapSpec>::findItem(ObjectId id) const
{
    const typename RemoteReplicatedMap<RemoteReplicatedMapSpec>::ItemType* result = nullptr;   
    
    typename InnerMapType::const_iterator itr = mMap.find(id);
    if (itr != mMap.end())
        result = itr->second;

    return result;
}

template<class RemoteReplicatedMapSpec>
typename RemoteReplicatedMap<RemoteReplicatedMapSpec>::ItemType *RemoteReplicatedMap<RemoteReplicatedMapSpec>::findItem(ObjectId id)
{
    typename RemoteReplicatedMap<RemoteReplicatedMapSpec>::ItemType* result = nullptr;

    typename InnerMapType::iterator itr = mMap.find(id);
    if (itr != mMap.end())
        result = itr->second;

    return result;
}

template<class RemoteReplicatedMapSpec>
bool RemoteReplicatedMap<RemoteReplicatedMapSpec>::cloneItem(ObjectId key, typename RemoteReplicatedMap<RemoteReplicatedMapSpec>::BaseTdfType &copy)
{
    typename RemoteReplicatedMap<RemoteReplicatedMapSpec>::ItemType* item = findItem(key);
    if (item != nullptr)
    {
        item->copyInto(copy);
    }
    
    return (item != nullptr);
}

template<class RemoteReplicatedMapSpec>
void RemoteReplicatedMap<RemoteReplicatedMapSpec>
    ::addSubscriber(Subscriber& subscriber)
{
    mSubscriberList.push_back(&subscriber);
}

template<class RemoteReplicatedMapSpec>
void RemoteReplicatedMap<RemoteReplicatedMapSpec>
    ::removeSubscriber(Subscriber& subscriber)
{
    SubscriberListIterator end = mSubscriberList.end();
    SubscriberListIterator iter = eastl::find(mSubscriberList.begin(), end, &subscriber);
    if (iter!=end)
    {
        mSubscriberList.erase(iter);
    }
}


template<class RemoteReplicatedMapSpec>
bool RemoteReplicatedMap<RemoteReplicatedMapSpec>
    ::cleanupDataFromInstance(InstanceId instanceId, const EA::TDF::TimeValue& startTime)
{
    // first abort anyone waiting on versions associated with this 
    // instance because otherwise they would wait forever.
    cancelWaiters(instanceId, ERR_DISCONNECTED);

    mLastReceivedSubscriptionIds.erase(instanceId);

    if (!hasObjectsOwnedByInstance(instanceId))
        return true;
    
    const ComponentData* compData = ComponentData::getComponentData(getCollectionId().getComponentId());
    if (compData == nullptr)
    {
        EA_FAIL_MSG("Collection has component id without component info.");
        return true; //don't continue trying to clean this data
    }

    uint64_t countErased = 0;
    uint32_t throttleItemCount = getThrottleItemCount();
    if (throttleItemCount < 1)
        throttleItemCount = 1;

    ObjectIdSetByInstanceIdMap::iterator oidSetIt = mObjectIdSetByInstanceIdMap.find(instanceId);
    if (oidSetIt != mObjectIdSetByInstanceIdMap.end())
    {
        Fiber::setCurrentContext(getDestroyContextStr());

        ObjectIdSet& objectIdSet = oidSetIt->second;
        // NOTE: We intentionally never remove the objectIdSet itself since there is only one per remote instance
        while (!objectIdSet.empty())
        {
            ObjectIdSet::iterator oidIt = objectIdSet.begin();
            ObjectId key = *oidIt;
            objectIdSet.erase(oidIt);
            typename InnerMapType::iterator mapIt = mMap.find(key);
            if (mapIt != mMap.end())
            {
                typename RemoteReplicatedMap<RemoteReplicatedMapSpec>::ItemType* oldItem = mapIt->second.get();

                // NOTE: With eastl::hash_map erase by key is faster than by iterator 
                // because it doesn't need to return a useless next item iterator
                mMap.erase(key);
                mRefCountMap.erase(key);

                //Let everyone know this was erased.
                Subscriber::makeEraseCall(mMediator, *this, key, *oldItem, nullptr);

                SubscriberListIterator si = mSubscriberList.begin();
                SubscriberListIterator se = mSubscriberList.end();
                for(; si != se; ++si)
                {
                    Subscriber::makeEraseCall(**si, *this, key, *oldItem, nullptr);
                }

                oldItem->decRef();

                ++countErased;

                if ((countErased % throttleItemCount) == 0)
                {
                    if (RemoteMapCollection::getThrottleTimeExceeded(startTime))
                        return false;
                }
            }
        }
    }
    
    return true;
}

template<class RemoteReplicatedMapSpec>
bool RemoteReplicatedMap<RemoteReplicatedMapSpec>::synchronizeMap(SlaveSession* session)
{
    BLAZE_TRACE_LOG(Log::REPLICATION, "[RemoteReplicatedMap].synchronizeMap: Starting component=" << BlazeRpcComponentDb::getComponentNameById(getCollectionId().getComponentId()) 
                    << "(" << SbFormats::Raw("0x%04x", getCollectionId().getComponentId()) << ") collection=" << getCollectionId().getCollectionNum());

    // First notify the subscriber of the map's existence (dynamic only)
    if (isDynamic())
    {
        // For dynamic maps only send entire maps that are owned by this instance,
        // return false to let the caller know they do not need to send out a map complete notification
        if (GetInstanceIdFromInstanceKey64(getCollectionId().getCollectionNum()) != getInstanceId())
            return false;

        mSharedCollection->sendSyncCreateNotice(session, getCollectionId());
    }

    // Iterate over the map and send an update notification for each entry
    iterator it = begin();
    iterator itrend = end();
    if (isDynamic())
    {
        for (; it != itrend; ++it) 
        {
            // For dynamic maps we've already checked above, so we can unconditionally send all entries
            mSharedCollection->sendSyncUpdateNotice(session, getCollectionId(), it.getObjectId(), **it, getRemoteVersion(getInstanceId()));
        }
    }
    else
    {
        for (; it != itrend; ++it) 
        {
            // For static maps, we send out only the individual entries within the map for our instance,
            if (GetInstanceIdFromInstanceKey64(it.getObjectId()) == getInstanceId())
                mSharedCollection->sendSyncUpdateNotice(session, getCollectionId(), it.getObjectId(), **it, getRemoteVersion(getInstanceId()));
        }
    }

    BLAZE_TRACE_LOG(Log::REPLICATION, "[RemoteReplicatedMap].synchronizeMap: Finished component=" << BlazeRpcComponentDb::getComponentNameById(getCollectionId().getComponentId())
                    << "(" << SbFormats::Raw("0x%04x", getCollectionId().getComponentId())  << ") collection=" << getCollectionId().getCollectionNum());

    // Return true to let the caller know they do need to send out a map complete notification
    return true;
}

template<class RemoteReplicatedMapSpec>
const EA::TDF::Tdf* RemoteReplicatedMap<RemoteReplicatedMapSpec>::findObject(ObjectId id) const
{
    return findItem(id);
}

template<class RemoteReplicatedMapSpec>
EA::TDF::Tdf* RemoteReplicatedMap<RemoteReplicatedMapSpec>::findObject(ObjectId id)
{
    return findItem(id);
}


template<class RemoteReplicatedMapSpec>
bool RemoteReplicatedMap<RemoteReplicatedMapSpec>::
    remoteUpdate(const RemoteMapCollection& collection, ReplicationUpdate& update, bool isBlockingFiber)
{
    ObjectId key = update.getObjectId();
    const EA::TDF::Tdf* context = update.getContext();
    bool insertion = true;
    bool prevSnapshot = mOwner.getEnablePreviousSnapshot();
    typename RemoteReplicatedMap<RemoteReplicatedMapSpec>::ItemType *item = findItem(key); 
    if (item != nullptr)
    {     
        insertion = false;
        if (prevSnapshot)
            item->saveSnapshot(); // save snapshot of prev item
    }
    else
    {
        if (!update.getIncrementalUpdate())
        {
            item = new(mItemPool.Malloc()) typename RemoteReplicatedMap<RemoteReplicatedMapSpec>::ItemType();
            item->setPool(&mItemPool);
            item->incRef();
            mMap.insert(eastl::make_pair(key, item));
            mObjectIdSetByInstanceIdMap[GetInstanceIdFromInstanceKey64(key)].insert(key);
        }
        else
        {
            // we can't insert an item into the map 
            // based on a partial update
            BLAZE_INFO_LOG(Log::REPLICATION, "[RemoteReplicatedMap].remoteUpdate: discarding partial update for object that doesn't exist: component=" 
                         << BlazeRpcComponentDb::getComponentNameById(update.getComponentId()) << "(" << SbFormats::Raw("0x%04x", update.getComponentId()) 
                         << ") collectionId=" << update.getCollectionId() << " collecitonType=" << update.getCollectionType() 
                         << " objid=" << update.getObjectId() << " version=" << update.getVersion() << " incrementalUpdate=" << update.getIncrementalUpdate());
            return false;
        }
    }   

    BlazeRpcError rc = ERR_OK;

    EA::TDF::MemberVisitOptions opts;
    opts.onlyIfSet = true;
    update.getObjectTdf()->copyIntoObject(*item, opts); //if its an update we extract incrementally
    if (rc != ERR_OK)
    {
        // This should never fail since the master and slave processes should always be
        // matched TDF versions.
        BLAZE_FAIL_LOG(Log::REPLICATION, "[RemoveReplicatedMap].remoteUpdate: Failed to extract TDF for object: component=" 
                     << BlazeRpcComponentDb::getComponentNameById(update.getComponentId()) << "(" << SbFormats::Raw("0x%04x", update.getComponentId()) 
                     << ") collectionId=" << update.getCollectionId() << " collecitonType=" << update.getCollectionType() 
                     << " objid=" << update.getObjectId() << " version=" << update.getVersion());
        if (item != nullptr)
        {
            if (insertion)
            {
                // remove the item.
                mMap.erase(key);
                item->decRef();
            }
            else if (prevSnapshot)
                item->resetSnapshot();
        }

        // assert because this is non-recoverable, the replicated map is corrupted.
        EA_ASSERT_MSG(false, "Decode of tdf during remoteUpdate failed!");

        return false;
    }

    BLAZE_TRACE_REPLICATION_LOG(collection.getLogCategory(),
            "update notification [component=" << BlazeRpcComponentDb::getComponentNameById(update.getComponentId()) << "(" << SbFormats::Raw("0x%04x", update.getComponentId()) 
            << ") collectionId=" << update.getCollectionId() << " collectionType=" << update.getCollectionType() << " objid=" << update.getObjectId() 
            << " version=" << update.getVersion() << "]\n" << item << ((update.getContext() == nullptr) ? "" : "\nContext:\n")
            << update.getContext());

    //Let the mediator know there was an update. 
    if (insertion)
    {
        if (!isBlockingFiber)
            Fiber::setCurrentContext(getInsertContextStr());
        Subscriber::makeInsertCall(mMediator, *this, key, *item, context); 
    }
    else
    {
        if (!isBlockingFiber)
            Fiber::setCurrentContext(getUpdateContextStr());
        Subscriber::makeUpdateCall(mMediator, *this, key, *item, context);
    }

    //Let all the subscribers know
    SubscriberListIterator i = mSubscriberList.begin();
    SubscriberListIterator e = mSubscriberList.end();
    for(; i != e; ++i)
    {
        if (insertion)
            Subscriber::makeInsertCall(**i, *this, key, *item, context); 
        else
            Subscriber::makeUpdateCall(**i, *this, key, *item, context);
    }

    //Finally set the remote version.
    setRemoteVersion(update.getVersion());

    if (prevSnapshot)
        item->resetSnapshot(); // lose previous snapshot

    return insertion;
}

template<class RemoteReplicatedMapSpec>
void RemoteReplicatedMap<RemoteReplicatedMapSpec>::
    remoteErase(ObjectId key, ReplicatedMapVersion version, const EA::TDF::Tdf* context, ReplicationSubscriptionId subscriptionId)
{
    typename RemoteReplicatedMap<RemoteReplicatedMapSpec>::ItemType *oldItem = nullptr;

    iterator find = mMap.find(key);
    if (find != mMap.end())
    {
        oldItem = *find;
        mMap.erase(find);     
    }

    ObjectIdSetByInstanceIdMap::iterator it = mObjectIdSetByInstanceIdMap.find(GetInstanceIdFromInstanceKey64(key));
    if (it != mObjectIdSetByInstanceIdMap.end())
        it->second.erase(key); // erase by key is faster than by iterator, also we intentionally never remove ObjectIdSet

    //Let everyone know this was erased.
    if (oldItem != nullptr)
    {       
        //First make any mediator call
        Subscriber::makeEraseCall(mMediator, *this, key, *oldItem, context);

        SubscriberListIterator i = mSubscriberList.begin();
        SubscriberListIterator e = mSubscriberList.end();
        for(; i != e; ++i)
        {
            Subscriber::makeEraseCall(**i, *this, key, *oldItem, context);
        }

        oldItem->decRef();
    }

    //A "true" erase will have no subscription id because we didn't initiate the erase.  If we did, leave the ref count alone.
    if (subscriptionId == INVALID_REPLICATION_SUBSCRIPTION_ID)
        mRefCountMap.erase(key);

    setRemoteVersion(version);
}

} // Blaze

#endif // BLAZE_READREPLICATEDMAP_H
