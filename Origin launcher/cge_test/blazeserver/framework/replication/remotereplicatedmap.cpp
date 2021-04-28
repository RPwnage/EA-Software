/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/system/fiber.h"
#include "framework/replication/replicateditempool.h"
#include "framework/replication/remotereplicatedmap.h"
#include "framework/replication/mapcollection.h"
#include "framework/replication/replicator.h"
#include "EATDF/tdf.h"
#include "framework/controller/controller.h"
#include "EASTL/map.h"

namespace Blaze
{

/*! ***********************************************************************/
/*! \class RemoteReplicatedMapBase
    \brief The base class type for all remote replicated maps.

    This class holds base functionality common to remote replicated maps
    regardless of type.
***************************************************************************/


RemoteReplicatedMapBase::RemoteReplicatedMapBase(const CollectionId& collectionId, RemoteMapCollection& owner,
    const MemoryGroupId& memoryId, bool dynamic, ReplicatedItemPoolCollection& poolCollection, size_t poolItemSize, SharedCollection* sharedCollection)
    : mMemoryId(memoryId),
      mItemPool(poolCollection.createPoolAllocator(poolItemSize)),
      mSharedCollection(sharedCollection),
      mRefCountMap(BlazeStlAllocator("RemoteReplicatedMapBase::mRefCountMap")),
      mLastReceivedSubscriptionIds(BlazeStlAllocator("RemoteReplicatedMapBase::mLastReceivedSubscriptionIds")),
      mOwner(owner),
      mCollectionId(collectionId),
      mNextSubcriptionId(FORCED_REPLICATION_SUBSCRIPTION_ID + 1),
      mDynamic(dynamic),
      mShutdown(false)
{
}

RemoteReplicatedMapBase::~RemoteReplicatedMapBase()
{
    shutdown();
}

template <>
EA::TDF::Tdf* RemoteReplicatedMapBase::createTdf<void>(MemoryGroupId memoryId)
{
    return nullptr;
}

bool RemoteReplicatedMapBase::hasObjectsOwnedByInstance(InstanceId instanceId) const
{
    return mOwner.hasObjectsOwnedByInstance(instanceId);
}

InstanceId RemoteReplicatedMapBase::getInstanceId() const
{
    return gController->getInstanceId();
}

uint32_t RemoteReplicatedMapBase::getThrottleItemCount() const
{
    return gReplicator->getSyncThrottleItemCount();
}


/*! ***************************************************************************/
/*! \brief Causes the fiber to block until the map is at the specified version.
    
    \param version The version to wait for.
*******************************************************************************/
BlazeRpcError RemoteReplicatedMapBase::waitForMapVersion(ReplicatedMapVersion version)
{
    if (EA_UNLIKELY(mShutdown))
        return ERR_SYSTEM;

    // we now track the version at the collection level, so call into the owner to set up the wait.
    return mOwner.waitForCollectionVersion(version);
}

void RemoteReplicatedMapBase::setRemoteVersion(ReplicatedMapVersion version)
{
    // we now track the version at the collection level, so call into the owner to set the version.
    mOwner.setRemoteVersion(version);
}

void RemoteReplicatedMapBase::cancelWaiters(InstanceId instanceId, BlazeRpcError rc)
{
    // we now track the version at the collection level, so call into the owner to signal waiters.
    mOwner.signalVersionWaiters(instanceId, rc);
    for (WaitForSubscriptionMap::iterator it = mWaitForSubscriptionResponseMap.begin(); it != mWaitForSubscriptionResponseMap.end();)
    {
        if (GetInstanceIdFromInstanceKey64(it->first) == instanceId)
        {
            Fiber::EventHandle ev = it->second;
            mWaitForSubscriptionResponseMap.erase(it);
            Fiber::signal(ev, rc);
            it = mWaitForSubscriptionResponseMap.begin();
        }
        else
            ++it;
    }
    for (WaitForObjectMap::iterator it = mWaitForObjectMap.begin(); it != mWaitForObjectMap.end();)
    {
        if (GetInstanceIdFromInstanceKey64(it->first) == instanceId)
        {
            Fiber::EventHandle ev = it->second;
            mWaitForObjectMap.erase(it);
            Fiber::signal(ev, rc);
            it = mWaitForObjectMap.begin();
        }
        else
            ++it;
    }
}

void RemoteReplicatedMapBase::signalSubscribeWaiters(ObjectId objId, ReplicationSubscriptionId subscriptionId, BlazeRpcError rc)
{
    // Handle object waiters(if any)
    WaitForObjectMap::iterator oit = mWaitForObjectMap.find(objId);
    // 99.999% of the time we bail early;
    // this is why we implement if/do while()
    if (oit != mWaitForObjectMap.end())
    {
        do
        {
            Fiber::EventHandle ev = oit->second;
            mWaitForObjectMap.erase(oit);
            Fiber::signal(ev, rc);
            oit = mWaitForObjectMap.find(objId);
        }
        while (oit != mWaitForObjectMap.end());
    }

    // Now handle subscription waiters (if any)
    if (subscriptionId == INVALID_REPLICATION_SUBSCRIPTION_ID)
        return;

    InstanceIdToReplicationSubscriptionIdMap::insert_return_type ret = mLastReceivedSubscriptionIds.insert(eastl::make_pair(GetInstanceIdFromInstanceKey64(objId), subscriptionId));
    if (!ret.second)
    {
        if (ret.first->second < subscriptionId)
        {
            ret.first->second = subscriptionId;
        }
        else if (subscriptionId != FORCED_REPLICATION_SUBSCRIPTION_ID)
        {
            EA_FAIL_MESSAGE("RemoteReplicatedMapBase::signalSubscribeWaiters: Received out of order subscription, aborting...");
            return;
        }
    }

    if (subscriptionId == FORCED_REPLICATION_SUBSCRIPTION_ID)
    {
        // If this instance is configured with immediatePartialLocalUserRepliction = true, it may receive replication updates for 'mapped' UserSessions before it subscribes to them. 
        // Nonetheless, subscription refcounts *must* be maintained to avoid this usersession disappearing (e.g: if an internal UserSession subscription also references the same session).
        // We associate the refcount with the sentinel FORCED_REPLICATION_SUBSCRIPTION_ID here to ensure that calls to subscribeToObject() can always early out when this refcount is detected.
        ++(mRefCountMap.insert(eastl::make_pair(objId, RefCountEntry(0, subscriptionId))).first->second.count);
        return;
    }

    WaitForSubscriptionMap::iterator it = mWaitForSubscriptionResponseMap.find(subscriptionId);
    // 99.999% of the time we bail early;
    // this is why we implement if/do while()
    if (it == mWaitForSubscriptionResponseMap.end())
        return; // early out
    do
    {
        Fiber::EventHandle ev = it->second;
        mWaitForSubscriptionResponseMap.erase(it);
        Fiber::signal(ev, rc);
        it = mWaitForSubscriptionResponseMap.find(subscriptionId);
    }
    while (it != mWaitForSubscriptionResponseMap.end());

    
}

ReplicatedMapVersion RemoteReplicatedMapBase::getRemoteVersion(InstanceId instanceId) const
{
    // we now track the version at the collection level, so call into the owner to get the version.
    return mOwner.getRemoteVersion(instanceId);
}

void RemoteReplicatedMapBase::shutdown()
{
    mShutdown = true;
}

BlazeRpcError RemoteReplicatedMapBase::waitForSubscriptionResponse(ObjectId objectId, ReplicationSubscriptionId subscriptionId, const TimeValue& absTimeout)
{
    if (subscriptionId != INVALID_REPLICATION_SUBSCRIPTION_ID)
    {
        InstanceIdToReplicationSubscriptionIdMap::const_iterator itr = mLastReceivedSubscriptionIds.find(GetInstanceIdFromInstanceKey64(objectId));
        if (itr != mLastReceivedSubscriptionIds.end() && subscriptionId <= itr->second)
            return ERR_OK;

        Fiber::EventHandle& ev = mWaitForSubscriptionResponseMap.insert(eastl::make_pair(subscriptionId, Fiber::getNextEventHandle()))->second;
        return Fiber::wait(ev, "RemoteReplicatedMapBase::waitForSubscriptionResponse", absTimeout);
    }

    return ERR_OK;
}

ReplicationSubscriptionId RemoteReplicatedMapBase::subscribeToObject(ObjectId objectId)
{
    ReplicationSubscriptionId result = INVALID_REPLICATION_SUBSCRIPTION_ID;
   
    ObjectToRefCountMap::insert_return_type ret = mRefCountMap.insert(eastl::make_pair(objectId, RefCountEntry(1, mNextSubcriptionId)));
    if (ret.second)
    {
        BLAZE_TRACE_REPLICATION_LOG(mOwner.getLogCategory(), "[RemoteReplicatedMapBase].subscribeToObject: Creating subscription in map [" 
               << mCollectionId.getComponentId() << "/" << mCollectionId.getCollectionType() << "/" << mCollectionId.getCollectionNum() 
               << "], id [" << objectId << "] from context " << Fiber::getCurrentContext());

        //new item
        ReplicationObjectSubscribe update;
        update.setComponentId(mCollectionId.getComponentId());
        update.setCollectionType(mCollectionId.getCollectionType());
        update.setCollectionId(mCollectionId.getCollectionNum());
        update.setObjectId(objectId);
        update.setInstanceId(gController->getInstanceId());
        update.setSubscribe(true);
        
        result = mNextSubcriptionId++;
        update.setSubscriptionId(result);
        if (!mOwner.subscribeToObject(update))
        {
            result = INVALID_REPLICATION_SUBSCRIPTION_ID; 
        }
        
    }
    else
    {
        //existing item, bump the ref count
        ++ret.first->second.count;

        BLAZE_TRACE_REPLICATION_LOG(mOwner.getLogCategory(), "[RemoteReplicatedMapBase].subscribeToObject: Incrementing subscription in map [" 
            << mCollectionId.getComponentId() << "/" << mCollectionId.getCollectionType() << "/" << mCollectionId.getCollectionNum() 
            << "], id [" << objectId << "] to level " << ret.first->second.count << " from context " << Fiber::getCurrentContext());

        //Our subscription id is the subscription id of the notification that went out for the original subscribe.
        result = ret.first->second.subscriptionId;

        //if it's already come back the last received id will be at least equal to this one.  If so, don't wait.
        InstanceIdToReplicationSubscriptionIdMap::const_iterator itr = mLastReceivedSubscriptionIds.find(GetInstanceIdFromInstanceKey64(objectId));
        if (itr != mLastReceivedSubscriptionIds.end() && result <= itr->second)
        {
            result = INVALID_REPLICATION_SUBSCRIPTION_ID;
        }        
    }

    return result;
}

ReplicationSubscriptionId RemoteReplicatedMapBase::unsubscribeFromObject(ObjectId objectId)
{
    ReplicationSubscriptionId result = INVALID_REPLICATION_SUBSCRIPTION_ID;
    ObjectToRefCountMap::iterator itr = mRefCountMap.find(objectId);
    if (itr != mRefCountMap.end())
    {
        if (--itr->second.count == 0)
        {
            BLAZE_TRACE_REPLICATION_LOG(mOwner.getLogCategory(), "[RemoteReplicatedMapBase].unsubscribeFrom: Removing subscription from map [" 
                << mCollectionId.getComponentId() << "/" << mCollectionId.getCollectionType() << "/" << mCollectionId.getCollectionNum() 
                << "], id [" << objectId << "] from context " << Fiber::getCurrentContext());

            //we've cleared the last reference, so tell the remote publisher we no longer want this object
            ReplicationObjectSubscribe update;
            update.setComponentId(mCollectionId.getComponentId());
            update.setCollectionType(mCollectionId.getCollectionType());
            update.setCollectionId(mCollectionId.getCollectionNum());
            update.setObjectId(objectId);
            update.setInstanceId(gController->getInstanceId());
            update.setSubscribe(false);
            
            result = mNextSubcriptionId++;
            update.setSubscriptionId(result);
            if (!mOwner.subscribeToObject(update))
            {
                result = INVALID_REPLICATION_SUBSCRIPTION_ID; 
            }

            //toss our ref count here.  If we resubscribe, it will be added back in.
            mRefCountMap.erase(itr);
        }
        else
        {
            BLAZE_TRACE_REPLICATION_LOG(mOwner.getLogCategory(), "[RemoteReplicatedMapBase].unsubscribeFromObject: Decrementing subscription in map [" 
                << mCollectionId.getComponentId() << "/" << mCollectionId.getCollectionType() << "/" << mCollectionId.getCollectionNum() 
                << "], id [" << objectId << "] to level " << itr->second.count << " from context " << Fiber::getCurrentContext());
        }
    }
    else
    {
        BLAZE_WARN_LOG(mOwner.getLogCategory(), "[RemoteReplicatedMapBase].unsubscribeFromObject: No subscription found in map [" 
            << mCollectionId.getComponentId() << "/" << mCollectionId.getCollectionType() << "/" << mCollectionId.getCollectionNum() 
            << "], id [" << objectId << "] from context " << Fiber::getCurrentContext());
    }

    return result;
}

BlazeRpcError RemoteReplicatedMapBase::waitForObject(ObjectId objectId, bool& found, const TimeValue& absTimeout)
{
    Fiber::EventHandle& ev = mWaitForObjectMap.insert(eastl::make_pair(objectId, Fiber::getNextEventHandle()))->second;
    BlazeRpcError rc = Fiber::wait(ev, "RemoteReplicatedMapBase::waitForObject", absTimeout);
    if (rc == ERR_OK)
        found = hasObject(objectId);
    else
        found = false;
    return rc;
}

} // Blaze
