/*! ************************************************************************************************/
/*!
    \file replicateditem.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_REPLICATED_ITEM_H
#define BLAZE_REPLICATED_ITEM_H

/*** Include files *******************************************************************************/

#include "PPMalloc/EAFixedAllocator.h"


/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

template <class T> class ReplicatedSafePtr;

template <class T>
class ReplicatedMapItem : public T
{
public:

    ReplicatedMapItem(Blaze::MemoryGroupId memGroupId = (DEFAULT_BLAZE_MEMGROUP)) : T(*Allocator::getAllocator(memGroupId), "ReplMapItem"), mReplicatedMapItemRefCount(0), mPool(nullptr) {}
    ~ReplicatedMapItem() override {}

    void incRef() 
    { 
        ++mReplicatedMapItemRefCount;
    }

    void decRef()
    {
        if (--mReplicatedMapItemRefCount == 0)
        {
            // The Replication Pool items (those not held on the master) do NOT go through the normal Tdf allocation process.
            // Instead, they use a seperate allocation pool, and should always have one reference (added when setPool is called).
            if (mPool != nullptr)
            {
                this->~ReplicatedMapItem();
                mPool->Free(this);
            }
            else
                this->Release();
        }
    }

    const T* getPrevSnapshot() const { return mPrevItem; }
    void saveSnapshot() { mPrevItem = BLAZE_NEW T(*this); }
    void resetSnapshot() { mPrevItem.reset(); }

    void setPool(EA::Allocator::FixedAllocatorBase* pool) 
    { 
        mPool = pool;
        if (mPool != nullptr)
        {
            this->AddRef();    // We add a self reference so that things that use the Tdf directly won't accidently try to delete it. 
        }
    }
    
private:

    NON_COPYABLE(ReplicatedMapItem);
    EA::TDF::tdf_ptr<T> mPrevItem;
    int32_t mReplicatedMapItemRefCount;
    EA::Allocator::FixedAllocatorBase* mPool;
};


template <class T>
class ReplicatedSafePtr
{
public:
    ReplicatedSafePtr() : mItem(nullptr){}
    
    explicit ReplicatedSafePtr(T* item) : mItem(item) 
    { 
        if (mItem != nullptr) mItem->incRef(); 
    }

    ReplicatedSafePtr(const ReplicatedSafePtr &other) : mItem(other.mItem)
    {
        if (mItem != nullptr) mItem->incRef();
    }

    ~ReplicatedSafePtr() 
    { 
        if (mItem != nullptr) mItem->decRef(); 
    }

    ReplicatedSafePtr &operator=(const ReplicatedSafePtr &other)
    {
        if (&other != this) acquire(other.mItem);
        
        return *this;
    }
    
    ReplicatedSafePtr &operator=(T* item)
    {
        acquire(item);

        return *this;
    }
    
    void acquire(T* item)
    {
        if (item != nullptr) item->incRef();
        
        if (mItem != nullptr) mItem->decRef();
        
        mItem = item;
    }
    
    void release()
    {
        if (mItem != nullptr)
        {
            mItem->decRef();
            
            mItem = nullptr;
        }
    }
    
    const T& operator->() const { return *mItem; }
    
    T* operator->() { return mItem; }

    T& operator*() { return *mItem; }
    
    T *getItem() const { return mItem; }
    
    bool isValid() const { return mItem != nullptr; }

    bool operator==(const T* val) const { return mItem == val; }
    bool operator!=(const T* val) const { return mItem != val; }
    
private:

    T *mItem;

}; 


} // Blaze

#endif // BLAZE_REPLICATED_ITEM_H

