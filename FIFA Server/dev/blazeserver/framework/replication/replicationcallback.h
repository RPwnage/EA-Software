/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REPLICATIONCALLBACK_H
#define BLAZE_REPLICATIONCALLBACK_H

/*** Include files *******************************************************************************/
#include "framework/replication/replication.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class RemoteReplicatedMapBase;

template <class ReplicatedMapType> 
class ReplicatedMapFactory;

template <class MapType>
class ReplicatedMapSubscriber
{
public:    
    typedef ReplicatedMapSubscriber<MapType> this_type;
    typedef typename MapType::ItemType ItemType;

    virtual ~ReplicatedMapSubscriber() {}

    /*! ************************************************************************************************/
    /*! \brief dispatched after a TDF value is inserted into a RemoteReplicatedMap.  

        \param[in] replMap The replicated map
        \param[in] key The key of the new map element.
        \param[in] value The TDF that was inserted in the map.
        \param[in] context A TDF providing context for the erase 
                    (often meta data about the reason the element was inserted).
    ***************************************************************************************************/
    virtual void onInsert(const MapType& replMap, ObjectId key, ItemType& value) = 0;


    /*! ************************************************************************************************/
    /*! \brief dispatched after a TDF value is updated in a RemoteReplicatedMap.  

        Note: we don't retain a copy of the previous version of the TDF; if you need that info, 
            consider converting to a replicated map with context (so you can pass the old value(s)
            in the context tdf for the update).

        \param[in] replMap The  replicated map
        \param[in] key The key of the updated map element.
        \param[in] value The TDF that was updated in the map.  Note: value == map[key]
    ***************************************************************************************************/
    virtual void onUpdate(const MapType& replMap, ObjectId key, ItemType& value) = 0;

    /*! ************************************************************************************************/
    /*! \brief dispatched after a TDF value is removed from a RemoteReplicatedMap.  The erasedValue is deleted once we've
            finished dispatching onErase to all subscribers. 
    
        \param[in] replMap The replicated map
        \param[in] erasedKey The erasedKey of the removed map element.
        \param[in] erasedValue The TDF that was removed from the map (will be deleted after this dispatch is finished)
    ***************************************************************************************************/
    virtual void onErase(const MapType& replMap, ObjectId erasedKey, ItemType& erasedValue) = 0;


    static void makeInsertCall(this_type& subscriber, const RemoteReplicatedMapBase& replMap, ObjectId key, ItemType& value, const EA::TDF::Tdf*)
    {
        subscriber.onInsert((const MapType&) replMap, key, value);
    }

    static void makeUpdateCall(this_type& subscriber, const RemoteReplicatedMapBase& replMap, ObjectId key, ItemType& value, const EA::TDF::Tdf*)
    {
        subscriber.onUpdate((const MapType&) replMap, key, value);
    }

    static void makeEraseCall(this_type& subscriber, const RemoteReplicatedMapBase& replMap, ObjectId key, ItemType& oldObj, const EA::TDF::Tdf*)
    {
        subscriber.onErase((const MapType&) replMap, key, oldObj);
    }
};

template <class MapType>
class ReplicatedMapContextSubscriber
{
public:
    typedef ReplicatedMapContextSubscriber<MapType> this_type;
    typedef typename MapType::ItemType ItemType;
    typedef typename MapType::ContextType ContextType;

    virtual ~ReplicatedMapContextSubscriber() {}

    /*! ************************************************************************************************/
    /*! \brief dispatched after a TDF value is inserted into a RemoteReplicatedMap.  
            
        \param[in] replMap The replicated map
        \param[in] key The key of the new map element.
        \param[in] value The TDF that was inserted in the map.
        \param[in] context A TDF providing context for the erase 
                    (often meta data about the reason the element was inserted).
    ***************************************************************************************************/
    virtual void onInsert(const MapType& replMap, ObjectId objectId, ItemType& tdfObject, const ContextType* tdfContext) = 0;

    /*! ************************************************************************************************/
    /*! \brief dispatched after a TDF value is updated in a RemoteReplicatedMap. 

        Note: we don't retain a copy of the previous version of the TDF, if you need that info, 
            it's best to pass it via the context tdf.

        \param[in] replMap The replicated map
        \param[in] key The key of the updated map element.
        \param[in] value The TDF that was updated in the map.  Note: value == map[key]
        \param[in] context A TDF providing context for the update 
                    (often meta data about the fields in the TDF that changed).
    ***************************************************************************************************/
    virtual void onUpdate(const MapType& replMap, ObjectId key, ItemType& value, const ContextType* tdfContext) = 0;

