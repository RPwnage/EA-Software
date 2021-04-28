/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/connection/connectionmanager.h"
#include "framework/replication/remotemapcollection.h"
#include "framework/replication/replicator.h"
#include "framework/replication/remotereplicatedmap.h"
#include "framework/replication/replicationcallback.h"
#include "framework/replication/mapfactory.h"
#include "framework/replication/replicator.h"
#include "framework/connection/outboundconnectionmanager.h"
#include "framework/controller/controller.h"
#include "framework/component/componentmanager.h"
#include "framework/connection/socketchannel.h"
#include "framework/tdf/replicationtypes_server.h"
#include "framework/system/job.h"
#include "framework/system/fibermanager.h"
#include "framework/system/fiber.h"

#include "eathread/shared_ptr_mt.h"

// TODO: optimize notifications so that jobs aren't queued when already running
//       on the correct thread.

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


/*** Public Methods ******************************************************************************/

RemoteMapCollection::RemoteMapCollection(Replicator& replicator, ComponentId componentId, MemoryGroupId memoryId, ReplicatedDataMediator& mediator)
    : mComponentId(componentId),
    mMemoryId(memoryId),
    mReplMaps(BlazeStlAllocator("RemoteMapCollection::mReplMaps", memoryId)),
    mDynamicReplMapsByInstanceId(BlazeStlAllocator("RemoteMapCollection::mDynamicReplMapsByInstanceId", memoryId)),
    mStaticReplMaps(BlazeStlAllocator("RemoteMapCollection::mStaticReplMaps", memoryId)),
    mReplicator(replicator),
    mMediator(mediator),
    mItemPoolCollection(memoryId),
    mCleanupEventHandlesByInstanceId(BlazeStlAllocator("RemoteMapCollection::mCleanupEventHandlesByInstanceId", memoryId)),
    mInstanceCleanupFiberId(Fiber::INVALID_FIBER_ID),
    mLogCategory(BlazeRpcLog::getLogCategory(componentId)),
    mShutdown(false),
    mEnablePreviousSnapshot(false),
    mRemoteVersions(BlazeStlAllocator("RemoteMapCollection::mRemoteVersions", memoryId)),
    mWaitForVersionMap(BlazeStlAllocator("RemoteMapCollection::mWaitForVersionMap", memoryId))
{
}

RemoteMapCollection::~RemoteMapCollection()
{
    for (MapFactories::iterator fi = mFactories.begin(), fe = mFactories.end(); fi != fe; ++fi)
        delete (*fi);
        
    delete &mMediator;
}

bool RemoteMapCollection::isSynced() const
{
    // find the component, which may not be active yet in the case of slave-slave replication
    Component* component = gController->getComponent(mComponentId, false, false);
    
    if (component != nullptr)
    {
        if (component->getReplicatedInstances().size() != mSyncComplete.size())
        {
            //early out for mismatched sizes
            return false;
        }

        for (Component::InstanceIdSet::const_iterator itr = component->getReplicatedInstances().begin(), end = component->getReplicatedInstances().end(); 
            itr != end; ++itr)
        {
            if (mSyncComplete.count(*itr) == 0)
            {
                return false;
            }
        }

        return true;
    }

    return false;
}

bool RemoteMapCollection::getThrottleTimeExceeded(TimeValue startTime)
{
    return (TimeValue::getTimeOfDay() - startTime) >= gReplicator->getSyncThrottleInterval();
}

void RemoteMapCollection::addMapFactorySubscriber(const CollectionIdRange &range, void* subscriber)
{
    ReplicatedMapFactoryBase *factory = getFactory(range);
    if (factory != nullptr)
    {
        factory->addSubscriber(subscriber);
    }
    else
    {
        BLAZE_ERR_LOG(Log::REPLICATION, "[RemoteMapCollection]: Tried to add subscriber to an invalid id range.");
    }
}

void RemoteMapCollection::removeMapFactorySubscriber(const CollectionIdRange &range, void* subscriber)
{
    ReplicatedMapFactoryBase *factory = getFactory(range);
    if (factory != nullptr)
    {
        factory->removeSubscriber(subscriber);
    }
    else
    {
        BLAZE_ERR_LOG(Log::REPLICATION, "[RemoteMapCollection]: Tried to remove subscriber from an invalid id range.");
    }
}

