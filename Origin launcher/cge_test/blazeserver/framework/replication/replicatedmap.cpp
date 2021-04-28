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
#include "framework/replication/replicatedmap.h"
#include "framework/replication/mapcollection.h"
#include "EATDF/tdf.h"

#include "framework/util/shared/rawbuffer.h"
#include "framework/controller/controller.h"

#include "EASTL/map.h"

namespace Blaze
{

ReplicatedMapBase::ReplicatedMapBase(const CollectionId& collectionId, MapCollection& owner, bool dynamic)
    : mCollectionId(collectionId),
      mOwner(owner),
      mDynamic(dynamic)
{
}

ReplicatedMapVersion ReplicatedMapBase::insert(ObjectId key, EA::TDF::Tdf& item, const EA::TDF::Tdf *context)
{
    // Non-local maps can not be changed
    // we anticipate the new version for logging purposes
    ReplicatedMapVersion version = mOwner.getNextVersion();

    BLAZE_TRACE_REPLICATION_LOG(mOwner.getLogCategory(),
            "master insert [component=" << (BlazeRpcComponentDb::getComponentNameById(mCollectionId.getComponentId())) << "(" 
             << SbFormats::Raw("0x%04x", mCollectionId.getComponentId()) << ") collectionId=" << mCollectionId.getCollectionNum() << " objid=" << key 
             << " version=" << SbFormats::HexUpper(version) << "]\n" << item);

    mOwner.sendUpdateNotification(getCollectionId(), key, item, context, false);
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    item.clearIsSetRecursive();
#endif
    return version;
}


ReplicatedMapVersion ReplicatedMapBase::update(ObjectId key, EA::TDF::Tdf& item, const EA::TDF::Tdf *context)
{
    // we anticipate the new version for logging purposes
    ReplicatedMapVersion version = mOwner.getNextVersion();
    
    BLAZE_TRACE_REPLICATION_LOG(mOwner.getLogCategory(),
            "master update [component=" << (BlazeRpcComponentDb::getComponentNameById(mCollectionId.getComponentId())) << "(" 
            << SbFormats::Raw("0x%04x", mCollectionId.getComponentId()) << ") collectionId=" << mCollectionId.getCollectionNum() << " objid=" << key << " version=" 
            << SbFormats::HexUpper(version) << "]\n" << item << ((context == nullptr) ? "" : "\nContext:\n") << context);


    mOwner.sendUpdateNotification(getCollectionId(), key, item, context, true);
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    item.clearIsSetRecursive();
#endif
    return version;
}

ReplicatedMapVersion ReplicatedMapBase::erase(ObjectId key, const EA::TDF::Tdf *context)
{  
    // we anticipate the new version for logging purposes
    ReplicatedMapVersion version = mOwner.getNextVersion();


    BLAZE_TRACE_REPLICATION_LOG(mOwner.getLogCategory(),
            "master erase [component=" << (BlazeRpcComponentDb::getComponentNameById(mCollectionId.getComponentId())) << "(0x" 
            << SbFormats::Raw("0x%04x", mCollectionId.getComponentId()) << ") collectionId=" << mCollectionId.getCollectionNum() << " objid=" << key << " version=" 
            << SbFormats::HexUpper(version) << "]" << ((context == nullptr) ? "" : "\n")  << context);


    mOwner.sendEraseNotification(getCollectionId(), key, context);               
    return version;
}


} // Blaze
