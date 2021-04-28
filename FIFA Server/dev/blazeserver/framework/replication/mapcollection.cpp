/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/replication/mapcollection.h"
#include "framework/replication/replicator.h"
#include "framework/replication/replicatedmap.h"
#include "framework/replication/remotereplicatedmap.h"
#include "framework/tdf/replicationtypes_server.h"
#include "framework/system/job.h"
#include "framework/util/random.h"

#include "framework/component/component.h"
#include "framework/component/notification.h"
#include "framework/connection/connectionmanager.h"
#include "framework/connection/inboundrpcconnection.h"
#include "framework/controller/controller.h"

namespace Blaze
{



/*** Defines/Macros/Constants/Typedefs ***********************************************************/


/*** Public Methods ******************************************************************************/

MapCollectionBase::MapCollectionBase(ComponentId componentId, MemoryGroupId memoryId, bool localSubscription)
    : mComponentId(componentId),
    mMemoryId(memoryId),
    mLocalSubscription(localSubscription),
    mLogCategory(BlazeRpcLog::getLogCategory(componentId)),
    mVersion(BuildInstanceKey64(gController->getInstanceId(), INVALID_MAP_VERSION))
{
    incrVersion();
}

uint32_t MapCollectionBase::getInstanceId() const
{
    return gController->getInstanceId();
}

bool MapCollectionBase::isSubscribedForPartialReplication(ReplicatedMapType mapType, SlaveSession* session) const
{
    // only remote instances may be subscribed for partial replication
    if (session != nullptr)
    {
        InstanceId instanceId = GetInstanceIdFromInstanceKey64(session->getId());
        SubscriberByCollectionTypeMap::const_iterator it = mSubscriberByCollectionTypeMap.find(mapType);
        if (it != mSubscriberByCollectionTypeMap.end() && it->second.find(instanceId) != it->second.end())
        {
            return true;
        }
    }
    return false;
}

ReplicatedMapVersion MapCollectionBase::incrVersion()
{
    ReplicatedMapVersion currVersion = GetBaseFromInstanceKey64(mVersion);
    if (EA_UNLIKELY(++currVersion > INSTANCE_KEY_MAX_KEY_BASE_64))
    {
        currVersion = INVALID_MAP_VERSION + 1;
    }

    mVersion = BuildInstanceKey64(gController->getInstanceId(), currVersion);
    return mVersion;
}

ReplicatedMapVersion MapCollectionBase::getNextVersion() const
{
    ReplicatedMapVersion currVersion = GetBaseFromInstanceKey64(mVersion);
    if (EA_UNLIKELY((currVersion + 1 ) > INSTANCE_KEY_MAX_KEY_BASE_64))
    {
        currVersion = INVALID_MAP_VERSION + 1;
    }

    return BuildInstanceKey64(gController->getInstanceId(), currVersion);
}

void MapCollectionBase::addLocalSubscriber()
{
    mLocalSubscription = true;
    sync(nullptr);
}

MapCollection::~MapCollection()
{
    destroyAllLocalMaps();
}

void MapCollection::destroyAllLocalMaps()
{
    ReplicatedMaps::iterator mi = mReplMaps.begin();
    ReplicatedMaps::iterator me = mReplMaps.end();
    for(; mi != me; ++mi)
        delete mi->second;

    mReplMaps.clear();
}

void MapCollectionBase::addSlaveSubscriber(SlaveSession& session, const ReplicationSubscribeRequest& req)
{   
    const InstanceId instanceId = GetInstanceIdFromInstanceKey64(session.getId());

    //No need to add this session twice
    if (mSubscriberSet.find(instanceId) != mSubscriberSet.end())
    {
        BLAZE_WARN_LOG(Log::CONNECTION, "[MapCollectionBase].addSlaveSubscriber: componentId=" << SbFormats::Raw("0x%04x", mComponentId)
        << " sessionId=" << session.getId() << " No need to add this session twice to list=" << &mSubscriberSet);
        return;
    }

    BLAZE_INFO_LOG(Log::CONNECTION, "[MapCollectionBase].addSlaveSubscriber: componentId=" << SbFormats::Raw("0x%04x", mComponentId)
        << " sessionId=" << session.getId() << " to list=" << &mSubscriberSet);

    mSubscriberSet.insert(instanceId);

    for (ReplicationSubscribeRequest::ReplicationMapTypeList::const_iterator i = req.getPartialReplicationTypes().begin(), e = req.getPartialReplicationTypes().end(); i != e; ++i)
    {
        mSubscriberByCollectionTypeMap[*i].insert(instanceId);
    }

    session.addSessionListener(*this);

    sync(&session);
}

void MapCollectionBase::onSlaveSessionRemoved(SlaveSession& session)
{    
    BLAZE_INFO_LOG(mLogCategory, "[MapCollectionBase].removeSubscriber: componentId=" << SbFormats::Raw("0x%04x", mComponentId)
        << " sessionId = " << session.getId() << " from list=" << &mSubscriberSet);
    
    session.removeSessionListener(*this);

    const InstanceId instanceId = GetInstanceIdFromInstanceKey64(session.getId());
    SubscriberByCollectionTypeMap::iterator it = mSubscriberByCollectionTypeMap.begin();
    while (it != mSubscriberByCollectionTypeMap.end())
    {
        it->second.erase(instanceId);
        if (!it->second.empty())
            ++it;
        else
            it = mSubscriberByCollectionTypeMap.erase(it);
    }

    for (SubscriberByCollectionMap::iterator collIt = mSubscriberByCollectionMap.begin(); collIt != mSubscriberByCollectionMap.end();)
    {
        SubscriberByObjectIdMap& objMap = collIt->second;
        for (SubscriberByObjectIdMap::iterator objIt = objMap.begin(); objIt != objMap.end();)
        {
            SubscriberSet& subSet = objIt->second;
            subSet.erase(instanceId);
            if (!subSet.empty())
                ++objIt;
            else
                objIt = objMap.erase(objIt);
        }
        if (!objMap.empty())
            ++collIt;
        else
            collIt = mSubscriberByCollectionMap.erase(collIt);
    }

    mSubscriberSet.erase(instanceId);
}

void MapCollectionBase::updateObjectSubscription(const ReplicationObjectSubscribe& subscribe)
{
    InstanceId instanceId = subscribe.getInstanceId();
    SubscriberByCollectionTypeMap::const_iterator it = mSubscriberByCollectionTypeMap.find(subscribe.getCollectionType());
    if (it != mSubscriberByCollectionTypeMap.end() && 
        it->second.find(instanceId) != it->second.end())
    {
        CollectionId collectionId(subscribe.getComponentId(), subscribe.getCollectionId(), subscribe.getCollectionType());
        if (subscribe.getSubscribe())
        {
            EA::TDF::Tdf* item = findItem(collectionId, subscribe.getObjectId());
            if (item != nullptr)
            {
                bool inserted = mSubscriberByCollectionMap[collectionId][subscribe.getObjectId()].insert(instanceId).second;
                BLAZE_TRACE_REPLICATION_LOG(getLogCategory(),
                    "subscribed update notification" << (inserted ? "" : "(dupe)") << " [component=" << (BlazeRpcComponentDb::getComponentNameById(subscribe.getComponentId())) << "(" 
                    << SbFormats::Raw("0x%04x", subscribe.getComponentId()) << ") collectionId=" << subscribe.getCollectionId() << " objid=" << subscribe.getObjectId() 
                    << " instance=" << subscribe.getInstanceId() << "]");
                // inserted first reference, send onInsert() to the subscriber
                ReplicationUpdate update;
                update.setComponentId(collectionId.getComponentId());
                update.setCollectionId(collectionId.getCollectionNum());
                update.setCollectionType(collectionId.getCollectionType());
                update.setObjectId(subscribe.getObjectId());
                update.setObjectTdf(*item);
                update.setVersion(mVersion);
                update.setIncrementalUpdate(false);
                update.setSubscriptionId(subscribe.getSubscriptionId());
                sendNotificationToSlaves(Component::REPLICATION_OBJECT_INSERT_NOTIFICATION_ID, &update, false, &instanceId, 1);
            }
            else
            {
                BLAZE_TRACE_REPLICATION_LOG(getLogCategory(),
                    "subscribed erase notification [component=" << (BlazeRpcComponentDb::getComponentNameById(subscribe.getComponentId())) << "(" 
                    << SbFormats::Raw("0x%04x", subscribe.getComponentId()) << ") collectionId=" << subscribe.getCollectionId() << " objid=" << subscribe.getObjectId() 
                    << " instance=" << subscribe.getInstanceId() << "]");
                // If an object doesn't exist it means we just missed its destruction and
                // we need to send onErase() to the remote subscriber to have him clean up
                // any outstanding references it accumulated for this object. Note, target
                // must correctly handle double-erase!
                ReplicationDeletionUpdate update;
                update.setComponentId(collectionId.getComponentId());
                update.setCollectionId(collectionId.getCollectionNum());
                update.setCollectionType(collectionId.getCollectionType());
                update.setObjectId(subscribe.getObjectId());
                update.setVersion(mVersion);
                update.setSubscriptionId(subscribe.getSubscriptionId());
                sendNotificationToSlaves(Component::REPLICATION_OBJECT_ERASE_NOTIFICATION_ID, &update, false, &instanceId, 1);
            }
        }
        else
        {
            SubscriberByCollectionMap::iterator collIt = mSubscriberByCollectionMap.find(collectionId);
            if (collIt != mSubscriberByCollectionMap.end())
            {
                SubscriberByObjectIdMap& objMap = collIt->second;
                SubscriberByObjectIdMap::iterator objIt = objMap.find(subscribe.getObjectId());
                if (objIt != objMap.end())
                {
                    SubscriberSet& subSet = objIt->second;
                    SubscriberSet::iterator subIt = subSet.find(instanceId);
                    if (subIt != subSet.end())
                    {
                        subSet.erase(subIt);
                        if (subSet.empty())
                        {
                            objMap.erase(objIt);
                            if (objMap.empty())
                            {
                                mSubscriberByCollectionMap.erase(collIt);
                            }
                        }
                        BLAZE_TRACE_REPLICATION_LOG(getLogCategory(),
                            "subscribed erase notification [component=" << (BlazeRpcComponentDb::getComponentNameById(subscribe.getComponentId())) << "(" 
                            << SbFormats::Raw("0x%04x", subscribe.getComponentId()) << ") collectionId=" << subscribe.getCollectionId() << " objid=" << subscribe.getObjectId() 
                            << " instance=" << subscribe.getInstanceId() << "]");
                        // removed last reference, send onErase() to the subscriber
                        ReplicationDeletionUpdate update;
                        update.setComponentId(collectionId.getComponentId());
                        update.setCollectionId(collectionId.getCollectionNum());
                        update.setCollectionType(collectionId.getCollectionType());
                        update.setObjectId(subscribe.getObjectId());
                        update.setVersion(mVersion);
                        update.setSubscriptionId(subscribe.getSubscriptionId());
                        sendNotificationToSlaves(Component::REPLICATION_OBJECT_ERASE_NOTIFICATION_ID, &update, false, &instanceId, 1);
                    }
                }
            }
        }
    }
    else
    {
        BLAZE_WARN_LOG(mLogCategory, "[MapCollectionBase].updateItemSubscription: "
            << "Server instance: " << instanceId << " did not subscribe for partial replication on collection(" 
            << BlazeRpcComponentDb::getComponentNameById(subscribe.getComponentId()) << ","
            << subscribe.getCollectionId() << ","
            << subscribe.getCollectionType() << ")");
    }
}

void MapCollection::sync(SlaveSession* session)
{
    // Iterate through each map and synchronize it
    for (ReplicatedMaps::iterator i = mReplMaps.begin(), e = mReplMaps.end(); i != e; ++i)
    {
        ReplicatedMapBase *map = i->second;
        ReplicatedMapType mapType = map->getCollectionId().getCollectionType();
        if (isSubscribedForPartialReplication(mapType, session))
        {
            // partially subscribed maps do not get synced
            continue;
        }
        map->synchronizeMap(session);
        ReplicationMapSynchronizationComplete mapCompleteTdf;
        mapCompleteTdf.setComponentId(mComponentId);
        mapCompleteTdf.setCollectionId(map->getCollectionId().getCollectionNum());
        mapCompleteTdf.setCollectionType(map->getCollectionId().getCollectionType());
        mapCompleteTdf.setVersion(map->getVersion());
        if (session != nullptr)
        {
            Notification notification(*ComponentData::getNotificationInfo(mComponentId, Component::REPLICATION_MAP_SYNC_COMPLETE_NOTIFICATION_ID), &mapCompleteTdf, NotificationSendOpts(true));
            session->sendNotification(notification);
        }
        else
        {
            gReplicator->onMapSynchronizationComplete(mapCompleteTdf);
        }
    }

    //Finish off with a "sync finished" message
    ReplicationSynchronizationComplete completeTdf;
    completeTdf.setComponentId(mComponentId);
    completeTdf.setInstanceId(gController->getInstanceId());

    if (session != nullptr)
    {
        Notification notification(*ComponentData::getNotificationInfo(mComponentId, Component::REPLICATION_SYNC_COMPLETE_NOTIFICATION_ID), &completeTdf, NotificationSendOpts(true));
        session->sendNotification(notification);
    }
    else
    {
        gReplicator->onSynchronizationComplete(completeTdf);
    }
}

EA::TDF::Tdf* MapCollection::findItem(const CollectionId& collectionId, ObjectId objectId)
{
    ReplicatedMaps::iterator mapItr = mReplMaps.find(collectionId);
    if (mapItr != mReplMaps.end())
    {
        ReplicatedMapBase* map = mapItr->second;
        return map->findItem(objectId);
    }
    return nullptr;
}

void MapCollection::destroyDynamicMap(const CollectionId& collectionId, const EA::TDF::Tdf *context)
{
    ReplicatedMaps::iterator mapItr = mReplMaps.find(collectionId);
    if (mapItr != mReplMaps.end())
    {
        ReplicatedMapBase* map = mapItr->second;

        if (map->isDynamic())
        {
            // Remove map from the list
            mReplMaps.erase(mapItr);            

            //Kill it
            delete map;
            
            //Let everyone know we killed the map.          
            sendDestroyNotification(collectionId, context);
        }
        else
        {
            //This should never happen.
            BLAZE_ERR_LOG(Log::REPLICATION, "[MapCollection] Tried to delete non-dyanmic collection " << collectionId.getComponentId() 
                          << "," << collectionId.getCollectionNum() << ".");
        }
    }
    else
    {
        // This should never happen.
        BLAZE_ERR_LOG(Log::REPLICATION, "[MapCollection] Tried to delete unknown collection " << collectionId.getComponentId() 
                      << "," << collectionId.getCollectionNum() << ".");
    }
}


void MapCollection::addMap(ReplicatedMapBase& mapToAdd)
{
    // Get/create a collection
    mReplMaps.insert(eastl::make_pair(mapToAdd.getCollectionId(), &mapToAdd));
}


ReplicatedMapBase* MapCollection::getMap(const CollectionId& collectionId)
{
    ReplicatedMapBase* map = nullptr;    
    ReplicatedMaps::iterator it = mReplMaps.find(collectionId);
    if (it != mReplMaps.end())
        map = it->second;
    return map;
}

const ReplicatedMapBase* MapCollection::getMap(const CollectionId& collectionId) const
{
    ReplicatedMapBase* map = nullptr;    
    ReplicatedMaps::const_iterator it = mReplMaps.find(collectionId);
    if (it != mReplMaps.end())
        map = it->second;
    return map;
}

/*! ***************************************************************************/
/*! \brief Determines whether a map exists for the given collection id.
    \return True if the map exists.
*******************************************************************************/
bool MapCollection::mapExists(const CollectionId& collectionId) const
{
    return (mReplMaps.count(collectionId) != 0);
}


/*** Protected Methods ***************************************************************************/

/*** Private Methods *****************************************************************************/


/************************************************************************/
/* Notification senders                                                 */
/************************************************************************/

/*! ***************************************************************************/
/*! \brief Sends a create notification to subscribed sessions.

    This function will take ownership of the context tdf, so pass a clone.

    \param collectionId  Map being created.
*******************************************************************************/
void MapCollectionBase::sendCreateNotification(const CollectionId& collectionId, const EA::TDF::Tdf *context)
{
    // always increment the version before making changes to the collection
    incrVersion();

    //This is safe because for now because we only modify this map right before the send.
    ReplicationMapCreationUpdate update;
    update.setComponentId(collectionId.getComponentId());
    update.setCollectionId(collectionId.getCollectionNum());
    update.setCollectionType(collectionId.getCollectionType());
    update.setVersion(mVersion);

    if (context != nullptr)
        update.setContext(const_cast<EA::TDF::Tdf&>(*context));  //REPLTODO - Get rid of the const_cast

    sendNotificationToSlaves(Component::REPLICATION_MAP_CREATE_NOTIFICATION_ID, &update, false, mSubscriberSet.begin(), mSubscriberSet.size());    

    if (isLocalSubscription())
    {
        Fiber::BlockingSuppressionAutoPtr suppressAutoPtr;
        gReplicator->onDynamicMapCreated(update);
    }
}

/*! ***************************************************************************/
/*! \brief Sends a destroy notification to subscribed sessions.
    
    This function will take ownership of the context tdf, so pass a clone.

    \param collectionId  Map being destroyed.
*******************************************************************************/
void MapCollectionBase::sendDestroyNotification(const CollectionId& collectionId, const EA::TDF::Tdf *context)
{
    // always increment the version before making changes to the collection
    incrVersion();

    ReplicationMapDeletionUpdate update;
    update.setComponentId(collectionId.getComponentId());
    update.setCollectionId(collectionId.getCollectionNum());
    update.setCollectionType(collectionId.getCollectionType());
    update.setVersion(mVersion);

    if (context != nullptr)
        update.setContext(const_cast<EA::TDF::Tdf&>(*context));  //REPLTODO - Get rid of the const_cast

    sendNotificationToSlaves(Component::REPLICATION_MAP_DESTROY_NOTIFICATION_ID, &update, false, mSubscriberSet.begin(), mSubscriberSet.size());   

    if (isLocalSubscription())
    {
        Fiber::BlockingSuppressionAutoPtr suppressAutoPtr;
        gReplicator->onDynamicMapDeleted(update);        
    }
}

/*! ***************************************************************************/
/*! \brief Sends an update notification to all sessions.
    
    This function will take ownership of the object and context tdfs, so pass clones.

    \param collectionId Map this notification refers to.
    \param objectId Id of object being inserted/updated.
    \param item The object being inserted/updated.  This item must have its reference incremented before passing in.
    \param context Optional context field to describe the insertion/deletion.
*******************************************************************************/
void MapCollectionBase::sendUpdateNotification(const CollectionId& collectionId, ObjectId objectId, const EA::TDF::Tdf& item, const EA::TDF::Tdf* context, bool incrementalUpdate)
{
    // always increment the version before making changes to the collection
    incrVersion();

    ReplicationUpdate update;
    update.setComponentId(collectionId.getComponentId());
    update.setCollectionId(collectionId.getCollectionNum());
    update.setCollectionType(collectionId.getCollectionType());
    update.setObjectId(objectId);
    update.setObjectTdf(const_cast<EA::TDF::Tdf&>(item));
    if(context != nullptr)
        update.setContext(const_cast<EA::TDF::Tdf&>(*context));  //REPLTODO - Get rid of the const_cast
    update.setVersion(mVersion);
    update.setIncrementalUpdate(incrementalUpdate);

    InstanceIdList instances;
    getSlavesAffectedByObjectUpdate(collectionId, objectId, instances);
    CommandId notificationType;
    if (incrementalUpdate)
        notificationType = Component::REPLICATION_OBJECT_UPDATE_NOTIFICATION_ID;
    else
        notificationType = Component::REPLICATION_OBJECT_INSERT_NOTIFICATION_ID;
    sendNotificationToSlaves(notificationType, &update, incrementalUpdate, instances.begin(), instances.size());    

    if (isLocalSubscription())
    {
        Fiber::BlockingSuppressionAutoPtr suppressAutoPtr;
        //Now that the item is fixed up, call the local replicator
        gReplicator->onReplicatedItemUpdated(update);
    }
}

/*! ***************************************************************************/
/*! \brief Sends an erasure notification to all sessions.

    This function will take ownership of the object and context tdfs, so pass clones.

    \param collectionId Map this notification refers to.
    \param objectId Id of object being erased
    \param context Optional context field to describe the erasure.
*******************************************************************************/
void MapCollectionBase::sendEraseNotification(const CollectionId& collectionId, ObjectId objectId, const EA::TDF::Tdf* context)
{
    // always increment the version before making changes to the collection
    incrVersion();

    ReplicationDeletionUpdate update;
    update.setComponentId(collectionId.getComponentId());
    update.setCollectionId(collectionId.getCollectionNum());
    update.setCollectionType(collectionId.getCollectionType());
    update.setObjectId(objectId);
    if (context != nullptr)
        update.setContext(const_cast<EA::TDF::Tdf&>(*context));  //REPLTODO - Get rid of the const_cast
    update.setVersion(mVersion);

    InstanceIdList instances;
    getSlavesAffectedByObjectUpdate(collectionId, objectId, instances);
    sendNotificationToSlaves(Component::REPLICATION_OBJECT_ERASE_NOTIFICATION_ID, &update, false, instances.begin(), instances.size());    

    if (isLocalSubscription())
    {
        Fiber::BlockingSuppressionAutoPtr suppressAutoPtr;
        gReplicator->onReplicatedItemDeleted(update);        
    }
}

void MapCollectionBase::sendSyncCreateNotice(SlaveSession* session, const CollectionId& id) const
{
    ReplicationMapCreationUpdate createUpdate;
    createUpdate.setComponentId(id.getComponentId());
    createUpdate.setCollectionId(id.getCollectionNum());
    createUpdate.setCollectionType(id.getCollectionType());
 
    if (session != nullptr)
    {
        Notification notification(*ComponentData::getNotificationInfo(mComponentId, Component::REPLICATION_MAP_CREATE_NOTIFICATION_ID), &createUpdate, NotificationSendOpts(true));
        session->sendNotification(notification);
    }
    else if (isLocalSubscription())
    {
        Fiber::BlockingSuppressionAutoPtr suppressAutoPtr;
        gReplicator->onDynamicMapCreated(createUpdate);        
    }
}

void MapCollectionBase::sendSyncUpdateNotice(SlaveSession* session, const CollectionId &id, ObjectId objectId, EA::TDF::Tdf &tdf, ReplicatedMapVersion version) const
{
    ReplicationUpdate update;
    update.setComponentId(id.getComponentId());
    update.setCollectionId(id.getCollectionNum());
    update.setCollectionType(id.getCollectionType());
    update.setObjectId(objectId);
    update.setVersion(version);
    update.setObjectTdf(tdf);

    if (session != nullptr)
    {
        // TODO_MC: Notification needs to support logCategory, and this category is Log::REPLICATION...  Same as other replication notifications.
        Notification notification(*ComponentData::getNotificationInfo(mComponentId, Component::REPLICATION_OBJECT_INSERT_NOTIFICATION_ID), &update, NotificationSendOpts(true));
        session->sendNotification(notification);
    } 
    else if (isLocalSubscription())
    {
        Fiber::BlockingSuppressionAutoPtr suppressAutoPtr;
        gReplicator->onReplicatedItemUpdated(update);
    }    
}

/*! ***************************************************************************/
/*! \brief Internal function to send a notification to the subscribed slaves.

    \param notificationType  Type of notification to send.
    \param tdf Notification payload.
*******************************************************************************/
void MapCollectionBase::sendNotificationToSlaves(uint16_t notificationType, const EA::TDF::Tdf* tdf, bool onlySendChanges, InstanceId* instances, size_t instanceCount) const
{
    if (instanceCount > 0)
    {
        Notification notification(*ComponentData::getNotificationInfo(mComponentId, notificationType), tdf, NotificationSendOpts(true, 0, 0, true));

        const int32_t offset = (instanceCount > 1) ? Random::getRandomNumber((int32_t)instanceCount) : 0;
        for (int32_t idx = 0; idx < (int32_t)instanceCount; ++idx)
        {
            const int32_t pos = (offset + idx) % (int32_t)instanceCount;
            const InstanceId instanceId = instances[pos];
            const SlaveSessionPtr session = gController->getConnectionManager().getSlaveSessionByInstanceId(instanceId);
            if (session != nullptr)
            {
                session->sendNotification(notification);
            }
        }
    }
}

void MapCollectionBase::getSlavesAffectedByObjectUpdate(const CollectionId& collectionId, ObjectId objectId, InstanceIdList& instances) const
{
    instances.clear();
    instances.reserve(mSubscriberSet.size());
    SubscriberByCollectionTypeMap::const_iterator it = mSubscriberByCollectionTypeMap.find(collectionId.getCollectionType());
    for (SubscriberSet::const_iterator i = mSubscriberSet.begin(), e = mSubscriberSet.end(); i != e; ++i)
    {
        InstanceId instanceId = *i;
        if (it != mSubscriberByCollectionTypeMap.end() && it->second.find(instanceId) != it->second.end())
        {
            // this instance is partially subscribed to this map type, see if we have any subscriptions
            SubscriberByCollectionMap::const_iterator collIt = mSubscriberByCollectionMap.find(collectionId);
            if (collIt == mSubscriberByCollectionMap.end())
                continue;
            const SubscriberByObjectIdMap& objMap = collIt->second;
            SubscriberByObjectIdMap::const_iterator objIt = objMap.find(objectId);
            if (objIt == objMap.end())
                continue;
            const SubscriberSet& subSet = objIt->second;
            SubscriberSet::const_iterator subIt = subSet.find(instanceId);
            if (subIt == subSet.end())
                continue;
        }
        instances.push_back(instanceId);
    }
}

RemoteReplicatedMapBase* SharedCollection::getMap(const CollectionId& collectionId)
{
    return gReplicator->getRemoteMap(collectionId);
}

const RemoteReplicatedMapBase* SharedCollection::getMap(const CollectionId& collectionId) const
{
    return gReplicator->getRemoteMap(collectionId);
}

void SharedCollection::destroyDynamicMap(const CollectionId& collectionId, const EA::TDF::Tdf *context)
{
    EA_ASSERT(collectionId.getCollectionNum() <= INSTANCE_KEY_MAX_KEY_BASE_64);
    CollectionId shardedId(collectionId.getComponentId(),
                           BuildInstanceKey64(gController->getInstanceId(), collectionId.getCollectionNum()),
                           collectionId.getCollectionType());

    RemoteReplicatedMapBase* map = gReplicator->getRemoteMap(shardedId);
    if (map != nullptr)
    {
        if (map->isDynamic())
        {
            //Let everyone know we killed the map.
            sendDestroyNotification(collectionId, context);
        }
        else
        {
            //This should never happen.
            BLAZE_ERR_LOG(Log::REPLICATION, "[SharedCollection] Tried to delete non-dynamic collection componentId: "
                          << collectionId.getComponentId() << ", collectionNum: " << collectionId.getCollectionNum() << ".");
        }
    }
    else
    {
        // This should never happen.
        BLAZE_ERR_LOG(Log::REPLICATION, "[SharedCollection] Tried to delete unknown collection componentId: "
        << collectionId.getComponentId() << ", collectionNum: " << collectionId.getCollectionNum() << ".");
    }
}

void SharedCollection::sync(SlaveSession* session)
{
    RemoteMapCollection* collection = gReplicator->getRemoteCollection(mComponentId);

    for (RemoteMapCollection::RemoteReplicatedMaps::const_iterator mapItr = collection->mReplMaps.begin(); mapItr != collection->mReplMaps.end(); ++mapItr)
    {
        RemoteReplicatedMapBase* map = mapItr->second;
        ReplicatedMapType mapType = map->getCollectionId().getCollectionType();
        if (isSubscribedForPartialReplication(mapType, session))
        {
            // partially subscribed maps do not get synced
            continue;
        }
        if (map->synchronizeMap(session))
        {
            ReplicationMapSynchronizationComplete mapCompleteTdf;
            mapCompleteTdf.setComponentId(mComponentId);
            mapCompleteTdf.setCollectionId(map->getCollectionId().getCollectionNum());
            mapCompleteTdf.setCollectionType(map->getCollectionId().getCollectionType());
            mapCompleteTdf.setVersion(map->getRemoteVersion(gController->getInstanceId()));

            if (session != nullptr)
            {
                Notification notification(*ComponentData::getNotificationInfo(mComponentId, Component::REPLICATION_MAP_SYNC_COMPLETE_NOTIFICATION_ID), &mapCompleteTdf, NotificationSendOpts(true));
                session->sendNotification(notification);
            }
            else
            {
                gReplicator->onMapSynchronizationComplete(mapCompleteTdf);
            }
        }
    }

    //Finish off with a "sync finished" message
    ReplicationSynchronizationComplete completeTdf;
    completeTdf.setComponentId(mComponentId);
    completeTdf.setInstanceId(gController->getInstanceId());

    if (session != nullptr)
    {
        Notification notification(*ComponentData::getNotificationInfo(mComponentId, Component::REPLICATION_SYNC_COMPLETE_NOTIFICATION_ID), &completeTdf, NotificationSendOpts(true));
        session->sendNotification(notification);
    }
    else
    {
        gReplicator->onSynchronizationComplete(completeTdf);
    }
}

EA::TDF::Tdf* SharedCollection::findItem(const CollectionId& collectionId, ObjectId objectId)
{
    RemoteMapCollection* collection = gReplicator->getRemoteCollection(mComponentId);
    RemoteMapCollection::RemoteReplicatedMaps::iterator mapItr = collection->mReplMaps.find(collectionId);
    if (mapItr != collection->mReplMaps.end())
    {
        RemoteReplicatedMapBase* map = mapItr->second;
        return map->findObject(objectId);
    }
    return nullptr;
}

} // Blaze