BlazeRpcError RemoteMapCollection::waitForSyncCompletion()
{
    if (EA_UNLIKELY(mShutdown))
    {
        BLAZE_ERR_LOG(Log::REPLICATION, "[RemoteMapCollection].waitForSyncCompletion - error, collection shut down");

        return ERR_SYSTEM;
    }

    BLAZE_INFO_LOG(Log::REPLICATION, "[RemoteMapCollection].waitForSyncCompletion");

    BlazeRpcError result = ERR_OK;
    if (!isSynced())
    {       
        //We'll want to call the callback when this subscription completes
        Fiber::EventHandle ev = Fiber::getNextEventHandle();
        mSyncWaitList.push_back(ev);

        //Go to sleep, to be woken later - we can't miss this call because the replicator thread wakes 
        //us by using a job, so this function has to succeed before the job can be processed.
        result = Fiber::wait(ev, "RemoteMapCollection::waitForSyncCompletion");
        if (result == ERR_OK && !isSynced())
        {
            BLAZE_FATAL_LOG(Log::REPLICATION, "[RemoteMapCollection].waitForSyncCompletion: premature wake up!");
            result = ERR_SYSTEM;
        }

        BLAZE_INFO_LOG(Log::REPLICATION, "[RemoteMapCollection].waitForSyncCompletion: sleep finished");

    }

    return result;
}

void RemoteMapCollection::shutdown()
{
    mShutdown = true;

    if (!isSynced())
    {
        handleSynchronizationComplete(ERR_SYSTEM, gController->getInstanceId());
    }

    //Release all waiting version fibers.
    while (!mWaitForVersionMap.empty())
    {
        const Fiber::EventHandle ev = mWaitForVersionMap.begin()->second;
        mWaitForVersionMap.erase(mWaitForVersionMap.begin());
        Fiber::signal(ev, ERR_CANCELED);
    }

    RemoteReplicatedMaps::iterator mi = mReplMaps.begin();
    RemoteReplicatedMaps::iterator me = mReplMaps.end();
    for(; mi != me; ++mi)
    {
        mi->second->shutdown();
    }
}

void RemoteMapCollection::destroy()
{
    mDynamicReplMapsByInstanceId.clear();
    mStaticReplMaps.clear();
    for (RemoteReplicatedMaps::iterator mi = mReplMaps.begin(), me = mReplMaps.end(); mi != me; ++mi)
        delete mi->second;
    mReplMaps.clear();
}

void RemoteMapCollection::createRemoteDynamicMap(const CollectionId &collectionId, const EA::TDF::Tdf* context)
{
    ReplicatedMapFactoryBase *factory = getFactory(collectionId);   
    if (factory != nullptr)
    {
        if (getMap(collectionId) == nullptr)
        {
            RemoteReplicatedMapBase *map = factory->createMap(collectionId, *this);
            if (map != nullptr)
            {
                if (map->isDynamic())
                {
                    Fiber::setCurrentContext(map->getCreateContextStr());
                }
                addMap(*map);

                factory->makeCreateNotification(*map, context);
            }
            else
            {
                BLAZE_ERR_LOG(Log::REPLICATION, "[RemoteMapCollection] Could not create map for range collection id " 
                    << collectionId.getComponentId() << "," << collectionId.getCollectionNum() << "!");
            }
        }
        else
        {
            BLAZE_WARN_LOG(Log::REPLICATION, "[RemoteMapCollection] Could not create dynamic map for componentId: "
                << collectionId.getComponentId() << ", collectionNum: " << collectionId.getCollectionNum() << " as one already exists!");
        }
    }
    else
    {
        BLAZE_ERR_LOG(Log::REPLICATION, "[RemoteMapCollection] No dynamic map factory registered for range. Component "
            << collectionId.getComponentId() << " collection id " << collectionId.getCollectionNum() <<", collection type "\
            << collectionId.getCollectionType()<<"!");
    }       
}

