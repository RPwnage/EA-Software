/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_LRUCACHE_H
#define BLAZE_LRUCACHE_H

/*** Include files *******************************************************************************/

#include "framework/callback.h"
#include "EASTL/intrusive_list.h"
#include "EASTL/hash_map.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

/*************************************************************************************************/
/*!
    \class LruCache

    This templated class implements a simple least-recently-used (LRU) cache.  The cache is a
    fixed size.  If the cache is full and new elements are added, the items already in the cache
    that have been accessed least recently will be automatically expired to make room for the
    new elements.
*/
/*************************************************************************************************/

template < typename K, typename V, typename Hash = eastl::hash<K>, typename Predicate = eastl::equal_to<K> >
class LruCache
{
public:
    typedef K key_type;
    typedef V value_type;

    typedef Functor2<const K&, V> RemoveCb;

    /*! \brief Construct a new cache with the given size.
     *
     * An optional callback may be provided to handle deletion of elements when they are expired
     * due to inactivity.  If no handler is provided, the objects will just be deleted when
     * removed.
     */
    LruCache(size_t size, const BlazeStlAllocator& allocator, const RemoveCb& removeCb = RemoveCb())
        : mMaxSize(0), // intentionally set to 0 as setMaxSize will take care of updating it to the correct value
          mValuesByKey(allocator),
          mRemoveCb(removeCb)
    {
        EA_ASSERT(size > 0);

        // EASTL hashtable constructor asserts on a size of 10 million or more,
        // but it is allowed to grow beyond that, so we will be consistent and use setMaxSize
        // both when we are constructing our LRU cache, and any time after that if we wish
        // to resize our LRU cache
        setMaxSize(size);
    }

    virtual ~LruCache()
    {
        for(key_iterator i = mValuesByKey.begin(), e = mValuesByKey.end(); i != e; ++i)
        {
            releaseNode(i->second);
        }
    }

    /*! \brief Add a new element to the cache.
     *
     * An existing element may be removed due to this operation if the cache is full.
     * An existing element may be replaced with the new element if a key collision occurs.
     */
    void add(const K& key, const V& value)
    {
        insert_return_type result = mValuesByKey.insert(key);
        node& n = result.first->second;
        if (result.second)
        {
            if (mValuesByKey.size() > mMaxSize)
            {
                node& old = mValuesByAccess.back();
                releaseNode(old);

                // Note that it is important that we do not simply call erase(old.key)
                // as that would be passing in a reference to memory that will not exist
                // for the entire duration of the call as the erase call itself
                // will delete the object from under our feet, instead we make a copy
                K oldKey = old.key;
                mValuesByKey.erase(oldKey);
            }
            mValuesByAccess.push_front(n);
        }
        else
        {
            releaseNode(n);
            mValuesByAccess.push_front(n);
        }
        n.key = key;
        n.value = value;
    }

    /*! \brief Explicitly remove an element from the cache.
     */
    bool remove(const K& key)
    {
        key_iterator find = mValuesByKey.find(key);
        if (find == mValuesByKey.end())
            return false;

        remove(find);
        return true;
    }

    /*! \brief Remove items from the lru based on a callback result. 
     *
     *  Note: This call is O(n) time since it must iterate through the entire list. Be very careful about how 
     *  this is used. 
     
        @param[in] decisionCb Class that implements () which returns bool, true for remove. 
        @return number of elements removed. 
    */
    template<class DecisionCb> 
    size_t removeMultiple(DecisionCb decisionCb) 
    { 
        key_iterator iter = mValuesByKey.begin(); 
        key_iterator next = mValuesByKey.begin(); 
        size_t removeCount = 0; 
        
        for(; iter != mValuesByKey.end(); iter = next) 
        { 
            next++; 
            if (decisionCb(iter->first, iter->second.value)) 
            { 
                removeCount++; 
                remove(iter);
            } 
        } 
        
        return removeCount; 
    } 

    /*! \brief Lookup an element in the cache.
     *
     * Accessing an element will move it to the back of the expiry queue and prevent it from
     * being deleted by subsequent adds to the cache.
     */
    bool get(const K& key, V& value)
    {
        key_iterator find = mValuesByKey.find(key);
        if (find == mValuesByKey.end())
            return false;

        // Move the accessed item up the LRU chain to prevent its expiry
        node& node = find->second;
        mValuesByAccess.remove(node);
        mValuesByAccess.push_front(node);
        value = node.value;
        return true;
    }

    /*! \brief Empty the cache
    *
    * Deletes all elements in the cache, resetting it for use.
    */
    void reset()
    {
        key_iterator i = mValuesByKey.begin();
        key_iterator e = mValuesByKey.end();

        for(; i != e; ++i)
        {
            releaseNode(i->second);
        }
        mValuesByKey.clear();
    }

    /*! \brief Return current number of elements in cache
    *
    * Return the current number of elements in the cache determined by the current 
    * size of the underlying hashmap.  mValuesByKey was used rather than mValuesByAccess
    * due to the latter's implementation of determining size.
    */
    size_t getSize() const
    {
        return mValuesByKey.size();
    }

    /*! \brief Return max number of elements in cache
    *
    * Return the max number of elements allowed to be added in cache
    */
    size_t getMaxSize() const
    {
        return mMaxSize;
    }

    size_t setMaxSize(size_t size)
    {
        size_t removedCount = 0;
        if (size == mMaxSize)
            return removedCount;

        mMaxSize = size;
        while (mValuesByKey.size() > mMaxSize)
        {
            node& old = mValuesByAccess.back();
            releaseNode(old);

            // Note that it is important that we do not simply call erase(old.key)
            // as that would be passing in a reference to memory that will not exist
            // for the entire duration of the call as the erase call itself
            // will delete the object from under our feet, instead we make a copy
            K oldKey = old.key;
            mValuesByKey.erase(oldKey);
            ++removedCount;
        }

        if (removedCount > 0)
        {
            // NOTE: Only rehash if we shrunk the table, otherwise leave it alone
            // This mirrors the code in EASTL's hashtable constructor which uses the rehash policy
            // to ensure we setup the bucket count to be a reasonable prime, the rehash method
            // itself respects whatever value is passed in
            mValuesByKey.rehash(mValuesByKey.rehash_policy().GetNextBucketCount(mMaxSize));
        }

        return removedCount;
    }

private:

    struct node : eastl::intrusive_list_node
    {
        K key;
        V value;
    };

    typedef eastl::intrusive_list<node> ValuesByAccess;
    typedef typename ValuesByAccess::iterator access_iterator;

    typedef eastl::hash_map<K, node, Hash, Predicate> ValuesByKey;
    typedef typename ValuesByKey::iterator key_iterator;
    typedef typename ValuesByKey::insert_return_type insert_return_type; 

    void remove(key_iterator item)
    {
        releaseNode(item->second);
        mValuesByKey.erase(item);
    }

    void releaseNode(node& node)
    {
        mRemoveCb(node.key, node.value);
        mValuesByAccess.remove(node);
    }

private:

    size_t mMaxSize;
    ValuesByAccess mValuesByAccess;
    ValuesByKey mValuesByKey;
    RemoveCb mRemoveCb;
};

} // Blaze

#endif // BLAZE_LRUCACHE_H

