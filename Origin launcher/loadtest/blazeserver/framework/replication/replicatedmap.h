/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REPLICATEDMAP_H
#define BLAZE_REPLICATEDMAP_H

/*** Include files *******************************************************************************/
#include "framework/replication/replication.h"
#include "framework/replication/replicationcallback.h"
#include "framework/replication/replicateditem.h"
#include "framework/replication/mapcollection.h"
#include "framework/component/blazerpc.h"
#include "EASTL/hash_map.h"
#include "EASTL/type_traits.h"
#include "EASTL/algorithm.h"
#include "EATDF/tdfobject.h"
/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class Fiber;
class SlaveSession;

class ReplicatedMapBase : public CommonReplicatedMapBase
{
    NON_COPYABLE(ReplicatedMapBase);
public:        
    ~ReplicatedMapBase() override {}

    // returns the owner's version because we now track the version at the collection level.
    ReplicatedMapVersion getVersion() const { return mOwner.getVersion(); }
    const CollectionId& getCollectionId() const { return mCollectionId; }
    MapCollection& getOwner() { return mOwner; }

    /*! ***************************************************************************/
    /*! \brief Returns whether this map is a dynamic or static map.

    \return True if this is a dynamic map, false if static.
    *******************************************************************************/
    bool isDynamic() const { return mDynamic; }

protected:
    friend class Replicator;
    friend class MapCollection;

    ReplicatedMapBase(const CollectionId& collectionId, MapCollection& owner, bool dynamic);

    ReplicatedMapVersion insert(ObjectId key, EA::TDF::Tdf& item, const EA::TDF::Tdf* context);
    ReplicatedMapVersion update(ObjectId key, EA::TDF::Tdf& item, const EA::TDF::Tdf* context);
    ReplicatedMapVersion erase(ObjectId key, const EA::TDF::Tdf* context);
    
private:
    virtual void synchronizeMap(SlaveSession* session) = 0;
    virtual const EA::TDF::Tdf* findItem(ObjectId key) const = 0;
    virtual EA::TDF::Tdf* findItem(ObjectId key) = 0;

private:
    CollectionId mCollectionId;
    MapCollection& mOwner;
    bool mDynamic;

};


template <class ItemType, class BaseTdfType, class ContextType>
class ReplicatedMapTemplateBase : public ReplicatedMapBase
{   
public:
    typedef eastl::hash_map<ObjectId, EA::TDF::tdf_ptr<ItemType> > InnerMapType;
    
    ~ReplicatedMapTemplateBase() override;

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
        { return base_type::mpNode->mValue.second; }

        ItemPointer operator->() const
        { return base_type::mpNode->mValue.second; }

        /*! ***************************************************************************/
        /*! \brief Returns the object id of the value the iterator is pointing to.
        *******************************************************************************/
        ObjectId getObjectId() const
        { return base_type::mpNode->mValue.first; }
    };

    typedef inner_iterator<false> iterator;
    typedef inner_iterator<true> const_iterator;


    size_t size() const { return mMap.size(); }
    bool empty() const { return mMap.empty(); }

    iterator begin() { return mMap.begin(); }
    const_iterator begin() const { return mMap.begin(); }
    
    iterator end() { return mMap.end(); }
    const_iterator end() const { return mMap.end(); }

    bool exists(ObjectId key) { return (mMap.count(key) != 0); }

    const ItemType* findItem(ObjectId key) const override;
    ItemType* findItem(ObjectId key) override;

    bool cloneItem(ObjectId key, BaseTdfType &copy);

    const InnerMapType& getInnerMap() const { return mMap; }

protected:
    friend class Replicator;
    friend class MapCollection;

    ReplicatedMapTemplateBase(CollectionId collectionId, MemoryGroupId memoryId, MapCollection& owner, bool dynamic) : 
            ReplicatedMapBase(collectionId, owner, dynamic), mMap(BlazeStlAllocator("ReplicatedMapMap", memoryId)) {}

private:
    //Private virtuals from ReplicateMapBase
    void synchronizeMap(SlaveSession* session) override;

protected:

    //The actual map
    InnerMapType mMap;
};


template <class ItemType, class BaseTdfType, class ContextType>
class ReplicatedMap : public ReplicatedMapTemplateBase<ItemType, BaseTdfType, ContextType>
{   
private:
    //GCC doesn't like us referring to our parent's members without qualifying it as our base
    //this typedef makes the code a bit more readable in that regard.
    typedef ReplicatedMapTemplateBase<ItemType, BaseTdfType, ContextType> base_type;

public:
    typedef ContextType Context;