void RemoteMapCollection::destroyRemoteDynamicMap(const CollectionId& collectionId, const EA::TDF::Tdf* context)
{
    RemoteReplicatedMaps::iterator mapItr = mReplMaps.find(collectionId);
    if (mapItr != mReplMaps.end())
    {
        RemoteReplicatedMapBase* map = mapItr->second;

        if (map->isDynamic())
        {
            Fiber::setCurrentContext(map->getDestroyContextStr());

            RemoteReplicatedMapsByInstanceId::iterator it = mDynamicReplMapsByInstanceId.find(GetInstanceIdFromInstanceKey64(collectionId.getCollectionNum()));
            if (it != mDynamicReplMapsByInstanceId.end())
            {
                // We intentionally never erase the maps collection for this instance id, 
                // because only one instance will exist for the lifetime of the server
                RemoteReplicatedMaps& maps = it->second;
                maps.erase(collectionId);
            }

            // Remove map from the list
            mReplMaps.erase(mapItr);

            //Let everyone know we killed the map.
            ReplicatedMapFactoryBase *factory = getFactory(collectionId);
            if (factory != nullptr)
            {
                factory->makeDestroyNotification(*map, context);
            }       

            delete map;
        }
        else
        {
            //This should never happen.
            BLAZE_ERR_LOG(Log::REPLICATION, "[RemoteMapCollection] Tried to delete non-dyanmic collection " << collectionId.getComponentId() 
                          << "," << collectionId.getCollectionNum() << "!");
        }
    }
    else
    {
        // This should never happen.
        BLAZE_ERR_LOG(Log::REPLICATION, "[RemoteMapCollection] Tried to delete unknown collection " << collectionId.getComponentId() 
                      << "," << collectionId.getCollectionNum() << "!");
    }
}


void RemoteMapCollection::addMap(RemoteReplicatedMapBase& mapToAdd)
{
    const CollectionId& collectionId = mapToAdd.getCollectionId();
    if (mapToAdd.isDynamic())
        mDynamicReplMapsByInstanceId[GetInstanceIdFromInstanceKey64(collectionId.getCollectionNum())][collectionId] = &mapToAdd;
    else
        mStaticReplMaps[collectionId] = &mapToAdd;

    // Insert the map into the collection
    mReplMaps.insert(eastl::make_pair(mapToAdd.getCollectionId(), &mapToAdd));      

    //If we're shutdown we still add the map, but mark it as shutdown too.
    if (EA_UNLIKELY(mShutdown))
        mapToAdd.shutdown();
}

void RemoteMapCollection::addFactory(ReplicatedMapFactoryBase& factoryToAdd)
{
    mFactories.push_back(&factoryToAdd);
}

RemoteReplicatedMapBase* RemoteMapCollection::getMap(const CollectionId& collectionId)
{
    RemoteReplicatedMapBase* map = nullptr;
    RemoteReplicatedMaps::iterator it = mReplMaps.find(collectionId);
    if (it != mReplMaps.end())
        map =  it->second;
    return map;
}

void RemoteMapCollection::getMaps(const CollectionIdRange& range, RemoteReplicatedMapList& list)
{
    RemoteReplicatedMaps::iterator it = mReplMaps.begin();
    RemoteReplicatedMaps::iterator end = mReplMaps.end();
    for (; it != end; ++it)
    {
        if (range.isInBounds(it->first))
        {
            list.push_back(it->second);
        }
    }
}

ReplicatedMapFactoryBase *RemoteMapCollection::getFactory(const CollectionId& collectionId)
{
    //Find the collection range that matches this collection id
    ReplicatedMapFactoryBase *factory = nullptr;   
    MapFactories::iterator itr = mFactories.begin();
    MapFactories::iterator end = mFactories.end();
    for (; itr != end; ++itr)
    {
        if ((*itr)->isInBounds(collectionId))
        {
            factory = *itr;
            break;
        }
    }

    return factory;
}

ReplicatedMapFactoryBase *RemoteMapCollection::getFactory(const CollectionIdRange& range)
{
    //Find the collection range that matches this collection id
    ReplicatedMapFactoryBase *factory = nullptr;   
    MapFactories::iterator itr = mFactories.begin();
    MapFactories::iterator end = mFactories.end();
    for (; itr != end; ++itr)
    {
        if ((*itr)->getRange() == range)
        {
            factory = *itr;
            break;
        }
    }

    return factory;
}

BlazeRpcError RemoteMapCollection::cleanupRemoteInstanceData(InstanceId instanceId)
{
    CleanupEventHandlesByInstanceId::insert_return_type ret = mCleanupEventHandlesByInstanceId.insert(instanceId);
    if (ret.second)
    {
        ret.first->second = Fiber::getNextEventHandle();
        if (!Fiber::isFiberIdInUse(mInstanceCleanupFiberId))
        {
            Fiber::CreateParams params(Fiber::STACK_HUGE);
            params.blocking = true;
            gSelector->scheduleFiberCall(this, &RemoteMapCollection::processCleanupRemoteInstanceData, "RemoteMapCollection::processCleanupRemoteInstanceData", params);
        }
        BlazeRpcError rc = ERR_OK;
        CleanupEventHandlesByInstanceId::iterator it = mCleanupEventHandlesByInstanceId.find(instanceId);
        if (it != mCleanupEventHandlesByInstanceId.end())
        {
            rc = Fiber::wait(it->second, "RemoteMapCollection::cleanupRemoteInstanceData -- waiting");
        }
        return rc;
    }
    else
    {
        EA_ASSERT_MSG(false, "Cleanup is already running!");
        return ERR_SYSTEM;
    }
}

