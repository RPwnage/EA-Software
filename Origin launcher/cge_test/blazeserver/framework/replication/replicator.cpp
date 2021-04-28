/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/replication/replicator.h"
#include "framework/replication/replicatedmap.h"
#include "framework/replication/remotereplicatedmap.h"
#include "framework/replication/mapcollection.h"
#include "framework/replication/remotemapcollection.h"
#include "framework/connection/connection.h"
#include "framework/connection/session.h"
#include "framework/connection/socketchannel.h"
#include "framework/connection/nameresolver.h"
#include "framework/system/job.h"
#include "framework/system/fibermanager.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/tdf/replicationtypes_server.h"
#include "framework/tdf/frameworkconfigtypes_server.h"
#include "framework/controller/controller.h"
#include "framework/component/notification.h"
#include "framework/usersessions/usersession.h"

namespace Blaze
{

Replicator::Replicator(Controller& controller) 
:   mShutdown(false)
{
}

Replicator::~Replicator()
{
    // Delete all maps in one pass
    for (RemoteComponentCollections::iterator i = mRemoteComponentCollections.begin(), e = mRemoteComponentCollections.end(); i != e; ++i)
    {
        RemoteMapCollection* collection = i->second;
        delete collection;
    }
}

void Replicator::registerLocalCollection(MapCollectionBase& collection)
{
    mLocalComponentCollections[collection.getComponentId()] = &collection;
}

/*! ***************************************************************************/
/*! \brief Registers this replicator to handle a remote component's replicated data.

    Creates a remote collection for a component's replicated data

    \param componentId The component to register.
    \param remoteAddress The address of the replicator to connect to. 
                         (Default is nullptr and uses resolver to find the address)
    \param mediatorFactory The factory function used to create the data mediator.
*******************************************************************************/
ReplicatedDataMediator* Replicator::registerRemoteComponent(uint16_t componentId, MemoryGroupId memoryId, DataMediatorFactoryFunc mediatorFactory)
{
    ReplicatedDataMediator *mediator = nullptr;

    RemoteComponentCollections::iterator it = mRemoteComponentCollections.find(componentId);
    if (it == mRemoteComponentCollections.end())
    {   
        mediator = mediatorFactory();
        RemoteMapCollection *collection = BLAZE_NEW_MGID(memoryId, "RemoteMapCollection") RemoteMapCollection(*this, componentId, memoryId, *mediator);
        mRemoteComponentCollections[componentId] = collection;                       

        if (EA_UNLIKELY(mShutdown))
        {
            collection->shutdown();
        }

        mediator->setup();
    }
    else
    {
        mediator = &it->second->getMediator();
    }

    return mediator;
}

void Replicator::enablePreviousSnapshot(ComponentId componentId)
{
    RemoteComponentCollections::iterator it = mRemoteComponentCollections.find(componentId);
    if (it != mRemoteComponentCollections.end())
    {
        it->second->enablePreviousSnapshot();
    }
}

/*! ***************************************************************************/
/*! \brief Signals start of replication and blocks the current fiber until the
            all the replicated data for a given component has been synced.

    \param component The component to wait for.
    \param waitForCompletion if true, wait for completion of replication sync
*******************************************************************************/
BlazeRpcError Replicator::synchronizeRemoteCollection(Component& component, bool waitForCompletion /* = true */, InstanceId selectedInstanceId /*= INVALID_INSTANCE_ID*/)
{
    BlazeRpcError rc = ERR_OK;
    BLAZE_TRACE_LOG(Log::REPLICATION, "[Replicator].synchronizeRemoteCollection: "
        "component=" << component.getName());
    RemoteMapCollection* remoteMap = getRemoteCollection(component.getId());
    if (remoteMap != nullptr)
    {
        rc = remoteMap->startSynchronization(component, selectedInstanceId);
        if (rc != ERR_OK)
        {
            BLAZE_WARN_LOG(Log::REPLICATION, "[Replicator].synchronizeRemoteCollection: "
                "Failure starting replication for component(" << component.getName() << ").");
            return rc;
        }

        if(waitForCompletion)
        {
            rc = waitForSyncCompletion(component.getId());
            if (rc != ERR_OK)
            {
                BLAZE_WARN_LOG(Log::REPLICATION, "[Replicator].synchronizeRemoteCollection: "
                    "Failure synchronizing replication for component(" << component.getName() << ").");
            }
        }
    }
    else
    {
        BLAZE_TRACE_LOG(Log::REPLICATION, "[Replicator].synchronizeRemoteCollection: "
            "Remote replicated map for component(" << component.getName() << ") not found, skipping sync.");
    }
    return rc;
}

BlazeRpcError Replicator::desynchronizeRemoteCollection(Component& component, InstanceId instanceId)
{
    BlazeRpcError result = component.replicationUnsubscribe(instanceId);
    if (result == ERR_OK)
    {
        result = cleanupRemoteCollection(component, instanceId);
    }

    return result;
}

/*! ***************************************************************************/
/*! \brief Blocks the current fiber until the all the replicated data for 
            a given component has been synced.

    \param component The component to wait for.
*******************************************************************************/
BlazeRpcError Replicator::waitForSyncCompletion(ComponentId component)
{
    BLAZE_TRACE_LOG(Log::REPLICATION, "[Replicator].waitForSyncCompletion: component=" << SbFormats::Raw("0x%04x", component));   

    BlazeRpcError result = ERR_SYSTEM;
    RemoteMapCollection* collection = getRemoteCollection(component);
    if (collection != nullptr)
    {
        result = collection->waitForSyncCompletion();
    }
    else if (!mShutdown)
    {
        BLAZE_ERR_LOG(Log::REPLICATION, "[Replicator].waitForSyncCompletion: no collection registered for component " << component);
    }

    return result;
}

/*! ***************************************************************************/
/*! \brief Blocks the current fiber until the specified map is available

    \param collectionId The collection id to wait for.
    \param map          Pointer to specified map if no error 
    \return             Error code
*******************************************************************************/
BlazeRpcError Replicator::waitForMapCreation(const CollectionId &collectionId, RemoteReplicatedMapBase* &map)
{
    BLAZE_TRACE_LOG(Log::REPLICATION, "[Replicator].waitForMapCreation: component=" << SbFormats::Raw("0x%04x", collectionId.getComponentId()) );   

    if (EA_UNLIKELY(mShutdown))
        return ERR_SYSTEM;

    BlazeRpcError result;
  
    // is the map available already?
    map = getRemoteMap(collectionId);
    if (EA_LIKELY(map != nullptr))
        return ERR_OK;

    Fiber::EventHandle ev = Fiber::getNextEventHandle();
    mWaitForCreationVector.push_back(eastl::make_pair(collectionId, ev));

    //Go to sleep until the map is created
    BLAZE_TRACE_LOG(Log::REPLICATION, "[Replicator].waitForMapCreation: Fiber " << Fiber::getCurrentContext() << " is sleeping until collection " 
                    << collectionId.getCollectionNum() << " is created for component " << SbFormats::Raw("0x%04x", collectionId.getComponentId()) << ".");

    result = Fiber::wait(ev, "Replicator::waitForMapCreation");

    WaitForCreationVector::iterator it = mWaitForCreationVector.begin();
    
    for (; it != mWaitForCreationVector.end(); it++)
    {
        if (it->first == collectionId && it->second == ev)
        {
            mWaitForCreationVector.erase(it);
            break;
        }
    }

    if (result == ERR_OK)
        map = getRemoteMap(collectionId);

    return result;
}

/*! ***************************************************************************/
/*! \brief Performs a blocking cleanup of all the data owned by the instance.

    \param component The component to clean up the local data for.
*******************************************************************************/
BlazeRpcError Replicator::cleanupRemoteCollection(Component& component, InstanceId instanceId)
{
    BlazeRpcError rc = ERR_OK;
    // don't trigger cleanup of the owning instance is actually us
    // since Controller::doConnectAndActivateInstance() will shutdown
    // a remoteInstance after discovering that it is just ourself   
    if (instanceId == gController->getInstanceId())
    {
        return rc;
    }

    BLAZE_TRACE_LOG(Log::REPLICATION, "[Replicator].cleanupRemoteCollection: "
        "component=" << component.getName() << " instanceId=" << instanceId);
    RemoteMapCollection* remoteMap = getRemoteCollection(component.getId());
    if (remoteMap != nullptr)
    {
        BLAZE_INFO_LOG(Log::REPLICATION, "[Replicator].cleanupRemoteCollection: "
            "Started remote component '" << component.getName() << "' replicated data cleanup.");
        TimeValue startTime = TimeValue::getTimeOfDay();
        rc = remoteMap->cleanupRemoteInstanceData(instanceId);
        if (rc == ERR_OK)
        {
            TimeValue totalTime = TimeValue::getTimeOfDay() - startTime;
            BLAZE_INFO_LOG(Log::REPLICATION, "[Replicator].cleanupRemoteCollection: "
                "Completed remote component '" << component.getName() << 
                "' replicated data cleanup in " << totalTime.getMillis() << "ms.");
        }
        else
        {
            BLAZE_WARN_LOG(Log::REPLICATION, "[Replicator].cleanupRemoteCollection: "
                "Failed to complete remote component '" << component.getName() 
                << "' replicated data cleanup, err: " << ErrorHelp::getErrorName(rc));
        }
    }
    else
    {
        BLAZE_TRACE_LOG(Log::REPLICATION, "[Replicator].cleanupRemoteCollection: "
            "Remote replicated map for component(" << component.getName() << ") not found, skipping cleanup.");
    }

    return rc;
}

void Replicator::shutdown()
{
    if (mShutdown)
        return;
        
    mShutdown = true;

    // Go through all the maps and cancel their synchronizations if they aren't synced
    for(RemoteComponentCollections::iterator i = mRemoteComponentCollections.begin(), e = mRemoteComponentCollections.end(); i != e; ++i)
    {
        RemoteMapCollection* collection = i->second;
        collection->shutdown();
    }
}

void Replicator::destroy()
{
    // Destroy the local collections held by the various components:
    for (LocalComponentCollections::iterator i = mLocalComponentCollections.begin(), e = mLocalComponentCollections.end(); i != e; ++i)
    {
        MapCollectionBase* collection = i->second;
        collection->destroyAllLocalMaps();
    }
    mLocalComponentCollections.clear();

    // Destroy all maps in one pass
    for (RemoteComponentCollections::iterator i = mRemoteComponentCollections.begin(), e = mRemoteComponentCollections.end(); i != e; ++i)
    {
        RemoteMapCollection* collection = i->second;
        collection->destroy();
    }
}


/*! ***************************************************************************/
/*! \brief Get an existing replicated map.

    \param collectionId The id of the map.
    \return  The map or nullptr if it does not exist.
*******************************************************************************/
RemoteReplicatedMapBase* Replicator::getRemoteMap(const CollectionId& collectionId)
{
    RemoteMapCollection* collection = getRemoteCollection(collectionId.getComponentId());
    return (collection != nullptr) ? collection->getMap(collectionId) : nullptr;
}

void Replicator::getRemoteMaps(const CollectionIdRange &range, RemoteReplicatedMapList &list)
{
    RemoteMapCollection* collection = getRemoteCollection(range.getComponentId());
    collection->getMaps(range, list);
}


/************************************************************************/
/* Collection management functions                                      */
/************************************************************************/

/*! ***************************************************************************/
/*! \brief Gets a map collection.

    This function is safe around accessing the map.  We don't need to lock after
    that because collections are safe internally and the poitner cannot be 
    altered after it has been created.

    \param componentId Id of the collection to return.
    \return  The collection or nullptr if it does not exist.
*******************************************************************************/
RemoteMapCollection* Replicator::getRemoteCollection(ComponentId componentId)
{
    RemoteMapCollection* collection = nullptr;
    RemoteComponentCollections::iterator it = mRemoteComponentCollections.find(componentId);
    if (it != mRemoteComponentCollections.end())
    {
        collection = it->second;
    }
    
    return collection;
}

/************************************************************************/
/* Notification handlers                                                */
/************************************************************************/


bool Replicator::handleReplicationNotification(Notification& notification)
{
    bool handled = true;
    BlazeRpcError err = Blaze::ERR_OK;

    switch (notification.getNotificationId())
    {
        case Component::REPLICATION_MAP_CREATE_NOTIFICATION_ID:
        {
            Blaze::ReplicationMapCreationUpdatePtr replPayload;
            err = notification.extractPayloadTdf(replPayload);
            if (err == Blaze::ERR_OK)
            {
                onDynamicMapCreated(*replPayload);
            }
            else
            {
                BLAZE_ERR_LOG(Log::REPLICATION, "[Replicator].handleReplicationNotification: failed to decode REPLICATION_MAP_CREATE_NOTIFICATION "
                    "for component=" << BlazeRpcComponentDb::getComponentNameById(notification.getComponentId()) << "(" << SbFormats::Raw("0x%04x", notification.getComponentId())    
                    << ")  msgNum=" << notification.getOpts().msgNum);
            }
            break;
        }
        case Component::REPLICATION_MAP_DESTROY_NOTIFICATION_ID:
        {
            Blaze::ReplicationMapDeletionUpdatePtr replPayload;
            err = notification.extractPayloadTdf(replPayload);
            if (err == Blaze::ERR_OK)
            {
                onDynamicMapDeleted(*replPayload);
            }
            else
            {
                BLAZE_ERR_LOG(Log::REPLICATION, "[Replicator].handleReplicationNotification: failed to decode REPLICATION_MAP_DESTROY_NOTIFICATION "
                    "for component=" << BlazeRpcComponentDb::getComponentNameById(notification.getComponentId()) << "(" << SbFormats::Raw("0x%04x", notification.getComponentId())    
                    << ")  msgNum=" << notification.getOpts().msgNum);
            }
            break;
        }
        case Component::REPLICATION_OBJECT_UPDATE_NOTIFICATION_ID:
        {
            Blaze::ReplicationUpdatePtr replPayload;
            err = notification.extractPayloadTdf(replPayload, true);
            if (err == Blaze::ERR_OK)
            {
                onReplicatedItemUpdated(*replPayload);
            }
            else
            {
                BLAZE_ERR_LOG(Log::REPLICATION, "[Replicator].handleReplicationNotification: failed to decode REPLICATION_OBJECT_UPDATE_NOTIFICATION "
                    "for component=" << BlazeRpcComponentDb::getComponentNameById(notification.getComponentId()) << "(" << SbFormats::Raw("0x%04x", notification.getComponentId())    
                    << ")  msgNum=" << notification.getOpts().msgNum);
            }
            break;
        }
        case Component::REPLICATION_OBJECT_INSERT_NOTIFICATION_ID:
        {
            Blaze::ReplicationUpdatePtr replPayload;
            err = notification.extractPayloadTdf(replPayload);
            if (err == Blaze::ERR_OK)
            {
                onReplicatedItemUpdated(*replPayload);
            }
            else
            {
                BLAZE_ERR_LOG(Log::REPLICATION, "[Replicator].handleReplicationNotification: failed to decode REPLICATION_OBJECT_INSERT_NOTIFICATION "
                    "for component=" << BlazeRpcComponentDb::getComponentNameById(notification.getComponentId()) << "(" << SbFormats::Raw("0x%04x", notification.getComponentId())    
                    << ")  msgNum=" << notification.getOpts().msgNum);
            }
            break;
        }
        case Component::REPLICATION_OBJECT_ERASE_NOTIFICATION_ID:
        {
            Blaze::ReplicationDeletionUpdatePtr replPayload;
            err = notification.extractPayloadTdf(replPayload);
            if (err == Blaze::ERR_OK)
            {
                onReplicatedItemDeleted(*replPayload);
            }
            else
            {
                BLAZE_ERR_LOG(Log::REPLICATION, "[Replicator].handleReplicationNotification: failed to decode REPLICATION_OBJECT_ERASE_NOTIFICATION "
                    "for component=" << BlazeRpcComponentDb::getComponentNameById(notification.getComponentId()) << "(" << SbFormats::Raw("0x%04x", notification.getComponentId())    
                    << ")  msgNum=" << notification.getOpts().msgNum);
            }
            break;
        }
        case Component::REPLICATION_OBJECT_SUBSCRIBE_NOTIFICATION_ID:
        {
            Fiber::setCurrentContext("ReplicationObjectSubscribeNotification");
            Blaze::ReplicationObjectSubscribePtr replPayload;
            err = notification.extractPayloadTdf(replPayload);
            if (err == Blaze::ERR_OK)
            {
                onReplicatedItemSubscribed(*replPayload);
            }
            else
            {
                BLAZE_ERR_LOG(Log::REPLICATION, "[Replicator].handleReplicationNotification: failed to decode REPLICATION_OBJECT_SUBSCRIBE_NOTIFICATION "
                    "for component=" << BlazeRpcComponentDb::getComponentNameById(notification.getComponentId()) << "(" << SbFormats::Raw("0x%04x", notification.getComponentId())    
                    << ")  msgNum=" << notification.getOpts().msgNum);
            }
            break;
        }
        case Component::REPLICATION_SYNC_COMPLETE_NOTIFICATION_ID:
        {
            Fiber::setCurrentContext("ReplicationSyncCompleteNotification");
            Blaze::ReplicationSynchronizationCompletePtr replPayload;
            err = notification.extractPayloadTdf(replPayload);
            if (err == Blaze::ERR_OK)
            {
                onSynchronizationComplete(*replPayload);
            }
            else
            {
                BLAZE_ERR_LOG(Log::REPLICATION, "[Replicator].handleReplicationNotification: failed to decode REPLICATION_SYNC_COMPLETE_NOTIFICATION "
                    "for component=" << BlazeRpcComponentDb::getComponentNameById(notification.getComponentId()) << "(" << SbFormats::Raw("0x%04x", notification.getComponentId())    
                    << ")  msgNum=" << notification.getOpts().msgNum);
            }
            break;
        }
        case Component::REPLICATION_MAP_SYNC_COMPLETE_NOTIFICATION_ID:
        {
            Fiber::setCurrentContext("ReplicationMapSyncCompleteNotification");
            Blaze::ReplicationMapSynchronizationCompletePtr replPayload;
            err = notification.extractPayloadTdf(replPayload);
            if (err == Blaze::ERR_OK)
            {
                onMapSynchronizationComplete(*replPayload);
            }
            else
            {
                BLAZE_ERR_LOG(Log::REPLICATION, "[Replicator].handleReplicationNotification: failed to decode REPLICATION_MAP_SYNC_COMPLETE_NOTIFICATION "
                    "for component=" << BlazeRpcComponentDb::getComponentNameById(notification.getComponentId()) << "(" << SbFormats::Raw("0x%04x", notification.getComponentId())    
                    << ")  msgNum=" << notification.getOpts().msgNum);
            }
            break;
        }
    default:
        handled = false;
        break;
    }

    return handled;
}

/*! ***************************************************************************/
/*! \brief Notification for when a dynamic map is created.

    \param update Notification parameters.
*******************************************************************************/
void Replicator::onDynamicMapCreated(const ReplicationMapCreationUpdate& update)
{
    BLAZE_TRACE_LOG(Log::REPLICATION, "[Replicator].processMapCreateNotification: component=" << SbFormats::Raw("0x%04x", update.getComponentId()) 
                    << " collection=" << update.getCollectionId() << " collectionType=" << update.getCollectionType());

    RemoteMapCollection* collection = getRemoteCollection(update.getComponentId());
    if (collection != nullptr)
    {
        BLAZE_TRACE_REPLICATION_LOG(collection->getLogCategory(),
                "createDynamicMap notification [component=" << (BlazeRpcComponentDb::getComponentNameById(update.getComponentId())) << "(" 
                << SbFormats::Raw("0x%04x", update.getComponentId()) << ") collectionId=" << update.getCollectionId() << " collectionType=" << update.getCollectionType() 
                << "]" << ((update.getContext() == nullptr) ? "" : "\n")  << update.getContext());

        const CollectionId collectionId(update.getComponentId(), update.getCollectionId(), update.getCollectionType());

        collection->createRemoteDynamicMap(collectionId, update.getContext());
        // update the collection version to reflect the new dynamic map
        collection->setRemoteVersion(update.getVersion());

        // signal all fibers waiting on creation of this dynamic map
        WaitForCreationVector::const_iterator it = mWaitForCreationVector.begin();
        for (; it != mWaitForCreationVector.end(); it++)
        {
            if (it->first == collectionId)
                Fiber::delaySignal(it->second, ERR_OK);
        } 
    }
}

/*! ***************************************************************************/
/*! \brief Notification for when a dynamic map is destroyed

\param update Notification parameters.
*******************************************************************************/
void Replicator::onDynamicMapDeleted(const ReplicationMapDeletionUpdate& update)
{
    BLAZE_TRACE_LOG(Log::REPLICATION, "[Replicator].onDynamicMapDeleted: component=" << SbFormats::Raw("0x%04x", update.getComponentId())
                    << " collection=" << update.getCollectionId() << " collectionType=" << update.getCollectionType());

    RemoteMapCollection* collection = getRemoteCollection(update.getComponentId()); 
    if (collection != nullptr)
    {
        BLAZE_TRACE_REPLICATION_LOG(collection->getLogCategory(),
                "destroyDynamicMap notification [component=" << (BlazeRpcComponentDb::getComponentNameById(update.getComponentId())) << "(" 
                << SbFormats::Raw("0x%04x", update.getComponentId()) << ") collectionId=" << update.getCollectionId() << " collectionType=" << update.getCollectionType() 
                << "]" << ((update.getContext() == nullptr) ? "" : "\n") << update.getContext());

        collection->destroyRemoteDynamicMap(CollectionId(update.getComponentId(), update.getCollectionId(), update.getCollectionType()), 
            update.getContext());
        // update the map version to reflect the destructed dynamic map
        collection->setRemoteVersion(update.getVersion());
    }
}

/*! ***************************************************************************/
/*! \brief Notification for when a replicated item is created or updated.

    \param update Notification parameters.
*******************************************************************************/
void Replicator::onReplicatedItemUpdated(ReplicationUpdate& update)
{
    BLAZE_TRACE_LOG(Log::REPLICATION, "[Replicator].onReplicatedItemUpdated: component=" << SbFormats::Raw("0x%04x", update.getComponentId()) << " collection=" 
                    << update.getCollectionId() << " collectionType=" << update.getCollectionType() << " objid=" << update.getObjectId());


    CollectionId collectionId(update.getComponentId(), update.getCollectionId(), update.getCollectionType());
    RemoteMapCollection* collection = getRemoteCollection(collectionId.getComponentId());   

    if (collection != nullptr)
    {
        RemoteReplicatedMapBase* map = collection->getMap(collectionId);
        if (map != nullptr)
        {
            if (!map->remoteUpdate(*collection, update, Fiber::isCurrentlyBlockable()))
                return;
            map->signalSubscribeWaiters(update.getObjectId(), update.getSubscriptionId(), ERR_OK);
        }
        else if (!mShutdown)
        {
            BLAZE_WARN_LOG(Log::REPLICATION, "[Replicator].onReplicatedItemUpdated: No replicated map found for component=" 
                         << SbFormats::Raw("0x%04x", update.getComponentId()) << " collection=" << update.getCollectionId() << " collectionType=" << update.getCollectionType() 
                         << " objid=" << update.getObjectId());
        }
    }
    else if (!mShutdown)
    {
        BLAZE_WARN_LOG(Log::REPLICATION, "[Replicator].onReplicatedItemUpdated: No collection found for component=" 
                     << SbFormats::Raw("0x%04x", update.getComponentId()) << " collection=" << update.getCollectionId() << " collecitonType=" << update.getCollectionType() 
                     << " objid=" << update.getObjectId());
    }
}

/*! ***************************************************************************/
/*! \brief Notification for when a replicated item is deleted.

    \param update Notification parameters.
*******************************************************************************/
void Replicator::onReplicatedItemDeleted(const ReplicationDeletionUpdate& update)
{
    BLAZE_TRACE_LOG(Log::REPLICATION, "[Replicator].onReplicatedItemDeleted: component=" << SbFormats::Raw("0x%04x", update.getComponentId()) << " collection=" 
                    << update.getCollectionId() << " collectionType=" << update.getCollectionType() << " objid=" << update.getObjectId());

    CollectionId collectionId(update.getComponentId(), update.getCollectionId(), update.getCollectionType());
    RemoteMapCollection* collection = getRemoteCollection(collectionId.getComponentId());

    if (collection != nullptr)
    {
        RemoteReplicatedMapBase* map = collection->getMap(collectionId);
        if (map != nullptr)
        {
            BLAZE_TRACE_REPLICATION_LOG(collection->getLogCategory(),
                    "erase notification [component=" << (BlazeRpcComponentDb::getComponentNameById(update.getComponentId())) << "(" 
                    << SbFormats::Raw("0x%04x", update.getComponentId()) << ") collectionId=" << update.getCollectionId() << " objid=" << update.getObjectId() 
                    << " version=" << update.getVersion() << "]");

            Fiber::setCurrentContext(map->getEraseContextStr());
            map->remoteErase(update.getObjectId(), update.getVersion(), update.getContext(), update.getSubscriptionId());
            // NOTE: ERR_DISCONNECTED has a special meaning here, we use it to signal events that are waiting on a replication subscription for an object that has already been erased.
            map->signalSubscribeWaiters(update.getObjectId(), update.getSubscriptionId(), ERR_DISCONNECTED);
        }
        else if (!mShutdown)
        {
            BLAZE_WARN_LOG(Log::REPLICATION, "[Replicator].onReplicatedItemDeleted: No replicated map found for component=" 
                          << SbFormats::Raw("0x%04x", update.getComponentId()) << " collection=" << update.getCollectionId() << " objid=" << update.getObjectId());
        }
    }
    else if (!mShutdown)
    {
        BLAZE_WARN_LOG(Log::REPLICATION, "[Replicator].onReplicatedItemDeleted: No collection found for component=" << SbFormats::Raw("0x%04x", update.getComponentId()) 
                      << " collection=" << update.getCollectionId() << " objid=" << update.getObjectId());
    }
}

void Replicator::onReplicatedItemSubscribed(const ReplicationObjectSubscribe& update)
{
    BLAZE_TRACE_LOG(Log::REPLICATION, "[Replicator].onReplicatedItemSubscribed: component=" << SbFormats::Raw("0x%04x", update.getComponentId()) << " collection=" 
        << update.getCollectionId() << " collectionType=" << update.getCollectionType() << " objid=" << update.getObjectId());

    LocalComponentCollections::iterator it = mLocalComponentCollections.find(update.getComponentId());
    if (it != mLocalComponentCollections.end())
    {
        MapCollectionBase* collection = it->second;
        collection->updateObjectSubscription(update);
    }
    else if (!mShutdown)
    {
        BLAZE_WARN_LOG(Log::REPLICATION, "[Replicator].onReplicatedItemSubscribed: No collection found for component=" << SbFormats::Raw("0x%04x", update.getComponentId()) 
            << " collection=" << update.getCollectionId() << " objid=" << update.getObjectId());
    }
}

/*! ***************************************************************************/
/*! \brief Notification for when a map has finished synchronizing.

\param data Notification parameters
*******************************************************************************/
void Replicator::onMapSynchronizationComplete(const Blaze::ReplicationMapSynchronizationComplete& data)
{   
    BLAZE_TRACE_LOG(Log::REPLICATION, "[Replicator].onMapSynchronizationComplete: component=" << SbFormats::Raw("0x%04x", data.getComponentId()) << " collection=" 
                    << data.getCollectionId() << " version=" << data.getVersion());


    //The subscription is complete - find out which collection and mark it accordingly.
    CollectionId collectionId(data.getComponentId(), data.getCollectionId());
    RemoteMapCollection* collection = getRemoteCollection(collectionId.getComponentId());

    if (collection != nullptr)
    {
        RemoteReplicatedMapBase* map = collection->getMap(collectionId);
        if (map != nullptr)
        {
            map->setRemoteVersion(data.getVersion());
        }        
    }
    else if (!mShutdown)
    {
        BLAZE_WARN_LOG(Log::REPLICATION, "[Replicator].onMapSynchronizationComplete: invalid component id " << SbFormats::Raw("0x%04x", data.getComponentId()) 
                       << " for map sync complete notice");
    }
}


/*! ***************************************************************************/
/*! \brief Notification for when a map has finished synchronizing.

    \param data Notification parameters
*******************************************************************************/
void Replicator::onSynchronizationComplete(const Blaze::ReplicationSynchronizationComplete& data)
{   
    BLAZE_TRACE_LOG(Log::REPLICATION, "[Replicator].onSynchronizationComplete: component=" << SbFormats::Raw("0x%04x", data.getComponentId()));

    //The subscription is complete - find out which collection and mark it accordingly.
    RemoteMapCollection *collection = nullptr;
    if ((collection = getRemoteCollection(data.getComponentId())) != nullptr)
    {
        collection->handleSynchronizationComplete(ERR_OK, data.getInstanceId());
    }
    else if (!mShutdown)
    {
        BLAZE_WARN_LOG(Log::REPLICATION, "[Replicator].onSynchronizationComplete: invalid component id 0x" << SbFormats::Raw("0x%04x", data.getComponentId()) 
                       << " for sync complete notice");
    }
}

TimeValue Replicator::getWaitForMapVersionTimeout() const
{
    return gController->getFrameworkConfigTdf().getReplicationConfig().getWaitForMapVersionTimeout();
}

TimeValue Replicator::getSyncThrottleInterval() const
{
    return gController->getFrameworkConfigTdf().getReplicationConfig().getSyncThrottleInterval();
}

uint32_t Replicator::getSyncThrottleItemCount() const
{
    return gController->getFrameworkConfigTdf().getReplicationConfig().getSyncThrottleItemCount();
}

bool Replicator::isShuttingDown() const
{
    return gController->isShuttingDown();
}


} // namespace Blaze