    /*! ************************************************************************************************/
    /*! \brief dispatched after a TDF value is removed from a RemoteReplicatedMap.  The erasedValue is deleted once we've
            finished dispatching onErase to all subscribers.  
    
        \param[in] replMap The replicated map
        \param[in] erasedKey The erasedKey of the removed map element.
        \param[in] erasedValue The TDF that was removed from the map (will be deleted after this dispatch is finished)
        \param[in] context A TDF providing context for the erase 
                    (often meta data about the reason the element was removed).
    ***************************************************************************************************/
    virtual void onErase(const MapType& replMap, ObjectId erasedKey, ItemType& erasedValue, const ContextType* context) = 0;

    static void makeInsertCall(this_type& subscriber, const RemoteReplicatedMapBase& replMap, ObjectId key, ItemType& value, const EA::TDF::Tdf *context)
    {
        subscriber.onInsert((const MapType&) replMap, key, value, (const ContextType *) context);
    }

    static void makeUpdateCall(this_type& subscriber, const RemoteReplicatedMapBase& replMap, ObjectId key, ItemType& value, const EA::TDF::Tdf *context)
    {
        subscriber.onUpdate((const MapType&) replMap, key, value, (const ContextType *) context);
    }

    static void makeEraseCall(this_type& subscriber, const RemoteReplicatedMapBase& replMap, ObjectId key, ItemType& oldObj, const EA::TDF::Tdf *context)
    {
        subscriber.onErase((const MapType&) replMap, key, oldObj, (const ContextType *) context);
    }
};

template <class ReplicatedMapType>
class DynamicMapFactorySubscriber
{
public:
    virtual ~DynamicMapFactorySubscriber() {}

    typedef DynamicMapFactorySubscriber<ReplicatedMapType> this_type;

    // Must return a factory class for use with the map
    virtual void onCreateMap(ReplicatedMapType& replMap) = 0;
    virtual void onDestroyMap(ReplicatedMapType& replMap) = 0;

private:
    friend class ReplicatedMapFactory<ReplicatedMapType>;
    friend class ReplicatedMapFactory<typename ReplicatedMapType::SharedType>;

    static void makeCreateMapCall(this_type& subscriber, const RemoteReplicatedMapBase& replMap, const EA::TDF::Tdf *)
    {
        subscriber.onCreateMap((ReplicatedMapType&) replMap);
    }

    static void makeDestroyMapCall(this_type& subscriber, const RemoteReplicatedMapBase& replMap, const EA::TDF::Tdf *)
    {
        subscriber.onDestroyMap((ReplicatedMapType&) replMap);
    }
};

template <class ReplicatedMapType, class ContextType>
class DynamicMapFactoryContextSubscriber
{
public:
    virtual ~DynamicMapFactoryContextSubscriber() {}

    typedef DynamicMapFactoryContextSubscriber<ReplicatedMapType, ContextType> this_type;

    // Must return a factory class for use with the map
    virtual void onCreateMap(ReplicatedMapType& replMap, const ContextType* context) = 0;
    virtual void onDestroyMap(ReplicatedMapType& replMap, const ContextType* context) = 0;

private:
    friend class ReplicatedMapFactory<ReplicatedMapType>;
    friend class ReplicatedMapFactory<typename ReplicatedMapType::SharedType>;

    static void makeCreateMapCall(this_type& subscriber, const RemoteReplicatedMapBase& replMap, const EA::TDF::Tdf* context)
    {
        subscriber.onCreateMap((ReplicatedMapType&) replMap, (const ContextType*) context);
    }

    static void makeDestroyMapCall(this_type& subscriber, const RemoteReplicatedMapBase& replMap, const EA::TDF::Tdf* context)
    {
        subscriber.onDestroyMap((ReplicatedMapType&) replMap, (const ContextType*) context);
    }
};

class ReplicatedDataMediator
{
public:
    virtual ~ReplicatedDataMediator() {}

    virtual void setup() = 0;
};


} // Blaze

#endif // BLAZE_REPLICATIONCALLBACK_H