void RemoteMapCollection::processCleanupRemoteInstanceData()
{
    mInstanceCleanupFiberId = Fiber::getCurrentFiberId();

    const char8_t* savedFiberContext = Fiber::getCurrentContext();

    // serially clean up all 
    while (!mCleanupEventHandlesByInstanceId.empty())
    {
        CleanupEventHandlesByInstanceId::iterator it = mCleanupEventHandlesByInstanceId.begin();

        InstanceId instanceId = it->first;
        Fiber::EventHandle ev = it->second;

        while (!cleanupDataFromInstance(instanceId)) 
        {
            if (mShutdown)
            {
                BLAZE_INFO_LOG(Log::REPLICATION, "[RemoteMapCollection].processCleanupRemoteInstanceData: "
                    "Skipping remaining replicated data cleanup for '" << BlazeRpcComponentDb::getComponentNameById(mComponentId) 
                    << "' due to local instance shutdown.");
                break;
            }
            BLAZE_INFO_LOG(Log::REPLICATION, "[RemoteMapCollection].processCleanupRemoteInstanceData: "
                "Throttling remote component '" << BlazeRpcComponentDb::getComponentNameById(mComponentId) 
                << "' replicated data cleanup.");
            Fiber::sleep(0, "Fiber::processCleanupRemoteInstanceData -- throttle");
        }

        mCleanupEventHandlesByInstanceId.erase(instanceId);

        Fiber::signal(ev, ERR_OK);
    }

    mInstanceCleanupFiberId = Fiber::INVALID_FIBER_ID;

    // restore fiber context before bailing from this fiber because replication methods mess with it
    Fiber::setCurrentContext(savedFiberContext);
}

bool RemoteMapCollection::cleanupDataFromInstance(InstanceId instanceId)
{
    //We're no longer sync'd at this point, so clear this out of the map
    mSyncComplete.erase(instanceId);

    TimeValue startTime = TimeValue::getTimeOfDay();

    Fiber::BlockingSuppressionAutoPtr suppressAutoPtr;

    RemoteReplicatedMapsByInstanceId::iterator it = mDynamicReplMapsByInstanceId.find(instanceId);
    if (it != mDynamicReplMapsByInstanceId.end())
    {
        RemoteReplicatedMaps& maps = it->second;
        while (!maps.empty())
        {
            //Go through our maps in reverse order (we enforce this to allow inter-dependencies between objects in maps)
            RemoteReplicatedMaps::reverse_iterator rit = maps.rbegin();
            //This cannot be a reference, because it's used inside destroyRemoteDynamicMap after the iterator is erased
            const CollectionId collectionId = rit->first;
            destroyRemoteDynamicMap(collectionId, nullptr);
            if (getThrottleTimeExceeded(startTime))
                return false;
        }
    }

    // Static maps never mutate
    // Go through our maps in reverse order (we enforce this to allow inter-dependencies between objects in maps)
    for (RemoteReplicatedMaps::reverse_iterator ri = mStaticReplMaps.rbegin(), re = mStaticReplMaps.rend(); ri != re; ++ri)
    {
        const CollectionId collectionId = ri->first;
        RemoteReplicatedMapBase* map = ri->second;

        BLAZE_INFO_LOG(Log::REPLICATION, "[Replicator].cleanupDataFromInstance: "
            "Begin cleanup of data owned by instance=" << instanceId << " in static map = (component=" 
            << BlazeRpcComponentDb::getComponentNameById(collectionId.getComponentId()) 
            << ", type=" << collectionId.getCollectionType() << ", num=" << collectionId.getCollectionNum() << ", size=" << map->size() << ")");

        //don't delete static maps, just their members
        if(!map->cleanupDataFromInstance(instanceId, startTime))
            return false;

        BLAZE_INFO_LOG(Log::REPLICATION, "[Replicator].cleanupDataFromInstance: "
            "End cleanup of data owned by instance=" << instanceId << " in static map = (component=" 
            << BlazeRpcComponentDb::getComponentNameById(collectionId.getComponentId()) 
            << ", type=" << collectionId.getCollectionType() << ", num=" << collectionId.getCollectionNum() << ", size=" << map->size() << ")");
    }
    
    return true;
}