    ReplicatedMap(CollectionId collectionId, MemoryGroupId memoryId, MapCollection& owner, bool dynamic) : 
        base_type(collectionId, memoryId, owner, dynamic) {}

    ReplicatedMapVersion insert(ObjectId key, ItemType& item, const ContextType* context);    
    ReplicatedMapVersion update(ObjectId key, ItemType& item, const ContextType* context);
    ReplicatedMapVersion cloneAndUpdate(ObjectId key, const BaseTdfType &newData, const ContextType* context);
    ReplicatedMapVersion erase(ObjectId key, const ContextType* context); 

private:
    friend class MapCollection;

};



template <class ItemType, class BaseTdfType>
class ReplicatedMap<ItemType, BaseTdfType, void> : public ReplicatedMapTemplateBase<ItemType, BaseTdfType, void>
{   
private:
    //GCC doesn't like us referring to our parent's members without qualifying it as our base
    //this typedef makes the code a bit more readable in that regard.
    typedef ReplicatedMapTemplateBase<ItemType, BaseTdfType, void> base_type;

public:  
    ReplicatedMap(CollectionId collectionId, MemoryGroupId memoryId, MapCollection& owner, bool dynamic) : 
        base_type(collectionId, memoryId, owner, dynamic) {}


    ReplicatedMapVersion insert(ObjectId key, ItemType& item);
    ReplicatedMapVersion update(ObjectId key, ItemType& item);
    ReplicatedMapVersion cloneAndUpdate(ObjectId key, const BaseTdfType &newData);
    ReplicatedMapVersion erase(ObjectId key);

private:
    friend class MapCollection;
};

template <class ItemType, class BaseTdfType, class ContextType>
ReplicatedMapTemplateBase<ItemType, BaseTdfType, ContextType>::~ReplicatedMapTemplateBase()
{
}


template <class ItemType, class BaseTdfType, class ContextType>
const ItemType *ReplicatedMapTemplateBase<ItemType, BaseTdfType, ContextType>::findItem(ObjectId id) const
{
    const ItemType* result = nullptr;   

    typename InnerMapType::const_iterator itr = mMap.find(id);
    if (itr != mMap.end())
        result = itr->second;

    return result;
}

template <class ItemType, class BaseTdfType, class ContextType>
ItemType *ReplicatedMapTemplateBase<ItemType, BaseTdfType, ContextType>::findItem(ObjectId id)
{
    ItemType* result = nullptr;

    typename InnerMapType::iterator itr = mMap.find(id);
    if (itr != mMap.end())
        result = itr->second;

    return result;
}

template <class ItemType, class BaseTdfType, class ContextType>
bool ReplicatedMapTemplateBase<ItemType, BaseTdfType, ContextType>::cloneItem(ObjectId key, BaseTdfType &copy)
{
    ItemType* item = findItem(key);
    if (item != nullptr)
        item->copyInto(copy);

    return item;
}


/*! ***************************************************************************/
/*! \brief Send a set of synchronization notices for a map.

Includes a create notification as well as a set of updates for the map.
This function will take ownership of the object and context tdfs, so pass clones.

\param session The session to send the synchronization messages to.
\param map The map to synchronize.
*******************************************************************************/
template <class ItemType, class BaseTdfType, class ContextType>
void ReplicatedMapTemplateBase<ItemType, BaseTdfType, ContextType>::
synchronizeMap(SlaveSession* session)
{
    BLAZE_TRACE_LOG(Log::REPLICATION, "[ReplicatedMap].synchronizeMap: Starting component=" << BlazeRpcComponentDb::getComponentNameById(getCollectionId().getComponentId()) 
               << "(" << SbFormats::Raw("0x%04x", getCollectionId().getComponentId()) << ") collection=" << getCollectionId().getCollectionNum());

    // First notify the subscriber of the map's existence (dynamic only)
    if (isDynamic())
    {
        getOwner().sendSyncCreateNotice(session, getCollectionId());
    }

    // Iterate over the map and send an update notification for each entry
    iterator it = begin();
    iterator itrend = end();
    for (; it != itrend; ++it) 
    {
        getOwner().sendSyncUpdateNotice(
                session, getCollectionId(), it.getObjectId(), **it, getVersion());
    }

    BLAZE_TRACE_LOG(Log::REPLICATION, "[ReplicatedMap].synchronizeMap: Finished component=" << BlazeRpcComponentDb::getComponentNameById(getCollectionId().getComponentId())
                   << "(" << SbFormats::Raw("0x%04x", getCollectionId().getComponentId())  << ") collection=" << getCollectionId().getCollectionNum());
}


