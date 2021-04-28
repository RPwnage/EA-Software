/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REPLICATIONUPDATE_H
#define BLAZE_REPLICATIONUPDATE_H

/*** Include files *******************************************************************************/

#include "EASTL/map.h"
#include "EATDF/tdf.h"
#include "EATDF/tdfobjectid.h"
#include "framework/callback.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

typedef uint64_t ObjectId;
typedef uint64_t ReplicatedMapVersion;
typedef uint64_t ReplicatedMapId;
typedef uint16_t ReplicatedMapType;
typedef Functor1<uint16_t> ReplicationSynchronizationCb;

static const ReplicatedMapVersion INVALID_MAP_VERSION = 0;

class RemoteReplicatedMapBase;
typedef eastl::vector<RemoteReplicatedMapBase*> RemoteReplicatedMapList;

class CollectionId
{
public:
    CollectionId() : mComponentId(0), mCollectionType(0), mCollectionId(0) {}
    CollectionId(EA::TDF::ComponentId componentId, ReplicatedMapId collectionId)
        : mComponentId(componentId), mCollectionType(0), mCollectionId(collectionId) {}
    CollectionId(EA::TDF::ComponentId componentId, ReplicatedMapId collectionId, ReplicatedMapType collectionType)
        : mComponentId(componentId), mCollectionType(collectionType), mCollectionId(collectionId) {}
    ~CollectionId() {}

    // Accessors
    void set(EA::TDF::ComponentId componentId, ReplicatedMapId collectionId)
    {
        mComponentId = componentId;
        mCollectionId = collectionId;
    }
    void set(EA::TDF::ComponentId componentId, ReplicatedMapId collectionId, ReplicatedMapType collectionType)
    {
        mComponentId = componentId;
        mCollectionId = collectionId;
        mCollectionType = collectionType;
    }
    void setComponentId(EA::TDF::ComponentId componentId)  { mComponentId = componentId; }
    void setCollectionNum(ReplicatedMapId collectionId) { mCollectionId = collectionId; }
    void setCollectionType(ReplicatedMapType collecitonType) { mCollectionType = collecitonType; }

    EA::TDF::ComponentId getComponentId() const { return mComponentId; }
    ReplicatedMapId getCollectionNum() const { return mCollectionId; }
    ReplicatedMapType getCollectionType() const { return mCollectionType; }

    bool operator<(const CollectionId& rhs) const
    {
        if (mComponentId != rhs.mComponentId)       
            return (mComponentId < rhs.mComponentId);
        if (mCollectionId != rhs.mCollectionId)     
            return (mCollectionId < rhs.mCollectionId);
        if (mCollectionType != rhs.mCollectionType) 
            return (mCollectionType < rhs.mCollectionType);

        return false;
    }

    bool operator==(const CollectionId &rhs) const
    {
        return mComponentId == rhs.mComponentId &&
               mCollectionId == rhs.mCollectionId &&
               mCollectionType == rhs.mCollectionType;
    }

private:
    EA::TDF::ComponentId mComponentId;
    ReplicatedMapType mCollectionType;
    ReplicatedMapId mCollectionId;
};

class CollectionIdRange
{
public:
    CollectionIdRange() : mComponentId(0), mCollectionType(0), mStartId(0), mEndId(0) {}
    CollectionIdRange(uint16_t componentId, ReplicatedMapId startId, ReplicatedMapId endId)
        : mComponentId(componentId), mCollectionType(0), mStartId(startId), mEndId(endId) {}
    CollectionIdRange(uint16_t componentId, ReplicatedMapId startId, ReplicatedMapId endId, ReplicatedMapType collectionType)
        : mComponentId(componentId), mCollectionType(collectionType), mStartId(startId), mEndId(endId) {}
    ~CollectionIdRange() {}

    // Accessors
    void set(EA::TDF::ComponentId componentId, ReplicatedMapId startId, ReplicatedMapId endId)
    {
        mComponentId = componentId;
        mStartId = startId;
        mEndId = endId;
    }

    void set(EA::TDF::ComponentId componentId, ReplicatedMapId startId, ReplicatedMapId endId, ReplicatedMapType collectionType)
    {
        mComponentId = componentId;
        mStartId = startId;
        mEndId = endId;
        mCollectionType = collectionType;
    }

    void setComponentId(EA::TDF::ComponentId componentId)  { mComponentId = componentId; }
    void setRange(ReplicatedMapId startId, ReplicatedMapId endId) { mStartId = startId; mEndId = endId; }
    void setCollectionType(ReplicatedMapType collectionType) { mCollectionType = collectionType; }

    EA::TDF::ComponentId getComponentId() const { return mComponentId; }
    ReplicatedMapId getStartId() const { return mStartId; }
    ReplicatedMapId getEndId() const { return mEndId; }
    ReplicatedMapType getCollectionType() const { return mCollectionType; }

    bool operator<(const CollectionIdRange& rhs) const
    {
        if (mComponentId != rhs.mComponentId)
            return (mComponentId < rhs.mComponentId);
        if (mStartId != rhs.mStartId)
            return (mStartId < rhs.mStartId);
        if (mEndId != rhs.mEndId)
            return (mEndId < rhs.mEndId);
        if (mCollectionType != rhs.mCollectionType)
            return (mCollectionType < rhs.mCollectionType);

        return false;
    }

    bool operator==(const CollectionIdRange& rhs) const
    {
        return mComponentId == rhs.mComponentId &&
                mCollectionType == rhs.mCollectionType &&
                mStartId == rhs.mStartId &&
                mEndId == rhs.mEndId;
    }

    bool isInBounds(const CollectionId &id) const
    { 
        return (id.getComponentId() == mComponentId &&
            mStartId <= (GetBaseFromInstanceKey64(id.getCollectionNum())) &&
            mEndId >= (GetBaseFromInstanceKey64(id.getCollectionNum())) &&
            id.getCollectionType() == mCollectionType);
    }

private:
    EA::TDF::ComponentId mComponentId;
    ReplicatedMapType mCollectionType;
    ReplicatedMapId mStartId;
    ReplicatedMapId mEndId;
};

class CommonReplicatedMapBase
{
public:
    virtual ~CommonReplicatedMapBase() {}

    static bool isValidVersion(ReplicatedMapVersion version) { return (GetBaseFromInstanceKey64(version) != INVALID_MAP_VERSION); }
};

} // Blaze

#endif // BLAZE_REPLICATIONUPDATE_H

