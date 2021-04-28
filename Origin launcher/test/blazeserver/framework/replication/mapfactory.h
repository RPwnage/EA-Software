/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REPLICATIONMAPFACTORY_H
#define BLAZE_REPLICATIONMAPFACTORY_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class MapCollection;
class RemoteMapCollection;
class RemoteReplicatedMapBase;
class ReplicatedItemPoolCollection;
class ReplicatedDataMediator;
class SharedCollection;

class ReplicatedMapFactoryBase
{
public:
    ReplicatedMapFactoryBase(const CollectionIdRange &range, MemoryGroupId memoryId, ReplicatedItemPoolCollection& pool, SharedCollection* sharedCollection) 
    : mMemoryId(memoryId), mItemPoolCollection(pool), mSharedCollection(sharedCollection), mRange(range) {}
    virtual ~ReplicatedMapFactoryBase() {}

    RemoteReplicatedMapBase* createMap(const CollectionId& collectionId, RemoteMapCollection &owner)
    {
        RemoteReplicatedMapBase* result = nullptr;
        if (isInBounds(collectionId))
        {
            result = internalCreate(collectionId, owner);
        }

        return result;
    }

    const CollectionIdRange &getRange() const { return mRange; }
    bool isInBounds(const CollectionId &id) const { return mRange.isInBounds(id); }
    
    virtual void addSubscriber(void* subscriber) = 0;
    virtual void removeSubscriber(void* subscriber) = 0;

protected:
    MemoryGroupId mMemoryId;
    ReplicatedItemPoolCollection& mItemPoolCollection;
    SharedCollection* mSharedCollection;
    
private:
    friend class Replicator;
    friend class RemoteMapCollection;

    CollectionIdRange mRange;
    virtual void makeCreateNotification(RemoteReplicatedMapBase& map, const EA::TDF::Tdf* context) = 0;
    virtual void makeDestroyNotification(RemoteReplicatedMapBase& map, const EA::TDF::Tdf* context) = 0;
    
    virtual RemoteReplicatedMapBase* internalCreate(const CollectionId& collectionId, RemoteMapCollection &owner) = 0;

    NON_COPYABLE(ReplicatedMapFactoryBase)
};



template <class ReplicatedMapType>
class ReplicatedMapFactory : public ReplicatedMapFactoryBase
{
public:
    typedef ReplicatedMapFactory<ReplicatedMapType> this_type;
    typedef typename ReplicatedMapType::MapFactorySubscriber Subscriber;

    ReplicatedMapFactory(const CollectionIdRange& range, MemoryGroupId memoryId, ReplicatedDataMediator& mediator, ReplicatedItemPoolCollection& pool, SharedCollection* sharedCollection) :    
        ReplicatedMapFactoryBase(range, memoryId, pool, sharedCollection), mMediator(static_cast<typename ReplicatedMapType::MediatorType&>(mediator)), 
        mSubscriberList(BlazeStlAllocator("SubscriberList", memoryId)) {}

    void addSubscriber(void* subscriber) override;
    void removeSubscriber(void* subscriber) override;

private:
    
    void makeCreateNotification(RemoteReplicatedMapBase& map, const EA::TDF::Tdf* context) override;
    void makeDestroyNotification(RemoteReplicatedMapBase& map, const EA::TDF::Tdf* context) override;

    RemoteReplicatedMapBase* internalCreate(const CollectionId& collectionId, RemoteMapCollection& owner) override
    {
        return BLAZE_NEW_MGID(mMemoryId, "ReplicatedMap") ReplicatedMapType(collectionId, owner, mMemoryId, true, mMediator, mItemPoolCollection, mSharedCollection);
    }

private:

    typedef eastl::list<Subscriber *> SubscriberList;
    typedef typename SubscriberList::iterator SubscriberListIterator;
    
    typename ReplicatedMapType::MediatorType& mMediator;
    SubscriberList mSubscriberList;

};

template <class ReplicatedMapType>
void ReplicatedMapFactory<ReplicatedMapType>::addSubscriber(void* subscriber)
{
    mSubscriberList.push_back((Subscriber *) subscriber);
}

template <class ReplicatedMapType>
void ReplicatedMapFactory<ReplicatedMapType>::removeSubscriber(void* subscriber) 
{
    mSubscriberList.remove((Subscriber *) subscriber);
}

template <class ReplicatedMapType>
void ReplicatedMapFactory<ReplicatedMapType>::makeCreateNotification(RemoteReplicatedMapBase& map, const EA::TDF::Tdf *context)
{
    //Call the mediator first from the replicator thread.
    Subscriber::makeCreateMapCall(mMediator, map, context);

    SubscriberListIterator itr = mSubscriberList.begin();
    SubscriberListIterator end = mSubscriberList.end();
    for (; itr != end; ++itr)
    {
        Subscriber::makeCreateMapCall(**itr, map, context);
    }
}

template <class ReplicatedMapType>
void ReplicatedMapFactory<ReplicatedMapType>::makeDestroyNotification(RemoteReplicatedMapBase& map, const EA::TDF::Tdf* context)
{
    Subscriber::makeDestroyMapCall(mMediator, map, context);

    SubscriberListIterator itr = mSubscriberList.begin();
    SubscriberListIterator end = mSubscriberList.end();
    for (; itr != end; ++itr)
    {
        Subscriber::makeDestroyMapCall(**itr, map, context);
    }
}

} // Blaze

#endif // BLAZE_REPLICATIONMAPFACTORY_H