template <class ItemType, class BaseTdfType, class ContextType>
ReplicatedMapVersion ReplicatedMap<ItemType, BaseTdfType, ContextType>
    ::insert(ObjectId key, ItemType& item, const ContextType *context)
{
    typename base_type::InnerMapType::insert_return_type ret = base_type::mMap.insert(key);
    ret.first->second = &item;
    ReplicatedMapVersion version;
    if (ret.second)
    {
        version = ReplicatedMapBase::insert(key, item, context);
    }
    else
    {
        version = ReplicatedMapBase::update(key, item, context);
    }
    return version;
}


template <class ItemType, class BaseTdfType, class ContextType>
ReplicatedMapVersion ReplicatedMap<ItemType, BaseTdfType, ContextType>
    ::update(ObjectId key, ItemType& item, const ContextType *context) 
{ 
    return ReplicatedMapBase::update(key, item, context);       
}

template <class ItemType, class BaseTdfType, class ContextType>
ReplicatedMapVersion ReplicatedMap<ItemType, BaseTdfType, ContextType>
    ::cloneAndUpdate(ObjectId key, const BaseTdfType& data, const ContextType *context) 
{ 
    ItemType *item = base_type::findItem(key);
    if (item != nullptr)
    {
        data.copyInto(*item);
        return ReplicatedMapBase::update(key, item, context); 
    }
    else
    {
        return BuildInstanceKey64(GetInstanceIdFromInstanceKey64(key), Blaze::INVALID_MAP_VERSION);
    }        
}


template <class ItemType, class BaseTdfType, class ContextType>
ReplicatedMapVersion ReplicatedMap<ItemType, BaseTdfType, ContextType>
    ::erase(ObjectId key, const ContextType *context)
{
    ReplicatedMapVersion result = BuildInstanceKey64(GetInstanceIdFromInstanceKey64(key), Blaze::INVALID_MAP_VERSION);
    typename base_type::iterator itr = base_type::mMap.find(key);
    if (itr !=  base_type::end())
    {
        base_type::mMap.erase(itr);        
        result = ReplicatedMapBase::erase(key, context);
    }

    return result;
}

template <class ItemType, class BaseTdfType>
ReplicatedMapVersion ReplicatedMap<ItemType, BaseTdfType, void>::insert(ObjectId key, ItemType& item) 
{
    typename base_type::InnerMapType::insert_return_type ret = base_type::mMap.insert(key);
    ret.first->second = &item;
    ReplicatedMapVersion version;
    if (ret.second)
    {
        version = ReplicatedMapBase::insert(key, item, nullptr);
    }
    else
    {
        version = ReplicatedMapBase::update(key, item, nullptr);
    }
    return version;
}

template <class ItemType, class BaseTdfType>
ReplicatedMapVersion ReplicatedMap<ItemType, BaseTdfType, void>::update(ObjectId key, ItemType& item) 
{ 
    return ReplicatedMapBase::update(key, item, nullptr); 
}

template <class ItemType, class BaseTdfType>
ReplicatedMapVersion ReplicatedMap<ItemType, BaseTdfType, void>::cloneAndUpdate(ObjectId key, const BaseTdfType& data) 
{ 
    ItemType *item = base_type::findItem(key);
    if (item != nullptr)
    {
        data.copyInto(*item);
        return ReplicatedMapBase::update(key, *item, nullptr); 
    }
    else
    {
        return BuildInstanceKey64(GetInstanceIdFromInstanceKey64(key), Blaze::INVALID_MAP_VERSION);
    }        
}


template <class ItemType, class BaseTdfType>
ReplicatedMapVersion ReplicatedMap<ItemType, BaseTdfType, void>::erase(ObjectId key)
{
    ReplicatedMapVersion result = BuildInstanceKey64(GetInstanceIdFromInstanceKey64(key), Blaze::INVALID_MAP_VERSION);
    typename base_type::iterator itr = base_type::mMap.find(key);
    if (itr != base_type::end())
    {

        base_type::mMap.erase(itr);        
        result = ReplicatedMapBase::erase(key, nullptr);
    }

    return result;  
}



} // Blaze

#endif // BLAZE_REPLICATEDMAP_H