// Subscription methods - These methods follow the process for connecting to a remote map collectino
// for a component and synchronizing to it. 

/*! ***************************************************************************/
/*! \brief Connects to a remote master and registers for replication, starting
           the sync process.  

           Must be run from a fiber!
*******************************************************************************/
BlazeRpcError RemoteMapCollection::startSynchronization(Component& componentToSync, InstanceId selectedIndex)
{   
    //We now have the remote replicator component - we now send a subscribe message to that 
    //replicator and have it start sending us updates.
    BLAZE_INFO_LOG(Log::REPLICATION, "[RemoteMapCollection].startSynchronization: subscribing to " << componentToSync.getName() << "(" 
                    << SbFormats::Raw("0x%04x", mComponentId) << "); ");

    // We're now connected to the appropriate remote replicator - send a subscription notice.
    BlazeRpcError err = componentToSync.replicationSubscribe(selectedIndex);

    BLAZE_INFO_LOG(Log::REPLICATION, "[Replicator].processSubscriptionResponse: "
        "subscription complete for component " << componentToSync.getName() << "(" << SbFormats::Raw("0x%04x", mComponentId) << ") result: " 
        << (ErrorHelp::getErrorName(err)) << " (" << err << ")");

    if (err != ERR_OK)
    {
        //If the subscription returns an error it means something has gone wrong at the connection level.  Dump out...
        //(Must be called from the main thread)
        gSelector->scheduleCall(this, &RemoteMapCollection::handleSynchronizationComplete, err, gController->getInstanceId());
    }
    
    return err;
}

void RemoteMapCollection::handleSynchronizationComplete(BlazeRpcError err, InstanceId instanceId)
{
    if (err == ERR_OK)
    {
        mSyncComplete.insert(instanceId);
    }

    if (isSynced() || err != ERR_OK)
    {
        //Let everyone who wanted to subscribe know that we're synced
        SubscriptionEventList syncWaitList;
        // perform a fast switch, that way we don't have to clear the mSyncWaitList below
        mSyncWaitList.swap(syncWaitList);
        SubscriptionEventList::iterator itr = syncWaitList.begin();
        SubscriptionEventList::iterator end = syncWaitList.end();
        for (; itr != end; ++itr)
        {
            //Let anyone who subscribed before the sync finished know that we're done.
            Fiber::signal(*itr, err);
        }
    }
}

bool RemoteMapCollection::hasObjectsOwnedByInstance(InstanceId instanceId) const
{
    InstanceIdToVersionMap::const_iterator it = mRemoteVersions.find(instanceId);
    return it != mRemoteVersions.end() && GetBaseFromInstanceKey64(it->second) > (getInvalidVersion() + 1);
}

/*! ***************************************************************************/
/*! \brief Causes the fiber to block until the map is at the specified version.
    
    \param version The version to wait for.
*******************************************************************************/
BlazeRpcError RemoteMapCollection::waitForCollectionVersion(ReplicatedMapVersion version)
{
    if (EA_UNLIKELY(mShutdown))
        return ERR_SYSTEM;

    BlazeRpcError result;
  
    //If not found the version will default to INVALID_MAP_VERSION.
    InstanceIdToVersionMap::const_iterator instanceItr = mRemoteVersions.find(GetInstanceIdFromInstanceKey64(version));
    ReplicatedMapVersion currentVersion = (instanceItr != mRemoteVersions.end()) ? instanceItr->second : getInvalidVersion();
    if (currentVersion == getInvalidVersion() || currentVersion < version)
    {
        Fiber::EventHandle ev = Fiber::getNextEventHandle();

        mWaitForVersionMap.insert(eastl::make_pair(version, ev));

        //Go to sleep until the version occurs.
        BLAZE_TRACE_LOG(Log::REPLICATION, "[RemoteMapCollection].waitForCollectionVersion: Version is " << SbFormats::HexUpper(currentVersion) 
                        << "sleeping until version " << SbFormats::HexUpper(version) << ".");

        //We always want a minimum timeout on this operation.
        TimeValue timeout = (Fiber::getCurrentTimeout() == 0) ? gReplicator->getWaitForMapVersionTimeout() + TimeValue::getTimeOfDay() : 0;
        result = Fiber::wait(ev, "RemoteMapCollection::waitForCollectionVersion.keepWaiting", timeout);

        if (result != ERR_OK)
        {
            //Try removing us from the wake queue
            WaitForVersionMap::iterator begin = mWaitForVersionMap.lower_bound(version);
            if (begin != mWaitForVersionMap.end())
            {
                WaitForVersionMap::iterator itr = begin;
                WaitForVersionMap::iterator end = mWaitForVersionMap.upper_bound(version);
                for (; itr != end; ++itr)
                {
                    if (itr->second == ev)
                    {
                        mWaitForVersionMap.erase(itr);
                        break;
                    }
                }
            }
        }
    }
    else
    {
        //If we already have the desired map version or newer, no need to wait.
        BLAZE_TRACE_LOG(Log::REPLICATION, "[RemoteMapCollection].waitForCollectionVersion: Version is " << SbFormats::HexUpper(currentVersion) 
                        << ", version wanted " << SbFormats::HexUpper(version) << ".");
        result = ERR_OK;
    }

    BLAZE_TRACE_LOG(Log::REPLICATION, "[RemoteMapCollection].waitForCollectionVersion: Awoke after version " << SbFormats::HexUpper(version) 
                    << "; result=" << (ErrorHelp::getErrorName(result)));
    return result;
}

