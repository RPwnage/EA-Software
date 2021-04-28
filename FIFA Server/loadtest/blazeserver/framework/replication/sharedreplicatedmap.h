/*! ************************************************************************************************/
/*!
    \file sharedreplicatedmap.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_SHAREDREPLICATEDMAP_H
#define BLAZE_SHAREDREPLICATEDMAP_H

/*** Include files *******************************************************************************/
#include "framework/replication/remotereplicatedmap.h"
#include "framework/replication/mapcollection.h"
#include "framework/controller/controller.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

template <class RemoteReplicatedMapSpec>
class SharedReplicatedMap : public RemoteReplicatedMap<RemoteReplicatedMapSpec>
{
public:
    typedef SharedReplicatedMap<RemoteReplicatedMapSpec> this_type;
    typedef ReplicatedMapFactory<this_type> FactoryType;

    SharedReplicatedMap(CollectionId collectionId, RemoteMapCollection& owner, MemoryGroupId memoryId, bool dynamic, ReplicatedDataMediator& mediator,
        ReplicatedItemPoolCollection& poolCollection, SharedCollection* sharedCollection)
    : RemoteReplicatedMap<RemoteReplicatedMapSpec>(collectionId, owner, memoryId, dynamic, mediator, poolCollection, sharedCollection)
    {
    }

    void insert(ObjectId key, typename RemoteReplicatedMapSpec::BaseTdfClassType& item, const typename RemoteReplicatedMapSpec::ContextClassType* context)
    {
        EA_ASSERT(key <= INSTANCE_KEY_MAX_KEY_BASE_64);
        this->mSharedCollection->sendUpdateNotification(this->getCollectionId(), BuildInstanceKey64(gController->getInstanceId(), key), item, (EA::TDF::Tdf*)context, false);
    }

    void update(ObjectId key, typename RemoteReplicatedMapSpec::BaseTdfClassType& item, const typename RemoteReplicatedMapSpec::ContextClassType* context, bool incrementalUpdate = false)
    {
        EA_ASSERT(key <= INSTANCE_KEY_MAX_KEY_BASE_64);
        this->mSharedCollection->sendUpdateNotification(this->getCollectionId(), BuildInstanceKey64(gController->getInstanceId(), key), item, (EA::TDF::Tdf*)context, incrementalUpdate);
    }


    void erase(ObjectId key, const typename RemoteReplicatedMapSpec::ContextClassType* context)
    {
        EA_ASSERT(key <= INSTANCE_KEY_MAX_KEY_BASE_64);
        this->mSharedCollection->sendEraseNotification(this->getCollectionId(), BuildInstanceKey64(gController->getInstanceId(), key), (EA::TDF::Tdf*)context);
    }

private:
};

} // Blaze

#endif // BLAZE_SHAREDREPLICATEDMAP_H