ReplicatedMapVersion RemoteMapCollection::getRemoteVersion(InstanceId instanceId) const
{
    InstanceIdToVersionMap::const_iterator instanceItr = mRemoteVersions.find(instanceId);
    return (instanceItr != mRemoteVersions.end()) ? instanceItr->second : (BuildInstanceKey64(instanceId, getInvalidVersion()));
}

void RemoteMapCollection::setRemoteVersion(ReplicatedMapVersion version)
{
    mRemoteVersions[GetInstanceIdFromInstanceKey64(version)] = version;

    typedef eastl::pair<WaitForVersionMap::iterator, WaitForVersionMap::iterator> WaitForVersionMapIterPair;
    WaitForVersionMapIterPair pair = mWaitForVersionMap.equal_range_small(version);
    //Check the waitfor map to see if someone is waiting on this version
    if (pair.first != pair.second)
    {
        WaitForVersionMap::iterator itr = pair.first;
        WaitForVersionMap::iterator end = pair.second;
        for (; itr != end; ++itr)
        {
            BLAZE_TRACE_LOG(Log::REPLICATION, "[RemoteMapCollection].setRemoteVersion: version " << SbFormats::HexUpper(version) 
                << " reached, waiting on version " << SbFormats::HexUpper(itr->first) << ", signaling event " << itr->second.eventId);
            Fiber::signal(itr->second, ERR_OK);
        }
        mWaitForVersionMap.erase(pair.first, pair.second);
    }
}

void RemoteMapCollection::signalVersionWaiters(InstanceId instanceId, BlazeRpcError rc)
{
    if (EA_LIKELY(mWaitForVersionMap.empty()))
        return; // early out
    typedef eastl::vector<Fiber::EventHandle> EventList;
    EventList events;
    events.reserve(mWaitForVersionMap.size());
    // find all matching waiters in the wait queue
    for (WaitForVersionMap::const_iterator i = mWaitForVersionMap.begin(), e = mWaitForVersionMap.end(); i != e; ++i)
    {
        if (instanceId == GetInstanceIdFromInstanceKey64(i->first))
        {
            events.push_back(i->second);
        }
    }
    if (!events.empty())
    {
        BLAZE_TRACE_LOG(Log::REPLICATION, "[RemoteMapCollection].wakeVersionWaiters: "
            "Wake fibers waiting on instance id " << instanceId << " with result = " << ErrorHelp::getErrorName(rc));
        // signal all events used by matching waiters
        for (EventList::const_iterator i = events.begin(), e = events.end(); i != e; ++i)
        {
            Fiber::signal(*i, rc);
        }
        BLAZE_TRACE_LOG(Log::REPLICATION, "[RemoteMapCollection].wakeVersionWaiters: "
            "Woke " << events.size() << " waiters.");
    }
}

bool RemoteMapCollection::subscribeToObject(const ReplicationObjectSubscribe& update) const
{
    const SlaveSessionPtr session = gController->getConnectionManager().getSlaveSessionByInstanceId(GetInstanceIdFromInstanceKey64(update.getObjectId()));
    if (session != nullptr)
    {
        Notification notification(*ComponentData::getNotificationInfo(mComponentId, Component::REPLICATION_OBJECT_SUBSCRIBE_NOTIFICATION_ID), &update, NotificationSendOpts(true));
        session->sendNotification(notification);
        return true;
    }
    return false;
}

} // Blaze
